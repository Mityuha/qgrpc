#pragma once

#include <QThread>
#include <QTimer>
#include <set>
#include "QGrpcBase.h"

class MonitorPrivate : public QObject
{
	Q_OBJECT
		QTimer* cq_timer;
	std::set<QGrpcBase::AbstractService*> services;
public:
	MonitorPrivate() : cq_timer(nullptr) {}
	inline void addService(QGrpcBase::AbstractService* const service) { services.insert(service); }
	inline void deleteService(QGrpcBase::AbstractService* const service) { services.erase(service); }
	virtual ~MonitorPrivate() {}
	public slots :
		void start()
	{
		if (!cq_timer)
		{
			cq_timer = new QTimer(this);
			//cq_timer->setInterval(1000);
			bool ok = connect(cq_timer, SIGNAL(timeout()), this, SLOT(AsyncMonitorRpc())); assert(ok);
		}
		cq_timer->start();
	}
	void stop()
	{
		if (cq_timer) cq_timer->stop();
	}


	void AsyncMonitorRpc()
	{
		for (auto* const service : services)
			service->checkCQ();
	}

};

class QGrpcServiceMonitor : public QObject
{
	Q_OBJECT
	QThread monitorThread_;
	MonitorPrivate monitor_;
public:
	QGrpcServiceMonitor()
	{
		monitor_.moveToThread(&monitorThread_);
		bool c = connect(&monitorThread_, SIGNAL(started()), &monitor_, SLOT(start())); assert(c);
		c = connect(&monitorThread_, SIGNAL(finished()), &monitor_, SLOT(stop())); assert(c);
		//c = connect(&monitorThread, SIGNAL(finished()), &monitor, SLOT(deleteLater())); assert(c);
		c = connect(this, SIGNAL(toStopMonitor()), &monitor_, SLOT(stop())); assert(c);
	}
	inline void addService(QGrpcBase::AbstractService* const service) { monitor_.addService(service); }
	inline void deleteService(QGrpcBase::AbstractService* const service) { monitor_.deleteService(service); }
	inline void startMonitor()
	{
		if (!monitorThread_.isRunning())
			monitorThread_.start();
	}
	inline void stopMonitor()
	{
		emit toStopMonitor();
	}
	~QGrpcServiceMonitor()
	{
		monitorThread_.quit();
		monitorThread_.wait();
	}
	inline QThread* monitorThread() const { return const_cast<QThread*>(&(this->monitorThread_)); }
signals:
	void toStopMonitor();
};