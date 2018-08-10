#pragma once
#include <grpc++/grpc++.h>
#include <grpc/support/log.h>
#include <memory>
#include "assert.h"
#include <atomic>


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


namespace QGrpcSrvAbstract
{
    struct AbstractService 
    { 
        virtual void CheckCQ() = 0; 
        virtual void PrepareForShutdown() = 0;
    };
}



namespace QGrpcCliAbstract
{
    class AbstractService
    {
    public:
        virtual bool CheckCQ() = 0;
    };
}





