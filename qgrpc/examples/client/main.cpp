
#include <stdint.h>
#include <QApplication>
#include "qgrpc-test-client.h"

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	mainWindow mw;
	return app.exec();
}
