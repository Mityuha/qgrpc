#ifndef QGRPC_TEST_CLIENT
#define QGRPC_TEST_CLIENT

#include "pingpong.qgrpc.client.h"


//***********************************************************
//***********************************************************
//***********************************************************
//Client side
//***********************************************************
//***********************************************************
//***********************************************************
/**/

#include "ui_qgrpc-test-client.h"
#include <QScrollBar>

class mainWindow : public QMainWindow, public Ui_mainWindow
{
	Q_OBJECT
    QpingClientService pingPongSrv;
	//QPingPongService pingPongSrv;
	QGrpcServiceMonitor monitor;
	bool stop_1M_stream;
	bool stop_MM_stream;
public:
	mainWindow() : Ui_mainWindow(), stop_1M_stream(false), stop_MM_stream(false)
	{
		setupUi(this);
		bool c = false;
		c = connect(pb_sayhello, SIGNAL(clicked()), this, SLOT(onPbSayHello())); assert(c);
		c = connect(pb_start_read_stream, SIGNAL(clicked()), this, SLOT(onPbStartReadStream())); assert(c);
		c = connect(pb_start_write_stream, SIGNAL(clicked()), this, SLOT(onPbStartWriteStream())); assert(c);
		c = connect(pb_stop_write_stream, SIGNAL(clicked()), this, SLOT(onPbStopWriteStream())); assert(c);
		c = connect(pb_start_read_write_stream, SIGNAL(clicked()), this, SLOT(onPbStartReadWriteStream())); assert(c);
		c = connect(pb_stop_read_write_stream, SIGNAL(clicked()), this, SLOT(onPbStopReadWriteStream())); assert(c);
		c = connect(pb_connect, SIGNAL(clicked()), this, SLOT(onPbConnect())); assert(c);
		c = connect(pb_disconnect, SIGNAL(clicked()), this, SLOT(onPbDisconnect())); assert(c);
		c = connect(pb_reconnect, SIGNAL(clicked()), this, SLOT(onPbReconnect())); assert(c);
		//
		c = connect(&pingPongSrv, SIGNAL(SayHelloResponse(QpingClientService::SayHelloCallData*)), this, SLOT(onSayHelloResponse(QpingClientService::SayHelloCallData*))); assert(c);
		c = connect(&pingPongSrv, SIGNAL(GladToSeeMeResponse(QpingClientService::GladToSeeMeCallData*)), this, SLOT(onGladToSeeMeResponse(QpingClientService::GladToSeeMeCallData*))); assert(c);
		c = connect(&pingPongSrv, SIGNAL(GladToSeeYouResponse(QpingClientService::GladToSeeYouCallData*)), this, SLOT(onGladToSeeYouResponse(QpingClientService::GladToSeeYouCallData*))); assert(c);
		c = connect(&pingPongSrv, SIGNAL(BothGladToSeeResponse(QpingClientService::BothGladToSeeCallData*)), this, SLOT(onBothGladToSeeResponse(QpingClientService::BothGladToSeeCallData*))); assert(c);
		c = connect(&pingPongSrv, SIGNAL(channelStateChanged(int, int)), this, SLOT(onPingPongStateChanged(int, int))); assert(c);
		monitor.addService(&pingPongSrv);
		monitor.startMonitor();
		//
		pb_stop_write_stream->setDisabled(true);
		pb_stop_read_write_stream->setDisabled(true);
		pb_disconnect->setDisabled(true);
		pb_reconnect->setDisabled(true);
		this->show(); 
	}

	void setText(const QString& text)
	{
		textEdit->insertPlainText(text);
		QScrollBar *sb = textEdit->verticalScrollBar();
		sb->setValue(sb->maximum());
	}

	std::string randomStr(size_t len = 20)
	{
		static const char alphanum[] =
				"0123456789"
				"!@#$%^&*"
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
				"abcdefghijklmnopqrstuvwxyz";
		assert(sizeof(alphanum));
		size_t alphabetLen = sizeof(alphanum) - 1;
		std::string str;
		for (size_t i = 0; i < len; ++i)
			str += alphanum[rand() % alphabetLen];
		return str;
	}

	public slots:
	void onPbSayHello()
	{
		PingRequest request;
		request.set_name("user");
		request.set_message("user");
		pingPongSrv.SayHello(request);
	}
	void onPbStartReadStream()
	{
		PingRequest request;
		request.set_name("user");
		pingPongSrv.GladToSeeMe(request);
	}
	void onPbStartWriteStream()
	{
		stop_1M_stream = false;
		pingPongSrv.GladToSeeYou();
		pb_stop_write_stream->setDisabled(false);
		pb_start_write_stream->setDisabled(true);
	}
	void onPbStopWriteStream() 
	{ 
		stop_1M_stream = true; 
		pb_stop_write_stream->setDisabled(true);
		pb_start_write_stream->setDisabled(false);
	}
	void onPbStartReadWriteStream()
	{
		stop_MM_stream = false;
		pingPongSrv.BothGladToSee();
		pb_stop_read_write_stream->setDisabled(false);
		pb_start_read_write_stream->setDisabled(true);
	}
	void onPbStopReadWriteStream() 
	{
		stop_MM_stream = true; 
		pb_stop_read_write_stream->setDisabled(true);
		pb_start_read_write_stream->setDisabled(false);
	}
	void onPbConnect() 
	{
		QString ip = le_ipaddr->text();
		if (ip.isEmpty()) return;
		int port = sb_port->value();
		pingPongSrv.grpc_connect(QString("%1:%2").arg(ip).arg(port).toStdString()); 
		pb_connect->setDisabled(true);
		pb_disconnect->setDisabled(false);
		pb_reconnect->setDisabled(false);
		le_ipaddr->setDisabled(true);
		sb_port->setDisabled(true);
	}
	void onPbDisconnect() 
	{ 
		pingPongSrv.grpc_disconnect(); 
		pb_connect->setDisabled(false);
		pb_disconnect->setDisabled(true);
		pb_reconnect->setDisabled(true);
		le_ipaddr->setDisabled(false);
		sb_port->setDisabled(false);
	}
	void onPbReconnect() { pingPongSrv.grpc_reconnect(); }

	void onSayHelloResponse(QpingClientService::SayHelloCallData* response)
	{
		setText("[11]: reply: " + QString::fromStdString(response->reply.message()) + "\n");
		if (response->CouldBeDeleted())
			delete response;
		int a = 0;
		++a;
	}

	void onGladToSeeMeResponse(QpingClientService::GladToSeeMeCallData* response)
	{
		if (response->StreamFinished())
		{
			response->Finish();
			return;
		}
		if (response->CouldBeDeleted())
		{
			delete response;
			return;
		}
		response->Read();
		if (!response->reply.message().empty())
			setText("[1M]: reply: " + QString::fromStdString(response->reply.message()) + "\n");
		int a = 0;
		++a;
	}
	void onGladToSeeYouResponse(QpingClientService::GladToSeeYouCallData* response)
	{
		if (response->CouldBeDeleted())
		{
			if(!response->reply.message().empty())
				setText("[M1]: reply: " + QString::fromStdString(response->reply.message()) + "\n");
			delete response;
			//onPbStopWriteStream();
			return;
		}
		if (response->WriteMode())
		{
			if (stop_1M_stream)
			{
				setText("[M1]: change to READ mode\n");
				response->ChangeMode();
				response->WritesDone();
				return;
			}
			auto str = randomStr();
			response->request.set_name(str);
			response->Write();
			setText("[M1]: request: " + QString::fromStdString(str) + "\n");
			return;
		}
		else
		{
			response->Finish();
		}
	}


	void onBothGladToSeeResponse(QpingClientService::BothGladToSeeCallData* response)
	{
		if (response->CouldBeDeleted())
		{
			delete response;
			//onPbStopReadWriteStream();
			return;
		}
		if (response->WriteMode())
		{
			if (stop_MM_stream)
			{
				setText("[MM]: change to READ mode\n");
				response->ChangeMode();
				response->WritesDone();
				return;
			}
			auto str = randomStr();
			response->request.set_name(str);
			response->Write();
			setText("[MM]: request: " + QString::fromStdString(str) + "\n");
			return;
		}
		else //Read Mode
		{
			if (response->StreamFinished())
			{
				response->Finish();
				return;
			}
			response->Read();
			if (!response->reply.message().empty())
				setText("[MM]: reply: " + QString::fromStdString(response->reply.message()) + "\n");
		}
	}


	void onPingPongStateChanged(int prevcode, int curcode)
	{
		QPixmap px(32, 32);
		grpc_connectivity_state prev = static_cast<grpc_connectivity_state>(prevcode);
		grpc_connectivity_state cur = static_cast<grpc_connectivity_state>(curcode);
		setText(QString("connectivity state changed from %1 to %2\n").arg(prev).arg(cur));
		QString tooltip;
		switch (cur)
		{
		case grpc_connectivity_state::GRPC_CHANNEL_IDLE:
			px.fill(Qt::darkYellow);
			tooltip = "idle";
			break;
		case grpc_connectivity_state::GRPC_CHANNEL_CONNECTING:
			tooltip = "connecting";
			px.fill(Qt::yellow);
			break;
		case grpc_connectivity_state::GRPC_CHANNEL_READY:
			tooltip = "ready";
			px.fill(Qt::darkGreen);
			break;
		case grpc_connectivity_state::GRPC_CHANNEL_SHUTDOWN:
			tooltip = "shutdown";
			px.fill(Qt::darkRed);
			break;
		case grpc_connectivity_state::GRPC_CHANNEL_TRANSIENT_FAILURE:
			tooltip = "transient failure";
			px.fill(Qt::darkRed);
			break;
		default:
			px.fill(Qt::darkRed);
			break;
		}
		pb_indicator->setIcon(px);
		pb_indicator->setToolTip(tooltip);
	}

};


#endif
