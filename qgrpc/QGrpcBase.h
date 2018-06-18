#pragma once
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <memory>
#include "assert.h"
#include <atomic>

using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using grpc::CompletionQueue;
using grpc::Server;
#ifndef WIN32
#if (__cplusplus >= 201103L && __cplusplus < 201402L)
namespace std {
	// Note: despite not being even C++11 compliant, Visual Studio 2013 has their own implementation of std::make_unique.
	// Define std::make_unique for pre-C++14
	template<typename T, typename... Args> inline unique_ptr<T> make_unique(Args&&... args) {
		return unique_ptr<T>(new T(forward<Args>(args)...));
	}
} // namespace std
#endif // C++11 <= version < C++14
#endif // WIN32


namespace QGrpcBase
{

	class AbstractServer
	{
	public:
		virtual CompletionQueue* CQ() = 0;
		virtual void Shutdown() = 0;
		virtual bool Started() = 0;
	};


    class AbstractService 
	{ 
	public:
		virtual void CheckCQ() = 0; 
		virtual std::string ListeningPort() = 0;
		virtual grpc::Service* Service() = 0;
		virtual void Start(const AbstractServer* server) = 0;
		virtual void PrepareForShutdown() = 0;
	};
}



