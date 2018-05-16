#include <QApplication>
#include "qgrpc-test-server.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	mainWindow mw;
	return app.exec();
}
