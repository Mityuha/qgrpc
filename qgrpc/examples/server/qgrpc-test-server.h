#ifndef QGRPC_TEST_SERVER_H
#define QGRPC_TEST_SERVER_H

#include "pingpong.qgrpc.server.h"

#include <unordered_map>

#include "ui_qgrpc-test-server.h"
#include <QScrollBar>

class mainWindow: public QMainWindow, public Ui_MainWindow
{
    Q_OBJECT;
	QpingServerService pingservice;
	QGrpcSrvServer server;
	
public:
	mainWindow():Ui_MainWindow()
	{
		setupUi(this);
		server.addService(&pingservice);

        bool is_ok;
		is_ok = connect(pb_start, SIGNAL(clicked()), this, SLOT(onStart1())); assert(is_ok);
        is_ok = connect(&pingservice, SIGNAL(SayHelloRequest(QpingServerService::SayHelloCallData*)), this, SLOT(onSayHello(QpingServerService::SayHelloCallData*))); assert(is_ok);
		is_ok = connect(&pingservice, SIGNAL(GladToSeeMeRequest(QpingServerService::GladToSeeMeCallData*)), this, SLOT(onGladToSeeMe(QpingServerService::GladToSeeMeCallData*))); assert(is_ok);
		is_ok = connect(&pingservice, SIGNAL(GladToSeeYouRequest(QpingServerService::GladToSeeYouCallData*)), this, SLOT(onGladToSeeYou(QpingServerService::GladToSeeYouCallData*))); assert(is_ok);
		is_ok = connect(&pingservice, SIGNAL(BothGladToSeeRequest(QpingServerService::BothGladToSeeCallData*)), this, SLOT(onBothGladToSee(QpingServerService::BothGladToSeeCallData*))); assert(is_ok);
		this->show();
    }
    ~mainWindow()
	{
	}

	void setText(QPlainTextEdit* te, const QString& text)
	{
		te->insertPlainText(text + "\n");
		QScrollBar *sb = te->verticalScrollBar();
		sb->setValue(sb->maximum());
	}

private slots:

	void onStart1()
	{
		auto ipaddr = le_ipaddr->text();
		auto port = sb_port->value();
		auto addr_port = QString("%1:%2").arg(ipaddr).arg(port);
		setText(plainTextEdit, QString("Server started on %1").arg(addr_port));
		pingservice.AddListeningPort(QString("%1:%2").arg(ipaddr).arg(port).toStdString());
		server.start();
		pb_start->setDisabled(true);
		//pb_stop->setDisabled(false);
	}

	void onStop1()
	{
	}

    void onSayHello(QpingServerService::SayHelloCallData* cd)
    {
		std::string prefix("Hello ");
		setText(plainTextEdit, QString("[%1][11]: request: %2").arg(QString::fromStdString(cd->peer())).arg(QString::fromStdString(cd->request.name())));
		cd->reply.set_message(prefix + cd->request.name());
		cd->Finish();
    }

	void onGladToSeeMe(QpingServerService::GladToSeeMeCallData* cd)
	{
		static std::unordered_map<void*, size_t> mc_many;

		void* tag = static_cast<void*>(cd);

		std::string prefix("Hello ");
		std::vector<std::string> greeting = 
		{ 
			std::string(prefix + cd->request.name() + "!"),
			"I'm very glad to see you!", 
			"Haven't seen you for thousand years.", 
			"I'm server now. Call me later." 
		};

		auto it = mc_many.find(tag);
		if (it == mc_many.cend())
		{
			mc_many.insert(std::make_pair(tag, 0));
			it = mc_many.find(tag);
			assert(it != mc_many.cend());
		}

		if (it->second < greeting.size())
		{
			if(it->second == 0)
				setText(plainTextEdit, QString("[%1][1M]: request: %2").arg(QString::fromStdString(cd->peer())).arg(QString::fromStdString(cd->request.name())));
			cd->reply.set_message(greeting.at(it->second++));
			cd->Write();
		}
		else
		{
			cd->Finish();
			mc_many.erase(cd);
		}
	}

	void onGladToSeeYou(QpingServerService::GladToSeeYouCallData* cd)
	{
		if (cd->StreamFinished())
		{
			setText(plainTextEdit, QString("[%1][M1]: sending reply and finishing...").arg(QString::fromStdString(cd->peer())));
			cd->reply.set_message("Hello, Client!");
			cd->Finish();
			return;
		}
		cd->Read();
		if(!cd->request.name().empty())
			setText(plainTextEdit, QString("[%1][M1]: request: %2").arg(QString::fromStdString(cd->peer())).arg(QString::fromStdString(cd->request.name())));
	}

	void onBothGladToSee(QpingServerService::BothGladToSeeCallData* cd)
	{
		void* tag = static_cast<void*>(cd);
		if (!cd->WriteMode())//reading mode
		{
			cd->Read();
			if (!cd->request.name().empty())
				setText(plainTextEdit, QString("[%1][MM]: request: %2").arg(QString::fromStdString(cd->peer())).arg(QString::fromStdString(cd->request.name())));
		}
		else
		{
			static std::unordered_map<void*, size_t> mc_many;
			std::string prefix("Hello ");
			std::vector<std::string> greeting =
			{
				std::string(prefix + cd->request.name() + "!"),
				"I'm very glad to see you!",
				"Haven't seen you for thousand years.",
				"I'm server now. Call me later."
			};

			auto it = mc_many.find(tag);
			if (it == mc_many.cend())
			{
				mc_many.insert(std::make_pair(tag, 0));
				it = mc_many.find(tag);
				assert(it != mc_many.cend());
			}

			if (it->second >= greeting.size() || cd->TimeToFinish())
			{
				cd->Finish();
				mc_many.erase(cd);
			}
			else
			{
				cd->reply.set_message(greeting.at(it->second++));
				cd->Write();
			}
		}
	}

} ;


#endif
