#include <QCoreApplication>
#include <tcp.h>

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	MyTcp t;
	t.init();

	return a.exec();
}
