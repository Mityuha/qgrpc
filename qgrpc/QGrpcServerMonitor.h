#pragma once

#include <QThread>
#include <QTimer>
#include <set>
#include "QAbstractGrpc.h"

class QSrvMonitorPrivate : public QObject
{
	Q_OBJECT
	QTimer* cq_timer_ = nullptr;
	QGrpcSrvAbstract::AbstractService& service_;
	std::atomic<bool> stopped_;
public:
	QSrvMonitorPrivate(QGrpcSrvAbstract::AbstractService& service)
		:service_(service),
		stopped_(false)
	{}

	virtual ~QSrvMonitorPrivate(){}
public slots :
	void start()
	{
		if (!cq_timer_)
		{
			cq_timer_ = new QTimer(this);
			cq_timer_->setSingleShot(true);
			bool ok = connect(cq_timer_, SIGNAL(timeout()), this, SLOT(AsyncMonitorRpc())); assert(ok);
		}
		cq_timer_->start();
		stopped_.store(false);
	}
	void stop()
	{
		stopped_.store(true);
		service_.PrepareForShutdown();
	}

	
private slots:
	void AsyncMonitorRpc()
	{
		service_.CheckCQ();
		if (stopped_.load())
			return;
		cq_timer_->start();
	}
};

class QGrpcSrvMonitor : public QObject
{
	Q_OBJECT
	QThread serverThread_;
	QSrvMonitorPrivate server_;
public:
	QGrpcSrvMonitor(QGrpcSrvAbstract::AbstractService& service)
		:server_(service)
	{
		server_.moveToThread(&serverThread_);
		bool c = connect(&serverThread_, SIGNAL(started()), &server_, SLOT(start())); assert(c);
	}

	inline void start()
	{
		if (!serverThread_.isRunning())
			serverThread_.start();
	}

	~QGrpcSrvMonitor()
	{
		server_.stop();
		serverThread_.quit();
		serverThread_.wait();
	}
	inline QThread* grpcThread() const { return const_cast<QThread*>(&(this->serverThread_)); }
};