#ifndef TCP_H
#define TCP_H
#include <QObject>
#include <QTimer>

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

class MyTcp : public QObject
{
	Q_OBJECT

	QTcpSocket* socket;
	QNetworkAccessManager *manager;
public:
	MyTcp();
	void init();
private:
	void decode(QByteArray);
	QTimer watchdog;
private slots:
	void connected();
	void disconnected();
	void bytesWritten(qint64 bytes);
	void readyRead();
	void watchdogTimeout();
};

#endif // TCP_H
