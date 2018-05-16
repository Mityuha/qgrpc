#pragma once

#include "QGrpcBase.h"

using grpc::Channel;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncWriter;
using grpc::ClientAsyncReaderWriter;
using grpc::CompletionQueue;
using grpc::Status;


namespace QGrpcCliBase
{

	struct RPC_KIND_UNARY_T;
	struct RPC_KIND_SERVERSTREAMING_T;
	struct RPC_KIND_CLIENTSTREAMING_T;
	struct RPC_KIND_BIDISTREAMING_T;

	template<typename RequestType, typename ReplyType>
	struct ClientCallDataResponse
	{
		RequestType request;
		ReplyType reply;
	};


	template< template<typename... Args> class R, typename...Args> struct ClientResponderBase
	{
		using responder_type = std::unique_ptr< R< Args...> >;
		ClientContext context;
		Status status;
		ClientResponderBase() :couldBeDeleted_(false), tag_(nullptr) {}
		bool CouldBeDeleted() const { return couldBeDeleted_; };
		virtual ~ClientResponderBase() {}
	protected:
		responder_type responder; //must be deleted before context
		bool couldBeDeleted_;
		void* tag_;
		virtual bool processEvent(void*, bool) = 0;
	};

	template<typename KIND, typename ReplyType, typename RequestType> struct ClientResponder;
	template<typename ReplyType, typename RequestType>
	struct ClientResponder<RPC_KIND_UNARY_T, ReplyType, RequestType> : public ClientResponderBase< ClientAsyncResponseReader, ReplyType >, public ClientCallDataResponse<RequestType, ReplyType>
	{
		virtual ~ClientResponder() {}
	protected:
		virtual bool processEvent(void* tag, bool) override { this->tag_ = tag; return (this->couldBeDeleted_ = true); }
	};

	template<typename ReplyType, typename RequestType>
	struct ClientResponder<RPC_KIND_SERVERSTREAMING_T, ReplyType, RequestType> : public ClientResponderBase< ClientAsyncReader, ReplyType >, public ClientCallDataResponse<RequestType, ReplyType>
	{
		ClientResponder() : isFinished_(false) {}
		virtual ~ClientResponder() {}
		inline bool StreamFinished() const { return isFinished_; }
		inline void Finish() { this->responder->Finish(&this->status, this->tag_); this->couldBeDeleted_ = true; }
		inline void Read() { return this->responder->Read(&this->reply, this->tag_); }
	protected:
		virtual bool processEvent(void* tag , bool ok) override 
		{ 
			this->tag_ = tag; 
			isFinished_ = !ok; 
			return true; 
		}
		bool isFinished_;
	};

	template<typename ReplyType, typename RequestType>
	struct ClientResponder<RPC_KIND_CLIENTSTREAMING_T, ReplyType, RequestType> : public ClientResponderBase< ClientAsyncWriter, RequestType >, public ClientCallDataResponse<RequestType, ReplyType>
	{
		ClientResponder() : write_mode_(true) {}
		inline bool WriteMode() const { return write_mode_; }
		inline void ChangeMode() { write_mode_ = !write_mode_; }
		inline void Write() { return this->responder->Write(this->request, this->tag_); }
		inline void WritesDone() { return this->responder->WritesDone(this->tag_); }
		inline void Finish() { this->couldBeDeleted_ = true; return this->responder->Finish(&this->status, this->tag_); }
	protected:
		virtual bool processEvent(void* tag, bool ok) override
		{
			this->tag_ = tag;
			if (!this->couldBeDeleted_) this->couldBeDeleted_ = !ok;
			return true;
		}
		bool write_mode_;
	};

	template<typename ReplyType, typename RequestType>
	struct ClientResponder<RPC_KIND_BIDISTREAMING_T, ReplyType, RequestType> : public ClientResponderBase< ClientAsyncReaderWriter, RequestType, ReplyType >, public ClientCallDataResponse<RequestType, ReplyType>
	{
		ClientResponder() : write_mode_(true), isFinished_(false) {}
		inline bool WriteMode() const { return write_mode_; }
		inline void ChangeMode() { write_mode_ = !write_mode_; }
		inline void Write() { return this->responder->Write(this->request, this->tag_); }
		inline void WritesDone() { return this->responder->WritesDone(this->tag_); }
		inline void Finish() { this->couldBeDeleted_ = true; return this->responder->Finish(&this->status, this->tag_); }
		inline bool StreamFinished() const { return isFinished_; }
		inline void Read() { return this->responder->Read(&this->reply, this->tag_); }
	protected:
		virtual bool processEvent(void* tag, bool ok) override
		{
			this->tag_ = tag;
			if (!this->couldBeDeleted_) this->couldBeDeleted_ = !ok;
			return true;
		}
		bool write_mode_;
		bool isFinished_;
	};


	inline std::chrono::system_clock::time_point deadlineFromSec(long long seconds) { return std::chrono::system_clock::now() + std::chrono::seconds(seconds); }
	inline std::chrono::system_clock::time_point deadlineFromMSec(long long mseconds) { return std::chrono::system_clock::now() + std::chrono::milliseconds(mseconds); }


	class simple_lock_guard
	{
		std::atomic<bool>& is_locked_;
	public:
		simple_lock_guard(std::atomic<bool>& is_locked_var) : is_locked_(is_locked_var)
		{
			while (is_locked_.load());
			is_locked_.store(true);
		}
		~simple_lock_guard() { is_locked_.store(false); }
	};


	class ChannelFeatures
	{
		static const int nettag = 0xff;
		CompletionQueue channel_cq_;
		long long mseconds_;
		grpc_connectivity_state state_;
		std::shared_ptr<Channel> channel_;
		mutable std::atomic<bool> me_locked_;
	public:
		explicit ChannelFeatures(std::shared_ptr<Channel>& channel) : mseconds_(1000u), state_(grpc_connectivity_state::GRPC_CHANNEL_CONNECTING), channel_(channel), me_locked_(false){}
		~ChannelFeatures() 
		{
			auto tmp = simple_lock_guard(me_locked_);
			waitForTimeout_(); 
			channel_cq_.Shutdown();
		}

		inline grpc_connectivity_state checkChannelState()
		{
			auto tmp = simple_lock_guard(me_locked_);
			void* tag;
			bool ok;
			/*auto st = */channel_cq_.AsyncNext(&tag, &ok, std::chrono::system_clock::time_point());
			subscribeOnChannelState_();
			state_ = channelState_();
			return state_;
		}

		inline grpc_connectivity_state channelState() const 
		{
			auto tmp = simple_lock_guard(me_locked_);
			return state_; 
		}

	private:
		inline bool channelTag_(void* tag) { return static_cast<int>(reinterpret_cast< intptr_t >(tag)) == nettag; }
		inline grpc_connectivity_state channelState_(bool try_to_connect = true) const { return channel_->GetState(try_to_connect); }
		inline void subscribeOnChannelState_()
		{
			if (!channel_) return;
			channel_->NotifyOnStateChange(state_, std::chrono::system_clock::time_point(), &channel_cq_, (void*)nettag);
		}
		void waitForTimeout_()
		{
			if (!channel_) return;
			void* tag;
			bool ok = false;
			while ((channel_cq_.AsyncNext(&tag, &ok, std::chrono::system_clock::time_point()) != grpc::CompletionQueue::TIMEOUT));
		}
	};


	struct AbstractConnectivityFeatures
	{
		virtual grpc_connectivity_state channelState() const = 0;
		virtual grpc_connectivity_state checkChannelState() const = 0;
		virtual bool connected() const = 0;
		CompletionQueue cq_;
	};


	template<typename GRPCService>
	class ConnectivityFeatures : public AbstractConnectivityFeatures
	{
		std::string target_;
		std::shared_ptr< grpc::ChannelCredentials > creds_;
		std::shared_ptr<Channel> channel_;
		std::unique_ptr<ChannelFeatures> channelFeatures_;
		std::atomic<bool> connected_;
	protected:
		std::unique_ptr<typename GRPCService::Stub> stub_;
	public:
		explicit ConnectivityFeatures() :
			connected_(false)
		{}
		void grpc_connect(const std::string& target, const std::shared_ptr< grpc::ChannelCredentials >& creds = grpc::InsecureChannelCredentials())
		{
			if (connected_.load()) return;
			target_ = target;
			creds_ = creds;
			grpc_connect_();
		}
		void grpc_disconnect()
		{
			if (!connected_.load()) return;
			connected_.store(false);
			channelFeatures_ = nullptr;
			channel_ = nullptr;
			stub_ = nullptr;
			//creds_.reset();
		}

		void grpc_reconnect()
		{
			grpc_disconnect();
			grpc_connect_();
		}
	protected:
		virtual grpc_connectivity_state channelState() const override 
		{ 
			if (!channelFeatures_) return grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN; 
			return channelFeatures_->channelState(); 
		}
		virtual grpc_connectivity_state checkChannelState() const override 
		{ 
			if (!channelFeatures_) return grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN; 
			return channelFeatures_->checkChannelState(); 
		}
		virtual bool connected() const override { return connected_.load(); }
	private:
		void grpc_connect_()
		{
			assert(!target_.empty()); assert(creds_);
			auto new_channel_ = grpc::CreateChannel(target_, creds_);
			channel_.swap(new_channel_);
			auto new_stub_ = GRPCService::NewStub(channel_);
			stub_.swap(new_stub_);
			channelFeatures_ = std::make_unique<ChannelFeatures>(channel_);
			channelFeatures_->checkChannelState();
			connected_.store(true);
		}
	};

	template<typename SERVICE>
	struct AbstractCallData { virtual void cqActions(const SERVICE*, bool) = 0; };

	template<typename SERVICE>
	class MonitorFeatures : public QGrpcBase::AbstractService
	{
		AbstractConnectivityFeatures* conn_;
		using FuncType = void (SERVICE::*)(int, int);
		FuncType channelStateChangedSignal_;
		bool not_connected_notified_;
	public:
		explicit MonitorFeatures(AbstractConnectivityFeatures* conn, FuncType signal) : conn_(conn), channelStateChangedSignal_(signal), not_connected_notified_(false) {}
		virtual void checkCQ() override
		{
			auto service_ = dynamic_cast< SERVICE* >(this);
			if (!conn_->connected())
			{
				if (not_connected_notified_) return;
				(const_cast< SERVICE* >(service_)->*channelStateChangedSignal_)(static_cast<int>(grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN), static_cast<int>(grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN));
				not_connected_notified_ = true;
				return;
			}

			auto old_state = conn_->channelState();
			auto new_state = conn_->checkChannelState();
			if (old_state != new_state)
			{
				(const_cast< SERVICE* >(service_)->*channelStateChangedSignal_)(static_cast<int>(old_state), static_cast<int>(new_state));
				not_connected_notified_ = false;
			}
			//
			void* tag;
			bool ok = false;
			grpc::CompletionQueue::NextStatus st;
			//auto deadline = QGrpcCliBase::deadlineFromMSec(100);
			st = conn_->cq_.AsyncNext(&tag, &ok, std::chrono::system_clock::time_point());
			if ((st == grpc::CompletionQueue::SHUTDOWN) || (st == grpc::CompletionQueue::TIMEOUT))/* || (st != grpc::CompletionQueue::GOT_EVENT) || !ok)*/
				return;
			static_cast< AbstractCallData< SERVICE >* >(tag)->cqActions(service_, ok);
			return;
		}
	};

	template<typename RPC, typename RPCCallData>
	class ClientCallData : public AbstractCallData<typename RPC::Service>, public ClientResponder< typename RPC::kind, typename RPC::ReplyType, typename RPC::RequestType >
	{
		using FuncType = void (RPC::Service::*)(RPCCallData*);
		FuncType func_;
	public:
		explicit ClientCallData(FuncType func) :func_(func) {}
		virtual ~ClientCallData() {}
		inline virtual void cqActions(const typename RPC::Service* service, bool ok) override
		{
			auto response = dynamic_cast<RPCCallData*>(this);
			void* tag = static_cast<void*>(response);
			if (!this->processEvent(tag, ok)) return;
			/*if (response->couldBeDeleted())
			{
			delete response;
			return;
			}*/
			(const_cast< typename RPC::Service* >(service)->*func_)( response );
		}
		friend typename RPC::Service;
	};

	template<typename KIND, typename REQUEST, typename REPLY, typename SERVICE> struct RPCtypes
	{
		using kind = KIND;
		using ReplyType = REPLY;
		using RequestType = REQUEST;
		using Service = SERVICE;
	};

}
