#include "tcp.h"

MyTcp::MyTcp() :
	QObject()
{

}

void MyTcp::init()
{
	manager = new QNetworkAccessManager(this);

	socket = new QTcpSocket(this);

	connect(socket, SIGNAL(connected()),this, SLOT(connected()));
	connect(socket, SIGNAL(disconnected()),this, SLOT(disconnected()));
	connect(socket, SIGNAL(bytesWritten(qint64)),this, SLOT(bytesWritten(qint64)));
	connect(socket, SIGNAL(readyRead()),this, SLOT(readyRead()));

	connect(&watchdog, SIGNAL(timeout()),this, SLOT(watchdogTimeout()));


	socket->connectToHost("192.168.1.136", 23);

	if(!socket->waitForConnected(15000))
	{
		qDebug() << "Error: " << socket->errorString();
		exit(-1);
	}

	watchdog.start(10000);
}

void MyTcp::connected()
{
	qDebug() << "connected...";
}

void MyTcp::disconnected()
{
	qDebug() << "disconnected...";
	exit(-1);
}

void MyTcp::bytesWritten(qint64 bytes)
{
	qDebug() << bytes << " bytes written...";
}

void MyTcp::readyRead()
{
	static QByteArray line;
	qDebug() << "reading...";

	// read the data from the socket
	QByteArray data= socket->readAll();
	qDebug() << data;
	for(int l=0;l<data.length();l++)
	{
		char c=data.at(l);
		if(c=='\r')
		{
			decode(line);
			line="";
		}
		else
		{
			if(c!='\n')
				line.append(c);
		}
	}
}

void MyTcp::watchdogTimeout()
{
	exit(-1);
}

void MyTcp::decode(QByteArray line)
{
	watchdog.start(10000);
	QNetworkRequest request;
	char cheksum=0;
	for(int l=0;l<line.length()-2;l++)
	{
		cheksum+=line.at(l);
		//qDebug()<<(int)line.at(l)<<line.at(l);
	}
	cheksum=(cheksum&0x3F)+0x20;
	if(cheksum!=line.right(1)[0])
	{
		qDebug()<<"cheksum error"<<(int)cheksum<<(int)line.right(1)[0]<<line.right(1)[0];
		return;
	}
	qDebug()<<"cheksum OK";
	QList<QByteArray> list=line.split(' ');
	if(list.length()<3)
		return;
	request.setUrl(QUrl("http://192.168.1.100:8080/rest/items/"+list.at(0)));
	request.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain; charset=utf-8");

	QNetworkReply *reply = manager->post(request,list.at(1));

	qDebug() << "send "+list.at(1)+" to "+list.at(0);
	/*connect(reply, SIGNAL(finished()), this, SLOT(close()));
	connect(reply, SIGNAL(errorOccurred()),
			this, SLOT(close()));
	connect(reply, SIGNAL(sslErrors()),
			this, SLOT(close()));*/
}
