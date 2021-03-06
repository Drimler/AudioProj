/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>
#include <QtNetwork>

#include "network.H"

Receiver::Receiver(QObject *parent)
    : QObject(parent)//QDialog(parent)
{
//    groupAddress = QHostAddress("192.168.7.35");

//    udpSocket = new QUdpSocket(this);
//    udpSocket->bind(QHostAddress::AnyIPv4, 44000, QUdpSocket::ShareAddress);
//    udpSocket->joinMulticastGroup(groupAddress);

//    connect(udpSocket, SIGNAL(readyRead()),
//            this, SLOT(processPendingDatagrams()));
    rcvStarted = false;
//    emit connected( rcvStarted );
}

Receiver::~Receiver() {
    udpSocket->disconnectFromHost();
    delete udpSocket;
}

void Receiver::startReceiver( bool state ) {
    if ( state == true )
        start();
    else {
        emit connected( false );
        udpSocket->disconnectFromHost();
        emit quit();
    }
}

void Receiver::start() {

//    if ( rcvStarted == true ) {
//        udpSocket->disconnectFromHost();
//        delete udpSocket;
//    }
//    groupAddress = QHostAddress("192.168.7.35");

//    udpSocket = new QUdpSocket(this);
//    udpSocket->bind(QHostAddress::AnyIPv4, 44002, QUdpSocket::ShareAddress);
//    udpSocket->joinMulticastGroup(groupAddress);

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));

    rcvStarted = true;
    emit connected( rcvStarted );
}

void Receiver::processPendingDatagrams()
{
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size());

        emit RcvEthData( datagram );
    }
}


Sender::Sender(QObject *parent)
    : QObject(parent)
    , sendStarted(false)
{
    groupAddress = QHostAddress("192.168.7.35");
    messageNo = 1;
    localPort = 44000;
    remotePort = 44002;
}

Sender::~Sender() {

    qWarning() << "Delete Sender";
    stopSending();
    udpSocket->disconnectFromHost();
//    delete udpSocket;
    delete sendDataBuf;
    delete timer;
}

void Sender::startSender( bool state ) {
    stopSender();

    if ( state == true ) {
        timer = new QTimer(this);
        sendDataBuf = new QByteArray;
        messageNo = 1;

        connect(timer, SIGNAL(timeout()), this, SLOT(sendDatagram()));

        startSending();

        sendStarted = true;
    }
    emit connected( sendStarted );
}
void Sender::stopSender() {
    if ( sendStarted == true ) {
        sendStarted = false;
        stopSending();
        delete sendDataBuf;
        delete timer;
        sendDataBuf = NULL;
        timer = NULL;
    }
}
void Sender::restartSender() {
//    stopSender();
    startSender( true );
}

void Sender::ttlChanged(int newTtl)
{
    udpSocket->setSocketOption(QAbstractSocket::MulticastTtlOption, newTtl);
}

void Sender::startSending()
{
//    startButton->setEnabled(false);
    timer->start(1000);
//    timer->stop();
//    emit sendStarted(true);
//    sendStarted = true;
}

void Sender::stopSending()
{
//    startButton->setEnabled(true);

//    timer->start(1000);
    timer->stop();
//    sendStarted = false;
}

void Sender::sendDatagram()
{
//    statusLabel->setText(tr("Now sending datagram %1").arg(messageNo));
    int cnt;
    QByteArray datagram = "Multicast message " + QByteArray::number(messageNo);
//    cnt = udpSocket->writeDatagram(datagram.data(), datagram.size(), groupAddress, remotePort);
//    if ( cnt == -1 ) {
//        qWarning() << "Error with sending UDP..";
//        return;
//    }
    ++messageNo;
    qWarning() << "sendDatagram -> " << datagram;
//    emit canWriteBuff(true);
}

void Sender::sendData( QByteArray data ) {

    int len = data.size();
    int cnt = udpSocket->writeDatagram(data.data(), len, groupAddress, remotePort);
    if ( cnt != len ) {
        qWarning() << "Error with sending UDP..";
    }
    qWarning() << "Data sended, len = " << len;
    qWarning() << "sendDatagram -> " << data;
//    emit canWriteBuff(true);
}


EthAudio::EthAudio(QObject *parent, quint16 portL)
    : QObject(parent)
    , ethStarted(false)
    , senderStarted(false)
    , rcverStarted(false)
    , localPort(portL)
{
    QString ipAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for ( int i = 0; i < ipAddressesList.size(); ++i )
    {
        if ( ipAddressesList.at(i) != QHostAddress::LocalHost &&
             ipAddressesList.at(i).toIPv4Address() )
        {
             ipAddress = ipAddressesList.at(i).toString();
             break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if ( ipAddress.isEmpty() )
        ipAddress = QHostAddress( QHostAddress::LocalHost ).toString();

    localAddr = ipAddress;

    localAddrH = QHostAddress( localAddr );
    qWarning() << "LocalHost addres -> " << localAddr;

    udpSocket = new QUdpSocket(this);
    if (!udpSocket) {
        qWarning() << "Error occured when create udpSocket";
        ethStarted = false;
        ~EthAudio();
        return;
    }
    bool socketState = udpSocket->bind( localAddrH, localPort, QUdpSocket::ShareAddress );
    if ( socketState == true )
        qWarning() << "Error occured when udpSocket binded";
}

EthAudio::~EthAudio() {

}

void EthAudio::setEthCfg( quint16 srcPort, quint16 dstPort, QString dstAddr ) {

    if ( srcPort == 0
         || dstPort == 0
         || QHostAddress(dstAddr).toIPv4Address() == 0 )
    {
        emit errorMsg("Error, invalid params for Eth Configs");
        return;
    }

    startEthAudio(false);

//at here we will need to set timeout that this task can be running for wait sender and rcver statuses
    while ( senderStarted || rcverStarted ) {} //wait when Sender & Rcver would disconnected

    localPort = srcPort;
    remotePort = dstPort;
    remoteAddr = dstAddr;

    udpSocket->disconnectFromHost();
    bool socketState = udpSocket->bind(QHostAddress::AnyIPv4, localPort, QUdpSocket::ShareAddress);
    if (socketState == false) {
        emit errorMsg("Error occurred when udpSocket binding");
        return;
    }

    startEthAudio(true);
}

void EthAudio::startEthAudio( bool state ) {

    if ( rcverStarted == true ) {
//        rcverStarted = false;

        if ( receiver != NULL ) {
            if ( receiver->isStarted() ) {
                receiver->startReceiver(false);
            }
        } else if ( threadEthRcv != NULL ) {
            if ( threadEthRcv->isRunning() ) {
                threadEthRcv->quit();
            }
        }
        while ( rcverStarted == true ) {}

        delete receiver;
        delete threadEthRcv;
        receiver = NULL;
        threadEthRcv = NULL;
    }
    if ( senderStarted == true ) {
//        senderStarted = false;

        if ( sender != NULL ) {
            if ( sender->isStarted() ) {
                sender->startSender(false);
            }
        } else if ( threadEthSend != NULL ) {
            if ( threadEthSend->isRunning() ) {
                threadEthSend->quit();
            }
        }
        while ( senderStarted == true ) {}

        delete sender;
        delete threadEthSend;
        sender = NULL;
        threadEthSend = NULL;
    }

    if ( state == true ) {

        threadEthRcv = new QThread(this);
        threadEthSend = new QThread(this);

        receiver = new Receiver;
        sender = new Sender;

        receiver->moveToThread(threadEthRcv);
        sender->moveToThread(threadEthSend);

//        connect(threadEthRcv,SIGNAL(started()),clientRcv,SLOT(start())); // ����� ����� ��������, �� ������ ���������� ������ ������ ������
        connect(receiver,SIGNAL(finished()),threadEthRcv,SLOT(quit())); // ����� ������ ����� ���������, ��������� �����
        connect(receiver,SIGNAL(finished()),receiver,SLOT(deleteLater())); // ����� ������ ����� ���������, ������� ��� ��������� ������
        connect(threadEthRcv,SIGNAL(finished()),threadEthRcv,SLOT(deleteLater())); // ����� ����� �����������, ������� ���

//        connect(threadEthSend,SIGNAL(started()),sender,SLOT(start())); // ����� ����� ��������, �� ������ ���������� ������ ������ ������
        connect(sender,SIGNAL(finished()),threadEthSend,SLOT(quit())); // ����� ������ ����� ���������, ��������� �����
        connect(sender,SIGNAL(finished()),sender,SLOT(deleteLater())); // ����� ������ ����� ���������, ������� ��� ��������� ������
        connect(threadEthSend,SIGNAL(finished()),threadEthSend,SLOT(deleteLater())); // ����� ����� �����������, ������� ���

        connect( receiver, SIGNAL(RcvEthData(QByteArray)),this, SLOT(RcvEthData(QByteArray)) );
        connect( this, SIGNAL(sendData(QByteArray)), sender, SLOT(sendData(QByteArray)));//,Qt::DirectConnection );

//        connect( receiver, SIGNAL(connected(bool)), this, SLOT(connectedRcv(bool)));
//        connect( sender, SIGNAL(connected(bool)), this, SLOT(connectedSend(bool)));
//        connect( sender, SIGNAL(canWriteBuff(bool)), this, SLOT(canWriteBuff(bool)));

    } else {

    }



}

void EthAudio::writeToBuf( QByteArray data ) {
    if ( senderStarted == true && sender )
        sender->sendData(data);
    else
        emit errorMsg("Error, Sender wasn't started before call to writeToBuff");
}

void EthAudio::startRcver( bool state ) {
    if ( state == true ) {
        if ( rcverStarted == true ) {
            delete receiver;
            receiver = new Receiver(this);
        }
    }
}

void EthAudio::startSender( bool state ) {

}

class ppp : QObject
{
private:
    Receiver *receiver;
    Sender *sender;
    QUdpSocket *udpSocket;
    QHostAddress localAddrH, remoteAddrH;
    bool ethStarted;
    bool senderStarted;
    bool rcverStarted;

    QThread *threadEthRcv;
    QThread *threadEthSend;

public:
    quint16 localPort, remotePort;
    QString localAddr, remoteAddr;

signals:
    void RcvEthAudioData( QByteArray );
    void connectedRcv( bool );
    void connectedSend( bool );
    void errorMsg( QString );

}
