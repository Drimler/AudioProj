#pragma once

#ifndef AUDIOCFG_H
#define AUDIOCFG_H

#include <QMainWindow>
#include <QMediaRecorder>
#include <QAudioRecorder>
#include <QAudioProbe>
#include <QUrl>
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QMutex>
#include <QTimer>

#include "MyAudioBase.H"
#include "network.H"


class MyGUI : public QMainWindow
{
    Q_OBJECT

public:
    explicit MyGUI(QWidget *parent = 0);
    virtual ~MyGUI();

protected:
    Ui::AudioCfg *ui;

};

class AudioInfo : public QIODevice
{
    Q_OBJECT

public:
    AudioInfo(const QAudioFormat &format, QObject *parent);
    ~AudioInfo();

    void start();
    void stop();

    QIODevice* qIOdevice;

    qreal level() const { return m_level; }

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    const QAudioFormat m_format;
    quint32 m_maxAmplitude;
    qreal m_level; // 0.0 <= m_level <= 1.0

signals:
    void update();
};

class MyAudioCfg : public MyGUI
{
    Q_OBJECT

public:
    explicit MyAudioCfg(QWidget *parent = 0);
    virtual ~MyAudioCfg();

    QAudioDeviceInfo deviceInfo, deviceIN, deviceOUT;
    QAudioFormat settings;
    QAudioFormat setINput;
    QAudioFormat setOUTput;
    QAudio::Mode modeCfg, modeIN;

private:
//    StreamVisual *m_canvas;
    AudioInfo *m_audioInfoIN, *m_audioInfoOUT;
    QAudioInput *m_audioInput;
    QAudioOutput *m_audioOutput;
    QIODevice *m_input, *m_audioDeviceIN, *m_audioDeviceOUT;
    bool m_pullMode;
    QByteArray m_buffer, myOutBuffer, myInBuffer;

    QTimer *refreshDisplayTimer;
    qreal m_level;

private:
    Receiver *clientRcv;
    Sender *clientSend;
    EthAudio *clientEthAudio;
    QThread *threadEthRcv;
    QThread *threadEthSend;
    QThread * threadEthAudio;
    QMutex myINmutex, myOUTmutex;

private slots: //Audio Cfg GUI Slots

    void on_findButton_clicked();
// audio device info
    void modeChanged(int idx);
    void deviceChanged(int idx);
    void sampleRateChanged(int idx);
    void channelChanged(int idx);
    void codecChanged(int idx);
    void sampleSizeChanged(int idx);
    void sampleTypeChanged(int idx);
    void endianChanged(int idx);
    void test();
    void populateTable();
    void on_MyAudioSetButton_clicked();

// Audio Stream Cfg Gui Slots

    void on_startCapture_clicked();
    void on_checkBox_clicked();
    void on_checkIN_clicked();
    void on_checkOUT_clicked();

//    void processBuffer(const QAudioBuffer& buffer);
    void refreshDisplayIN();
    void refreshDisplayOUT();

    void toggleSuspend();
    void handleStateChanged(QAudio::State state);
    void deviceINChanged(int index);
    void deviceOUTChanged(int index);
    void sliderInChanged(int value);
    void sliderOutChanged(int value);

// Audio Stream handle Slots

    void notified();
    void readMoreIN();
    void readMoreOUT();

    void writeToOUTfromStream(qint64 len);
    void readFromINtoStream(qint64 len);
//    void toggleMode();
    void streamINstate(bool state);
    void streamOUTstate(bool state);
    void writeEthBuffState(bool state);
    void rcvEthData(QByteArray dataBuf);

    void on_startSendEthButton_clicked();
    void on_stopSendEthButton_clicked();
    void on_setEthButton_clicked();

private:
    void loadTextFile();
    void setDefIOMediaFormat( QAudioFormat *def_format );

    void initAudioGUI();
    void initAudioIO();
//    bool createAudioInput(QAudioFormat m_format, QAudioDeviceInfo devInfo);
    void createAudioINput();
    void createAudioOUTput();

    bool isAudioINcreate, isAudioOUTcreate;

    bool isINtoOUTconnect;
    bool isWriteToStream;
    bool isPlayFromStream;
    bool isOutStreamOn;
    bool isInStreamOn;
    bool isCanWriteToEthBuff;

    QAudioRecorder *audioRecorder;
    QAudioProbe *probe;
    bool outputLocationSet;

private:
    void setEthGUI();

signals:
    void canWriteToOUT(qint64);
    void canReadFromIN(qint64);
    void sendToStream (QByteArray);

};


#endif // AUDIOCFG_H
