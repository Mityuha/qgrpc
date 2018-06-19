#pragma once

#include <QThread>
#include <QTimer>
#include <set>
#include "QAbstractGrpc.h"


struct SleepIfActive
{
	explicit SleepIfActive(size_t msec) : msec(msec) {}
	size_t msec;
};

struct SleepIfInactive
{
	explicit SleepIfInactive(size_t msec) : msec(msec) {}
	size_t msec;
};

class QCliServerPrivate : public QObject
{
	Q_OBJECT
	struct ServiceConfig
	{
		//QGrpcCliBase::AbstractService* service;
		size_t active_msec;
		size_t inactive_msec;
		QTimer* timer;
	};
	std::map<QGrpcCliAbstract::AbstractService*, ServiceConfig> services;
public:
	inline void addService(QGrpcCliAbstract::AbstractService* const service, SleepIfActive amsec, SleepIfInactive iamsec)
	{
		services[service] = { amsec.msec, iamsec.msec, nullptr };	
	}
	inline void deleteService(QGrpcCliAbstract::AbstractService* const service)
	{
		services.erase(service);
	}

	virtual ~QCliServerPrivate()
	{
	}
public slots :
	void start()
	{
		for (auto& p : services)
		{
			if (!p.second.timer)
			{
				QTimer* timer = new QTimer(this);
				bool ok = connect(timer, SIGNAL(timeout()), this, SLOT(MonitorRpc())); assert(ok);
				p.second.timer = timer;
			}
			p.second.timer->start(p.second.active_msec);
		}
	}
	void stop()
	{
		for (const auto& p : services)
			stop(p.first);
	}

	void stop(QGrpcCliAbstract::AbstractService* const service)
	{
		services.at(service).timer->stop();
	}
private slots:
	void MonitorRpc()
	{
		auto timer = qobject_cast<QTimer*>(QObject::sender());
		if (!timer->isActive())
			return;
		auto service_ = serviceByTimer(timer);
		auto config = services.at(service_);
		bool active = service_->CheckCQ();
		if (active)
			config.timer->start(config.active_msec);
		else
			config.timer->start(config.inactive_msec);
	}
private:
	QGrpcCliAbstract::AbstractService* serviceByTimer(QTimer* timer)
	{
		using spair = const std::pair<QGrpcCliAbstract::AbstractService*, ServiceConfig>;
		auto it = std::find_if(services.cbegin(), services.cend(), [&timer](spair it) {return it.second.timer == timer; });
		assert(it != services.cend());
		return it->first;
	}
};

class QGrpcCliServer : public QObject
{
	Q_OBJECT
	QThread serverThread_;
	QCliServerPrivate server_;
public:
	QGrpcCliServer()
	{
		server_.moveToThread(&serverThread_);
		bool c = connect(&serverThread_, SIGNAL(started()), &server_, SLOT(start())); assert(c);
		c = connect(&serverThread_, SIGNAL(finished()), &server_, SLOT(stop())); assert(c);
		c = connect(this, SIGNAL(toStop()), &server_, SLOT(stop())); assert(c);
		c = connect(this, SIGNAL(toStop(QGrpcCliAbstract::AbstractService* const)), &server_, SLOT(stop(QGrpcCliAbstract::AbstractService* const))); assert(c);
	}

	inline void addService(QGrpcCliAbstract::AbstractService* const service, 
							SleepIfActive amsec = SleepIfActive(0u), 
							SleepIfInactive iamsec = SleepIfInactive(3000u)) { server_.addService(service, amsec, iamsec); }
	inline void deleteService(QGrpcCliAbstract::AbstractService* const service) { server_.deleteService(service); }
	
	inline void start()
	{
		if (!serverThread_.isRunning())
			serverThread_.start();
	}

	inline void stop()
	{
		emit toStop();
	}

	inline void stop(QGrpcCliAbstract::AbstractService* const service) { emit toStop(service); }
	
	~QGrpcCliServer()
	{
		emit toStop();
		serverThread_.quit();
		serverThread_.wait();
	}
	inline QThread* grpcThread() const { return const_cast<QThread*>(&(this->serverThread_)); }
signals:
	void toStop();
	void toStop(QGrpcCliAbstract::AbstractService* const);
};