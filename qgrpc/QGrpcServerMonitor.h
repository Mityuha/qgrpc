#pragma once

#include <QThread>
#include <QTimer>
#include <set>
#include "QGrpcBase.h"

class QSrvServerPrivate : public QObject, public QGrpcBase::AbstractServer
{
	Q_OBJECT
	QTimer* cq_timer_ = nullptr;
	std::atomic<bool> started_ = false;
	std::set<QGrpcBase::AbstractService*> services_;
	std::unique_ptr<Server> server_;
	std::unique_ptr<ServerCompletionQueue> server_cq_;
public:
	inline void addService(QGrpcBase::AbstractService* const service) { services_.insert(service); }
	inline void deleteService(QGrpcBase::AbstractService* const service) { services_.erase(service); }

	inline virtual CompletionQueue* CQ() override { return server_cq_.get(); }
	virtual void Shutdown() override
	{
		started_.store(false);
		if (server_)
		{
			server_->Shutdown(std::chrono::system_clock::time_point());
			for (const auto service : services_)
				service->PrepareForShutdown();
			server_->Wait();
		}
		if (server_cq_)
		{
			server_cq_->Shutdown(); // Always after the associated server's Shutdown()! 
									// Drain the cq_ that was created 
			void* ignored_tag; bool ignored_ok;
			while (server_cq_->Next(&ignored_tag, &ignored_ok)) {}
		}
		server_ = nullptr;
		server_cq_ = nullptr;
	}

	virtual bool Started() override { return started_.load(); }

	virtual ~QSrvServerPrivate() 
	{
		Shutdown();
	}
public slots :
	void start()
	{
		if (!cq_timer_)
		{
			cq_timer_ = new QTimer(this);
			bool ok = connect(cq_timer_, SIGNAL(timeout()), this, SLOT(AsyncMonitorRpc())); assert(ok);
		}
		RegisterServices();
		cq_timer_->start();
	}
	void stop()
	{
		Shutdown();
	}

	
private slots:
	void AsyncMonitorRpc()
	{
		if (!started_.load())
		{
			cq_timer_->stop();
			return;
		}
		for (auto* const service : services_)
			service->CheckCQ();
	}
private:
	void RegisterServices()
	{
		ServerBuilder builder;
		for (const auto service : services_)
		{
			builder.AddListeningPort(service->ListeningPort(), grpc::InsecureServerCredentials());
			builder.RegisterService(service->Service());
		}
		server_cq_ = builder.AddCompletionQueue();
		server_ = builder.BuildAndStart();
		started_ = true;
		for (const auto service : services_)
			service->Start(this);
	}

};

class QGrpcSrvServer : public QObject
{
	Q_OBJECT
	QThread serverThread_;
	QSrvServerPrivate server_;
public:
	QGrpcSrvServer()
	{
		server_.moveToThread(&serverThread_);
		//bool c = connect(&serverThread_, SIGNAL(started()), &server_, SLOT(start())); assert(c);
		//bool c = connect(&serverThread_, SIGNAL(finished()), &server_, SLOT(stop())); assert(c);
		//c = connect(&monitorThread, SIGNAL(finished()), &monitor, SLOT(deleteLater())); assert(c);
		bool c = connect(this, SIGNAL(toStart()), &server_, SLOT(start())); assert(c);
		//c = connect(this, SIGNAL(toStop()), &server_, SLOT(stop()), Qt::QueuedConnection); assert(c);
	}
	inline void addService(QGrpcBase::AbstractService* const service) { server_.addService(service); }
	inline void deleteService(QGrpcBase::AbstractService* const service) { server_.deleteService(service); }

	inline void start()
	{
		if (!serverThread_.isRunning())
			serverThread_.start();
		emit toStart();
	}
	
	~QGrpcSrvServer()
	{
		server_.stop();
		serverThread_.quit();
		serverThread_.wait();
	}
	inline QThread* grpcThread() const { return const_cast<QThread*>(&(this->serverThread_)); }
signals:
	void toStart();
	//void toStop();
};