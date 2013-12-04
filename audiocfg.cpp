#include "ui_audiocfg.h"
#include "audiocfg.H"
#include <QFile>
#include <QTextStream>

#include <stdlib.h>
#include <math.h>

#include <QThread>
#include <QLocale>
#include <QDateTime>
#include <QDebug>
#include <qendian.h>
//#include <QMutex>

#include "network.H"

//#define PUSH_MODE_LABEL "Push"
//#define PULL_MODE_LABEL "Pull"
#define SUSPEND_LABEL   "Pause IN"
#define RESUME_LABEL    "Resume IN"

const int BufferSize = 4096;

MyGUI::MyGUI(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::AudioCfg)
{
    ui->setupUi(this);
}
MyGUI::~MyGUI()
{
    delete ui;
}


MyAudioCfg::MyAudioCfg(QWidget *parent) :
    MyGUI(parent)//,    ui(new Ui::AudioCfg)
{

    this->modeCfg = QAudio::AudioOutput;

//    QMetaObject::connectSlotsByName(this);

    isAudioINcreate = false;
    isAudioOUTcreate = false;

    isWriteToStream = false;
    isPlayFromStream = false;

    isOutStreamOn = false;
    isInStreamOn = false;

    m_buffer.resize(BufferSize);
    myInBuffer.resize(BufferSize);
    myOutBuffer.resize(BufferSize);

    setDefIOMediaFormat( &setINput );
    setDefIOMediaFormat( &setOUTput );

    connect( ui->modeBox, SIGNAL(activated(int)), SLOT(modeChanged(int)) );
    connect( ui->deviceBox, SIGNAL(activated(int)), SLOT(deviceChanged(int)) );
    connect( ui->findButton, SIGNAL(clicked()), SLOT(on_findButton_clicked()) );
    connect( ui->populateTableButton, SIGNAL(clicked()), SLOT(populateTable()) );
    connect( ui->testButton, SIGNAL(clicked()), SLOT(test()) );
    connect( ui->sampleRateBox, SIGNAL(activated(int)), SLOT(sampleRateChanged(int)) );
    connect( ui->channelsBox, SIGNAL(activated(int)), SLOT(channelChanged(int)) );
    connect( ui->codecsBox, SIGNAL(activated(int)), SLOT(codecChanged(int)) );
    connect( ui->sampleSizesBox, SIGNAL(activated(int)), SLOT(sampleSizeChanged(int)) );
    connect( ui->sampleTypesBox, SIGNAL(activated(int)), SLOT(sampleTypeChanged(int)) );
    connect( ui->endianBox, SIGNAL(activated(int)), SLOT(endianChanged(int)) );
    connect( ui->MyAudioSetButton, SIGNAL(clicked()), SLOT(on_MyAudioSetButton_clicked()) );

//    connect( ui->rcvEthWndw, SIGNAL());

    loadTextFile();
    ui->modeBox->setCurrentIndex(0);
    modeChanged(0);

    ui->deviceBox->setCurrentIndex(0);
    deviceChanged(0);

    initAudioGUI();
    initAudioIO();

    setEthGUI();
}
MyAudioCfg::~MyAudioCfg()
{
    refreshDisplayTimer->stop();
    delete refreshDisplayTimer;

    if ( isAudioINcreate == true ) {
        m_audioInput->stop();
        m_audioInput->disconnect(this);
        delete m_audioInput;
    }
    if ( isAudioOUTcreate == true ){
        m_audioOutput->stop();
        m_audioOutput->disconnect(this);
        delete m_audioOutput;
    }
}


void MyAudioCfg::initAudioGUI()
{
//    m_canvas = new StreamVisual(this);

    QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
    for(int i = 0; i < devices.size(); ++i)
        ui->AudioDevCBox_2->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));

    devices = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for(int i = 0; i < devices.size(); ++i)
        ui->AudioDevCBox_3->addItem(devices.at(i).deviceName(), qVariantFromValue(devices.at(i)));

    connect(ui->AudioDevCBox_2, SIGNAL(activated(int)), SLOT(deviceINChanged(int)));
    connect(ui->AudioDevCBox_3, SIGNAL(activated(int)), SLOT(deviceOUTChanged(int)));

    ui->AudioLevelSliderIN->setRange(0, 100);
    ui->AudioLevelSliderOUT->setRange(0, 100);

    connect(ui->AudioLevelSliderIN, SIGNAL(valueChanged(int)), SLOT(sliderInChanged(int)));
    connect(ui->AudioLevelSliderOUT, SIGNAL(valueChanged(int)), SLOT(sliderOutChanged(int)));

    ui->inpHandleButton->setText(tr(SUSPEND_LABEL));
    connect(ui->inpHandleButton, SIGNAL(clicked()), SLOT(toggleSuspend()));

    connect(ui->startCapture, SIGNAL(clicked()), SLOT(on_startCapture_clicked()));
    connect(ui->check_INOUT, SIGNAL(clicked()), SLOT(on_checkBox_clicked()));

    connect(ui->check_IN_Stream, SIGNAL(clicked()), SLOT(on_checkIN_clicked()));
    connect(ui->check_OUT_Stream, SIGNAL(clicked()), SLOT(on_checkOUT_clicked()));

    ui->AudioDevCBox_2->setCurrentIndex(0);
    deviceINChanged(0);
    ui->AudioDevCBox_3->setCurrentIndex(0);
    deviceOUTChanged(0);
}
void MyAudioCfg::initAudioIO()
{
    m_pullMode = true;

    if (!deviceIN.isFormatSupported(setINput)) {
        qWarning() << "User settings to INput format not supported - trying to use nearest";
        setINput = deviceIN.nearestFormat(setINput);
    }
    if (!deviceOUT.isFormatSupported(setOUTput)) {
        qWarning() << "User settings to OUTput format not supported - trying to use nearest";
        setOUTput = deviceIN.nearestFormat(setOUTput);
    }

//    audioRecorder = new QAudioRecorder(this);
//    probe = new QAudioProbe;
//    connect(probe, SIGNAL(audioBufferProbed(QAudioBuffer)), this, SLOT(processBuffer(QAudioBuffer)));
//    probe->setSource(audioRecorder);

//    m_audioInfoIN  = new AudioInfo(setINput, this);
//    m_audioDeviceIN = new QIODevice(setINput, this);
//    connect(m_audioInfoIN, SIGNAL(update()), SLOT(refreshDisplay()));
//    connect(m_audioInfoIN, SIGNAL(readyRead()), SLOT(readMore()));

//    isAudioINcreate = createAudioInput(setINput, deviceIN);
//    createAudioINput();

    connect(this, SIGNAL(canReadFromIN(qint64)), this, SLOT(readFromINtoStream(qint64)));
    connect(this, SIGNAL(canWriteToOUT(qint64)), this, SLOT(writeToOUTfromStream(qint64)));

//    createAudioOUTput();
    refreshDisplayTimer = new QTimer(this);
    connect(refreshDisplayTimer, SIGNAL(timeout()), this, SLOT(refreshDisplayIN()));
    connect(refreshDisplayTimer, SIGNAL(timeout()), this, SLOT(refreshDisplayOUT()));
}

//bool MyAudioCfg::createAudioInput(QAudioFormat m_format, QAudioDeviceInfo devInfo)
void MyAudioCfg::createAudioINput()
{
    qWarning() << "createAudioINput";
    if ( isAudioINcreate == true ) {
//        m_audioInfoIN->stop();
        m_audioDeviceIN->close();
        m_audioInput->stop();
        m_audioInput->disconnect(this);
        delete m_audioInput;
        isAudioINcreate = false;
    }

    if (!deviceIN.isFormatSupported(setINput)) {
        qWarning() << "User settings to INput format not supported - trying to use default";
        setDefIOMediaFormat( &setINput );
        setINput = deviceIN.nearestFormat(setINput);
    }

    m_audioInput = new QAudioInput(deviceIN, setINput, this);
//    m_audioInput = new QAudioInput(devInfo, m_format, this);

//    connect(m_audioInput, SIGNAL(notify()), SLOT(notified()));
    connect(m_audioInput, SIGNAL(stateChanged(QAudio::State)), SLOT(handleStateChanged(QAudio::State)));

    ui->AudioLevelSliderIN->setValue(m_audioInput->volume() * 100 + 0.5);

    m_audioDeviceIN = m_audioInput->start();
    connect(m_audioDeviceIN, SIGNAL(readyRead()), SLOT(readMoreIN()));
    isAudioINcreate = true;

//    quint32 readLen = m_audioInput->notifyInterval();
//    qWarning() << "Notify interval -> " << readLen;
//    return true;
//    refreshDisplayTimer->start(5);
}

void MyAudioCfg::createAudioOUTput() {
    qWarning() << "createAudioOUTput";
    if ( isAudioOUTcreate == true ) {
//        m_audioInfoIN->stop();
        m_audioDeviceOUT->close();
        m_audioOutput->stop();
        m_audioOutput->disconnect(this);
        delete m_audioOutput;
        isAudioOUTcreate = false;
    }

    m_audioOutput = new QAudioOutput(deviceOUT, setOUTput, this);

    ui->AudioLevelSliderOUT->setValue(m_audioOutput->volume()*100+0.5);
    m_audioDeviceOUT = m_audioOutput->start();

    isAudioOUTcreate = true;
}

void MyAudioCfg::setDefIOMediaFormat( QAudioFormat *def_format ) {
    if ( !def_format ) {
        qWarning() << "Ptr to AudioFormat == NULL";
        return;
    }
    def_format->setSampleRate(8000);
    def_format->setChannelCount(1);
    def_format->setSampleSize(8);
//    def_format->setSampleType(QAudioFormat::SignedInt);
    def_format->setSampleType(QAudioFormat::UnSignedInt);
    def_format->setByteOrder(QAudioFormat::LittleEndian);
    def_format->setCodec("audio/pcm");
}

// Utility functions for converting QAudioFormat fields into text

static QString toString(QAudioFormat::SampleType sampleType)
{
    QString result("Unknown");
    switch (sampleType) {
    case QAudioFormat::SignedInt:
        result = "SignedInt";
        break;
    case QAudioFormat::UnSignedInt:
        result = "UnSignedInt";
        break;
    case QAudioFormat::Float:
        result = "Float";
        break;
    case QAudioFormat::Unknown:
        result = "Unknown";
    }
    return result;
}
static QString toString(QAudioFormat::Endian endian)
{
    QString result("Unknown");
    switch (endian) {
    case QAudioFormat::LittleEndian:
        result = "LittleEndian";
        break;
    case QAudioFormat::BigEndian:
        result = "BigEndian";
        break;
    }
    return result;
}
void MyAudioCfg::on_findButton_clicked()
{
    QString searchString = ui->lineEdit->text();
    ui->textEdit->find(searchString, QTextDocument::FindWholeWords);
}

void MyAudioCfg::loadTextFile()
{
    QFile inputFile("d:\\Qt\\MyProj\\AudioProj\\AudioProj\\sometext.txt");
    if(!inputFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        ui->textEdit->setPlainText("Could not open file");
        return;
    }

    QTextStream in(&inputFile);
    QString line = in.readAll();
    inputFile.close();

    ui->textEdit->setPlainText(line);
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor, 1);
}

void MyAudioCfg::modeChanged(int idx)
{
    ui->testResult->clear();

    // mode has changed
    if (idx == 0)
        modeCfg = QAudio::AudioInput;
    else
        modeCfg = QAudio::AudioOutput;

    ui->deviceBox->clear();
    foreach (const QAudioDeviceInfo &deviceInfo, QAudioDeviceInfo::availableDevices(modeCfg))
        ui->deviceBox->addItem(deviceInfo.deviceName(), qVariantFromValue(deviceInfo));

    ui->deviceBox->setCurrentIndex(0);
    deviceChanged(0);
}

void MyAudioCfg::deviceChanged(int idx)
{
    ui->testResult->clear();

    if (ui->deviceBox->count() == 0)
        return;

    // device has changed
    deviceInfo = ui->deviceBox->itemData(idx).value<QAudioDeviceInfo>();

    ui->sampleRateBox->clear();
    QList<int> sampleRatez = deviceInfo.supportedSampleRates();
    for (int i = 0; i < sampleRatez.size(); ++i)
        ui->sampleRateBox->addItem(QString("%1").arg(sampleRatez.at(i)));
    if (sampleRatez.size())
        settings.setSampleRate(sampleRatez.at(0));

    ui->channelsBox->clear();
    QList<int> chz = deviceInfo.supportedChannelCounts();
    for (int i = 0; i < chz.size(); ++i)
        ui->channelsBox->addItem(QString("%1").arg(chz.at(i)));
    if (chz.size())
        settings.setChannelCount(chz.at(0));

    ui->codecsBox->clear();
    QStringList codecs = deviceInfo.supportedCodecs();
    for (int i = 0; i < codecs.size(); ++i)
        ui->codecsBox->addItem(QString("%1").arg(codecs.at(i)));
    if (codecs.size())
        settings.setCodec(codecs.at(0));
    // Add false to create failed condition!
    ui->codecsBox->addItem("audio/test");

    ui->sampleSizesBox->clear();
    QList<int> sampleSizez = deviceInfo.supportedSampleSizes();
    for (int i = 0; i < sampleSizez.size(); ++i)
        ui->sampleSizesBox->addItem(QString("%1").arg(sampleSizez.at(i)));
    if (sampleSizez.size())
        settings.setSampleSize(sampleSizez.at(0));

    ui->sampleTypesBox->clear();
    QList<QAudioFormat::SampleType> sampleTypez = deviceInfo.supportedSampleTypes();

    for (int i = 0; i < sampleTypez.size(); ++i)
        ui->sampleTypesBox->addItem(toString(sampleTypez.at(i)));
    if (sampleTypez.size())
        settings.setSampleType(sampleTypez.at(0));

    ui->endianBox->clear();
    QList<QAudioFormat::Endian> endianz = deviceInfo.supportedByteOrders();
    for (int i = 0; i < endianz.size(); ++i)
        ui->endianBox->addItem(toString(endianz.at(i)));
    if (endianz.size())
        settings.setByteOrder(endianz.at(0));

    ui->allFormatsTable->clearContents();
}

void MyAudioCfg::populateTable()
{
    int row = 0;

    QAudioFormat format;
    foreach (QString codec, deviceInfo.supportedCodecs()) {
        format.setCodec(codec);
        foreach (int sampleRate, deviceInfo.supportedSampleRates()) {
            format.setSampleRate(sampleRate);
            foreach (int channels, deviceInfo.supportedChannelCounts()) {
                format.setChannelCount(channels);
                foreach (QAudioFormat::SampleType sampleType, deviceInfo.supportedSampleTypes()) {
                    format.setSampleType(sampleType);
                    foreach (int sampleSize, deviceInfo.supportedSampleSizes()) {
                        format.setSampleSize(sampleSize);
                        foreach (QAudioFormat::Endian endian, deviceInfo.supportedByteOrders()) {
                            format.setByteOrder(endian);
                            if (deviceInfo.isFormatSupported(format)) {
                                ui->allFormatsTable->setRowCount(row + 1);

                                QTableWidgetItem *codecItem = new QTableWidgetItem(format.codec());
                                ui->allFormatsTable->setItem(row, 0, codecItem);

                                QTableWidgetItem *sampleRateItem = new QTableWidgetItem(QString("%1").arg(format.sampleRate()));
                                ui->allFormatsTable->setItem(row, 1, sampleRateItem);

                                QTableWidgetItem *channelsItem = new QTableWidgetItem(QString("%1").arg(format.channelCount()));
                                ui->allFormatsTable->setItem(row, 2, channelsItem);

                                QTableWidgetItem *sampleTypeItem = new QTableWidgetItem(toString(format.sampleType()));
                                ui->allFormatsTable->setItem(row, 3, sampleTypeItem);

                                QTableWidgetItem *sampleSizeItem = new QTableWidgetItem(QString("%1").arg(format.sampleSize()));
                                ui->allFormatsTable->setItem(row, 4, sampleSizeItem);

                                QTableWidgetItem *byteOrderItem = new QTableWidgetItem(toString(format.byteOrder()));
                                ui->allFormatsTable->setItem(row, 5, byteOrderItem);

                                ++row;

                            }
                        }
                    }
                }
            }
        }
    }
}

void MyAudioCfg::test()
{
    // tries to set all the settings picked.
    ui->testResult->clear();

    if (!deviceInfo.isNull()) {
        if (deviceInfo.isFormatSupported(settings)) {
            ui->testResult->setText(tr("Success"));
            ui->nearestSampleRate->setText("");
            ui->nearestChannel->setText("");
            ui->nearestCodec->setText("");
            ui->nearestSampleSize->setText("");
            ui->nearestSampleType->setText("");
            ui->nearestEndian->setText("");
        } else {
            QAudioFormat nearest = deviceInfo.nearestFormat(settings);
            ui->testResult->setText(tr("Failed"));
            ui->nearestSampleRate->setText(QString("%1").arg(nearest.sampleRate()));
            ui->nearestChannel->setText(QString("%1").arg(nearest.channelCount()));
            ui->nearestCodec->setText(nearest.codec());
            ui->nearestSampleSize->setText(QString("%1").arg(nearest.sampleSize()));
            ui->nearestSampleType->setText(toString(nearest.sampleType()));
            ui->nearestEndian->setText(toString(nearest.byteOrder()));
        }
    }
    else
        ui->testResult->setText(tr("No Device"));
}

void MyAudioCfg::sampleRateChanged(int idx)
{
    // sample rate has changed
    settings.setSampleRate(ui->sampleRateBox->itemText(idx).toInt());
}

void MyAudioCfg::channelChanged(int idx)
{
    settings.setChannelCount(ui->channelsBox->itemText(idx).toInt());
}
void MyAudioCfg::codecChanged(int idx)
{
    settings.setCodec(ui->codecsBox->itemText(idx));
}
void MyAudioCfg::sampleSizeChanged(int idx)
{
    settings.setSampleSize(ui->sampleSizesBox->itemText(idx).toInt());
}
void MyAudioCfg::sampleTypeChanged(int idx)
{
    switch (ui->sampleTypesBox->itemText(idx).toInt()) {
        case QAudioFormat::SignedInt:
            settings.setSampleType(QAudioFormat::SignedInt);
            break;
        case QAudioFormat::UnSignedInt:
            settings.setSampleType(QAudioFormat::UnSignedInt);
            break;
        case QAudioFormat::Float:
            settings.setSampleType(QAudioFormat::Float);
    }
}
void MyAudioCfg::endianChanged(int idx)
{
    switch (ui->endianBox->itemText(idx).toInt()) {
        case QAudioFormat::LittleEndian:
            settings.setByteOrder(QAudioFormat::LittleEndian);
            break;
        case QAudioFormat::BigEndian:
            settings.setByteOrder(QAudioFormat::BigEndian);
    }
}

void MyAudioCfg::on_MyAudioSetButton_clicked()
{
    // tries to set all the settings picked.
    ui->testResult->clear();

    if (!deviceInfo.isNull()) {
        if (deviceInfo.isFormatSupported(settings)) {

            if ( modeCfg == QAudio::AudioInput ) {
                ui->testResult->setText(tr("Success, Input setting was set"));
                setINput = settings;
            } else if ( modeCfg == QAudio::AudioOutput ) {
                ui->testResult->setText(tr("Success, OUTput setting was set"));
                setOUTput = settings;
            } else
                ui->testResult->setText(tr("Failed, select OUTput or Input mode"));

            ui->nearestSampleRate->setText("");
            ui->nearestChannel->setText("");
            ui->nearestCodec->setText("");
            ui->nearestSampleSize->setText("");
            ui->nearestSampleType->setText("");
            ui->nearestEndian->setText("");
        } else {
            QAudioFormat nearest = deviceInfo.nearestFormat(settings);
            ui->testResult->setText(tr("Failed"));
            ui->nearestSampleRate->setText(QString("%1").arg(nearest.sampleRate()));
            ui->nearestChannel->setText(QString("%1").arg(nearest.channelCount()));
            ui->nearestCodec->setText(nearest.codec());
            ui->nearestSampleSize->setText(QString("%1").arg(nearest.sampleSize()));
            ui->nearestSampleType->setText(toString(nearest.sampleType()));
            ui->nearestEndian->setText(toString(nearest.byteOrder()));
        }
    }
    else
        ui->testResult->setText(tr("No Device"));
}

AudioInfo::AudioInfo(const QAudioFormat &format, QObject *parent)
    : QIODevice(parent)
    , m_format(format)
    , m_maxAmplitude(0)
    , m_level(0.0)
{
    switch (m_format.sampleSize()) {
    case 8:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 255;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 127;
            break;
        default:
            break;
        }
        break;
    case 16:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 65535;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 32767;
            break;
        default:
            break;
        }
        break;

    case 32:
        switch (m_format.sampleType()) {
        case QAudioFormat::UnSignedInt:
            m_maxAmplitude = 0xffffffff;
            break;
        case QAudioFormat::SignedInt:
            m_maxAmplitude = 0x7fffffff;
            break;
        case QAudioFormat::Float:
            m_maxAmplitude = 0x7fffffff; // Kind of
        default:
            break;
        }
        break;

    default:
        break;
    }
}
AudioInfo::~AudioInfo()
{
}
void AudioInfo::start()
{
    open(QIODevice::WriteOnly);
}
void AudioInfo::stop()
{
    close();
}
qint64 AudioInfo::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data)
    Q_UNUSED(maxlen)

    return 0;
}
qint64 AudioInfo::writeData(const char *data, qint64 len)
{
    if (m_maxAmplitude) {
        Q_ASSERT(m_format.sampleSize() % 8 == 0);
        const int channelBytes = m_format.sampleSize() / 8;
        const int sampleBytes = m_format.channelCount() * channelBytes;
        Q_ASSERT(len % sampleBytes == 0);
        const int numSamples = len / sampleBytes;

        quint32 maxValue = 0;
        const unsigned char *ptr = reinterpret_cast<const unsigned char *>(data);

        for (int i = 0; i < numSamples; ++i) {
            for (int j = 0; j < m_format.channelCount(); ++j) {
                quint32 value = 0;

                if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    value = *reinterpret_cast<const quint8*>(ptr);
                } else if (m_format.sampleSize() == 8 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    value = qAbs(*reinterpret_cast<const qint8*>(ptr));
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint16>(ptr);
                    else
                        value = qFromBigEndian<quint16>(ptr);
                } else if (m_format.sampleSize() == 16 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint16>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint16>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::UnSignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qFromLittleEndian<quint32>(ptr);
                    else
                        value = qFromBigEndian<quint32>(ptr);
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::SignedInt) {
                    if (m_format.byteOrder() == QAudioFormat::LittleEndian)
                        value = qAbs(qFromLittleEndian<qint32>(ptr));
                    else
                        value = qAbs(qFromBigEndian<qint32>(ptr));
                } else if (m_format.sampleSize() == 32 && m_format.sampleType() == QAudioFormat::Float) {
                    value = qAbs(*reinterpret_cast<const float*>(ptr) * 0x7fffffff); // assumes 0-1.0
                }

                maxValue = qMax(value, maxValue);
                ptr += channelBytes;
            }
        }

        maxValue = qMin(maxValue, m_maxAmplitude);
        m_level = qreal(maxValue) / m_maxAmplitude;
    }

    emit update();
    return len;
}

void MyAudioCfg::notified()
{
    qWarning() << "bytesReady = " << m_audioInput->bytesReady()
               << ", " << "elapsedUSecs = " <<m_audioInput->elapsedUSecs()
               << ", " << "processedUSecs = "<<m_audioInput->processedUSecs();
}

void MyAudioCfg::readMoreIN()
{
    if ( !m_audioInput || isAudioINcreate == false ) {
        qWarning() << "m_audioInput not created yet";
        return;
    }
    qint64 len = m_audioInput->bytesReady();
//    qWarning() << "bytesReady len -> "<<len;
    if ( len > BufferSize )
        len = BufferSize;

    myInBuffer.resize(len);

    qint64 l = m_audioDeviceIN->read(myInBuffer.data(), len);
    if ( l > 0 ) {
        if ( isINtoOUTconnect == true && isAudioOUTcreate == true )
            m_audioDeviceOUT->write(myInBuffer.constData(), l);
//        qWarning() << "Bytes readyIN Len -> " << l;
        if ( isWriteToStream == true )
            emit canReadFromIN(l); //read audio data from stream (like an Eth connection)
    } else {
//        qWarning() << "Bytes readyIN Len -> " << l << "\nAudioOUTcreate -> " << isAudioOUTcreate;
    }
}
void MyAudioCfg::readMoreOUT()
{
    if ( !m_audioOutput || isAudioOUTcreate == false ) {
        qWarning() << "m_audioOutput not created yet";
        return;
    }
//    qWarning() << "ReadMoreOUT point was getted";

    qint64 len = m_audioOutput->bytesFree();
    if (len > BufferSize)
        len = BufferSize;
    qint64 l = m_audioDeviceIN->read(m_buffer.data(), len);
    if (l > 0) {
//        if ( isINtoOUTconnect == true )
//            m_audioInfoIN->writeData(m_buffer.constData(), l);
        if ( isPlayFromStream == true )
            emit canWriteToOUT(l); //write audio data from stream-Buff to OUT (like an Eth connection)
    }
//    qWarning() << "Bytes readyOUT Len -> " << l;
}

void MyAudioCfg::writeToOUTfromStream(qint64 len) {

    m_audioDeviceOUT->write(myOutBuffer.constData(), len);
}

void MyAudioCfg::readFromINtoStream(qint64 len) {

//    m_audioDeviceOUT->write(myOutBuffer.constData(), len);
//    if (isCanWriteToEthBuff == true ) {
    if (isWriteToStream == true ) {
        emit sendToStream(myInBuffer);
    }

//        clientSend->sendData(myInBuffer);
}

void MyAudioCfg::toggleSuspend()
{
    if ( isAudioINcreate == false ) {
        qWarning() << "First, push the Start button";
        return;
    }
    // toggle suspend/resume
    if (m_audioInput->state() == QAudio::SuspendedState) {
        qWarning() << "status: Suspended, resume()";
        m_audioInput->resume();
        ui->inpHandleButton->setText(tr(SUSPEND_LABEL));
    } else if (m_audioInput->state() == QAudio::ActiveState) {
        qWarning() << "status: Active, suspend()";
        m_audioInput->suspend();
        ui->inpHandleButton->setText(tr(RESUME_LABEL));
    } else if (m_audioInput->state() == QAudio::StoppedState) {
        qWarning() << "status: Stopped, resume()";
        m_audioInput->resume();
        ui->inpHandleButton->setText(tr(SUSPEND_LABEL));
    } else if (m_audioInput->state() == QAudio::IdleState) {
        qWarning() << "status: IdleState";
    }
}

void MyAudioCfg::handleStateChanged(QAudio::State state)
{
    qWarning() << "state = " << state;
}

void MyAudioCfg::refreshDisplayIN()
{
    if ( isAudioINcreate == true ) {
        ui->AudioLevelBarIN->setValue(m_audioInput->volume()*100);
//        qWarning() << "AudioInfo level is -> " << m_audioInfoIN->level()*100;
    }
//    else
//        qWarning() << "AudioInfoIN is still not opened";
}
void MyAudioCfg::refreshDisplayOUT()
{
    if ( isAudioOUTcreate == true ) {
        ui->AudioLevelBarOUT->setValue(m_audioOutput->volume()*100);

//        qWarning() << "AudioInfo level is -> " << m_audioInfoIN->level()*100;
    }
//    else
//        qWarning() << "AudioInfoOUT is still not opened";
}

void MyAudioCfg::deviceINChanged(int index)
{
    if ( isAudioINcreate == true ) {
        qWarning() << "isAudioINcreate is -> " << isAudioINcreate << "\n";
        m_audioInput->stop();
        m_audioInput->disconnect(this);
        delete m_audioInput;
        isAudioINcreate = false;
    }

    deviceIN = ui->AudioDevCBox_2->itemData(index).value<QAudioDeviceInfo>();

    if (!deviceIN.isFormatSupported(setINput)) {
        qWarning() << "User settings to INput format not supported - trying to use nearest";
        setINput = deviceIN.nearestFormat(setINput);
    }
}

void MyAudioCfg::deviceOUTChanged(int index)
{
    if ( isAudioOUTcreate == true ) {
        m_audioOutput->stop();
        m_audioOutput->disconnect(this);
        delete m_audioOutput;
        isAudioOUTcreate = false;
    }

    deviceOUT = ui->AudioDevCBox_3->itemData(index).value<QAudioDeviceInfo>();

    if (!deviceOUT.isFormatSupported(setOUTput)) {
        qWarning() << "User settings to INput format not supported - trying to use nearest";
        setOUTput = deviceOUT.nearestFormat(setOUTput);
    }
}

void MyAudioCfg::sliderInChanged(int value)
{
    if ( m_audioInput && isAudioINcreate == true ) {
        m_audioInput->setVolume(qreal(value+0.5)/100);
        qWarning() << "AudioInput level set to -> " << (quint32)100*m_audioInput->volume();
    }
    else
        qWarning() << "AudioInput is still not opened";
}
void MyAudioCfg::sliderOutChanged(int value)
{
    if ( m_audioOutput && isAudioOUTcreate == true ) {
        m_audioOutput->setVolume(qreal(value+0.5)/100);
        qWarning() << "AudioOutput level set to -> " << (quint32)100*m_audioOutput->volume();
    }
    else
        qWarning() << "AudioOutput is still not opened";
}
void MyAudioCfg::on_startCapture_clicked()
{
    if ( isAudioINcreate == false ){

//        if (!deviceIN.isFormatSupported(setINput)) {
//            qWarning() << "User settings to INput format not supported - trying to use nearest";
//            setINput = deviceIN.nearestFormat(setINput);
//        }
        createAudioINput();
        ui->startCapture->setText("Stop");
        ui->inpHandleButton->setEnabled(true);
        isAudioINcreate = true;

    } else {
        isAudioINcreate = false;
        ui->startCapture->setText("Start");
        ui->inpHandleButton->setEnabled(false);
        m_audioInput->stop();
        m_audioInput->disconnect(this);
        delete m_audioInput;
    }

}

void MyAudioCfg::on_checkBox_clicked()
{
    qWarning() << "Check Box clicked";
    if ( ui->check_INOUT->isChecked() == true ) {
        if ( ui->check_IN_Stream->isChecked() == true )
            ui->check_IN_Stream->setChecked(false);
//        if ( ui->check_IN_Stream->isChecked() == true )
//            ui->check_INOUT->setChecked(false);
//        if ( isPlayFromStream == false )
            createAudioOUTput();
        isINtoOUTconnect = true;
    } else {
        if ( isAudioOUTcreate == true ){
            isAudioOUTcreate = false;
            m_audioOutput->stop();
            m_audioOutput->disconnect(this);
            delete m_audioOutput;
        }
        isINtoOUTconnect = false;
    }
    if ( isWriteToStream == false )
        on_startCapture_clicked();
}

void MyAudioCfg::on_checkIN_clicked()//play from stream
{
    qWarning() << "Check IN Box clicked";
    if ( ui->check_IN_Stream->isChecked() == true ) {

        if ( isInStreamOn == false ) {
            qWarning() << "Something happened with IN connection,\ncheck Eth settings";
            return;
        }
        if ( ui->check_INOUT->isChecked() == true )
//            ui->check_INOUT->setCheckState( Qt::Unchecked );
            ui->check_INOUT->setChecked(false);

        if ( isAudioOUTcreate == false )
            createAudioOUTput();


        isPlayFromStream = true;
        isINtoOUTconnect = false;
    } else {
        if ( isAudioOUTcreate == true ) {
            m_audioOutput->stop();
            m_audioOutput->disconnect(this);
            delete m_audioOutput;
            isAudioOUTcreate = false;
        }
        isPlayFromStream = false;
    }
}

void MyAudioCfg::on_checkOUT_clicked() {//write to stream
    qWarning() << "Check OUT Box clicked";
    if ( ui->check_OUT_Stream->isChecked() == true ) {

        if ( isOutStreamOn == false ) {
            qWarning() << "Something happened with OUT connection,\ncheck Eth settings";
            return;
        }
        if ( ui->check_INOUT->isChecked() == true )
//            ui->check_INOUT->setCheckState( Qt::Unchecked );
            ui->check_INOUT->setChecked(false);

//        if ( isAudioINcreate == false )
////            createAudioINput();
//        on_startCapture_clicked();

        isWriteToStream = true;
        isINtoOUTconnect = false;
    } else {
//        if ( isAudioINcreate == true ) {
//            m_audioInput->stop();
//            m_audioInput->disconnect(this);
//            delete m_audioInput;
//            isAudioINcreate = false;
//        }
        isWriteToStream = false;
    }
    on_startCapture_clicked();
}

void MyAudioCfg::setEthGUI() {
//    clientRcv = new Receiver;
//    clientSend = new Sender;

    clientEthAudio = new EthAudio(this);

//    threadEthRcv = new QThread;//(clientRcv);  // создаём поток... вначале он создаётся остановленным
//    threadEthSend = new QThread;//(clientSend);
    threadEthAudio = new QThread;

    clientEthAudio->moveToThread(threadEthAudio);
//    connect(threadEthRcv,SIGNAL(started()),clientRcv,SLOT(start())); // когда поток стартует, то начать выполнение работы нашего класса
//    connect(clientRcv,SIGNAL(finished()),threadEthRcv,SLOT(quit())); // когда работа будет завершена, завершить поток
//    connect(clientRcv,SIGNAL(finished()),clientRcv,SLOT(deleteLater())); // когда работа будет завершена, удалить наш экземпляр класса
//    connect(threadEthRcv,SIGNAL(finished()),threadEthRcv,SLOT(deleteLater())); // когда поток остановится, удалить его

//    connect(clientRcv,SIGNAL(sendString(QString)),this,SLOT(reciveString(QString))); // соединить сигнал передачи строки нашего класса со слотом главного окна
////    connect(ui->pushButton_2,SIGNAL(clicked()),clientRcv,SLOT(stop()),Qt::DirectConnection); // при нажатии на вторую кнопку вызывать MyClass::stop()
//           // обрати внимание, что там ещё указан пятый параметр Qt::DirectConnection... он необходим, т.к. наш класс работает в цикле без перерывов
//           // и без этого параметра наш сигнал никогда не достигнет слота класса.
//           // Если бы мы класс делали основанным на событиях, где функции время от времени завершают работу, тогда этот параметр можно было не указывать.
//    thread->start(); // всё настроено, теперь просто запускаем поток.

    connect( clientEthAudio, SIGNAL(RcvEthData(QByteArray)),this, SLOT(rcvEthData(QByteArray)) );
    connect( this, SIGNAL(sendToStream(QByteArray)), clientEthAudio, SLOT(sendData(QByteArray)));//,Qt::DirectConnection );
//    connect(job,SIGNAL(sendString(QString)),ui->label,SLOT(setText(QString))); // Подсоединим сигнал нашего класса к слоту label

    connect( ui->startRcvEthButton, SIGNAL(clicked()), clientEthAudio, SLOT(startReceiver()));//,Qt::DirectConnection );
    connect( ui->startSendEthButton, SIGNAL(clicked()), this, SLOT(on_startSendEthButton_clicked()));//,Qt::DirectConnection );
    connect( ui->startSendEthButton, SIGNAL(clicked()), clientEthAudio, SLOT(startSender()));//,Qt::DirectConnection );
    connect( ui->stopSendEthButton, SIGNAL(clicked()), this, SLOT(on_stopSendEthButton_clicked()));//,Qt::DirectConnection );
    connect( ui->stopSendEthButton, SIGNAL(clicked()), clientEthAudio, SLOT(stopSender()));//,Qt::DirectConnection );
    connect( ui->setEthButton, SIGNAL(clicked()), this, SLOT(on_setEthButton_clicked()));//,Qt::DirectConnection );

    connect( clientEthAudio, SIGNAL(connectedRcv(bool)), this, SLOT(streamINstate(bool)));//,Qt::DirectConnection);
    connect( clientEthAudio, SIGNAL(connectedSend(bool)), this, SLOT(streamOUTstate(bool)));//,Qt::DirectConnection);
    connect( clientEthAudio, SIGNAL(canWriteBuff(bool)), this, SLOT(writeEthBuffState(bool)));//,Qt::DirectConnection);

    threadEthAudio->start();
}

void MyAudioCfg::streamINstate(bool state) {
    qWarning() << "streamINstate chenged to -> " << state;
    isInStreamOn = state;
}
void MyAudioCfg::streamOUTstate(bool state) {
    qWarning() << "streamOUTstate chenged to -> " << state;
    isOutStreamOn = state;
}

void MyAudioCfg::writeEthBuffState(bool state) {
    qWarning() << "isCanWriteToEthBuff chenged to -> " << state;
    isCanWriteToEthBuff = state;
}

void MyAudioCfg::rcvEthData(QByteArray dataBuf) {
//    ui->rcvEthWndw_3->appendPlainText(tr("%1\n").arg(dataBuf.data()));
    if ( isPlayFromStream == true ) {
        myOutBuffer = dataBuf;
    } else
        ui->rcvEthWndw_3->appendPlainText(tr("%1").arg(dataBuf.data()));
    qWarning() << dataBuf.data();
}

void MyAudioCfg::on_startSendEthButton_clicked () {
    ui->stopSendEthButton->setEnabled(true);
    ui->startSendEthButton->setEnabled(false);
}
void MyAudioCfg::on_stopSendEthButton_clicked () {
    ui->stopSendEthButton->setEnabled(false);
    ui->startSendEthButton->setEnabled(true);
}
void MyAudioCfg::on_setEthButton_clicked () {
    if ( isOutStreamOn == false )
        return;

    bool ok;
    QLocale c;
    QString addr = ui->setRemoteAddr->text();
    uint tmp = c.toUInt(ui->setLocalPort->text(),&ok);

    if ( ok == true && tmp > 0 && tmp < 0xffff )
        clientSend->localPort = (quint16)tmp;

    tmp = c.toInt(ui->setRemotePort->text(),&ok);
    if ( ok == true && tmp > 0 && tmp < 0xffff )
        clientSend->remotePort = (quint16)tmp;

    clientSend->remoteIPaddr = addr;
//    if ( clientSend->isStarted() == true )
//        clientSend->restartSender();
    qWarning() << "Remote IP Adrr -> " << clientSend->remoteIPaddr;
    qWarning() << "Remote IP Port -> " << clientSend->remotePort;
    qWarning() << "Local IP Adrr -> " << clientSend->localPort;
//    on_startSendEthButton_clicked();
    ui->startSendEthButton->setEnabled(true);
    ui->stopSendEthButton->setEnabled(false);
}
