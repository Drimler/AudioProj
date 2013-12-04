#ifndef NETWORK_H
#define NETWORK_H

#include "MyAudioBase.H"
#include <QDialog>
#include <QObject>
#include <QHostAddress>

class Receiver : protected QObject
{
    Q_OBJECT

public:
    Receiver(QObject *parent = 0);
    ~Receiver();

    quint16 dstPort, srcPort;
    QString dstIPaddr;

    void start();
    bool isStarted() { return rcvStarted;}

private slots:
    void processPendingDatagrams();

public slots:
    void startReceiver( bool state );

private:
//    QLabel *statusLabel;
//    QPushButton *quitButton;
    QUdpSocket *udpSocket;
    QHostAddress groupAddress;
    bool rcvStarted;
signals:
    void RcvEthData( QByteArray );
    void connected(bool);
    void quit();
};

class Sender : protected QObject
{
    Q_OBJECT

public:
    Sender(QObject *parent = 0);
    ~Sender();

    quint16 remotePort, localPort;
    QString remoteIPaddr;

    bool isStarted() { return sendStarted; }

private slots:
    void ttlChanged(int newTtl);
    void startSending();
    void stopSending();
    void sendDatagram();

public slots:
    void startSender();
    void stopSender();
    void restartSender();
    void sendData( QByteArray data );

signals:
//    void sendStarted(bool);
    void canWriteBuff(bool);
    void connected(bool);

private:
    QUdpSocket *udpSocket;
    QTimer *timer;
    QHostAddress groupAddress;
    QByteArray *sendDataBuf;
    int messageNo;
    bool sendStarted;
};

class EthAudio : protected QObject
{
    Q_OBJECT

public:
    EthAudio(QObject *parent = 0);
    ~EthAudio();

    void start();
    bool isStarted() { return ethStarted; }
    bool isRcvStarted() { return rcverStarted; }
    bool isSenderStarted() { return senderStarted; }

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

public slots:
    void startEthAudio( bool state );
    void setEthCfg( quint16 srcPort, quint16 dstPort, QString dstAddr );
    void writeToBuf( QByteArray data );
    void startRcver( bool state );
    void startSender( bool state );

signals:
    void RcvEthAudioData( QByteArray );
    void connectedRcv( bool );
    void connectedSend( bool );
    void errorMsg( QString );
};

#endif // NETWORK_H
