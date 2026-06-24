#include "DataProvider.h"

#include <QDateTime>
#include <QRandomGenerator>

#ifdef HAS_QT_SERIALPORT
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

DataProvider::DataProvider(QObject *parent)
    : QObject(parent)
{
#ifdef HAS_QT_SERIALPORT
    m_serial = new QSerialPort(this);
    connect(m_serial, &QSerialPort::readyRead, this, &DataProvider::onSerialReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error == QSerialPort::NoError)
            return;
        emit errorOccurred(m_serial->errorString());
        if (error == QSerialPort::ResourceError)
            disconnectSerial();
    });
#endif

    m_simulationTimer.setInterval(2000);
    connect(&m_simulationTimer, &QTimer::timeout, this, &DataProvider::onSimulationTick);

    m_snapshot.device.portName = QStringLiteral("SIMULATION");
    m_snapshot.device.baudRate = 9600;
    m_snapshot.device.esp32Connected = true;
    m_snapshot.device.rs485Active = true;
    setSimulationEnabled(true);
}

DataProvider::~DataProvider()
{
    disconnectSerial();
}

bool DataProvider::isSerialSupported() const
{
#ifdef HAS_QT_SERIALPORT
    return true;
#else
    return false;
#endif
}

void DataProvider::setSimulationEnabled(bool enabled)
{
    m_simulationEnabled = enabled;
    if (enabled) {
        m_snapshot.device.esp32Connected = true;
        m_snapshot.device.rs485Active = true;
        m_snapshot.device.portName = QStringLiteral("SIMULATION");
        m_simulationTimer.start();
        updateSimulation();
        emit connectionChanged(true, QStringLiteral("Simulation mode active"));
    } else {
        m_simulationTimer.stop();
        if (!isSerialConnected()) {
            m_snapshot.device.esp32Connected = false;
            m_snapshot.device.rs485Active = false;
            emit connectionChanged(false, QStringLiteral("Simulation disabled"));
        }
    }
}

bool DataProvider::connectSerial(const QString &portName, int baudRate)
{
#ifndef HAS_QT_SERIALPORT
    Q_UNUSED(portName)
    Q_UNUSED(baudRate)
    emit errorOccurred(QStringLiteral(
        "Qt SerialPort module is not installed. Use simulation mode or add SerialPort via Qt Maintenance Tool."));
    return false;
#else
    if (m_serial->isOpen())
        disconnectSerial();

    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit errorOccurred(QStringLiteral("Failed to open %1: %2").arg(portName, m_serial->errorString()));
        return false;
    }

    m_simulationEnabled = false;
    m_simulationTimer.stop();

    m_snapshot.device.esp32Connected = true;
    m_snapshot.device.rs485Active = true;
    m_snapshot.device.portName = portName;
    m_snapshot.device.baudRate = baudRate;

    emit connectionChanged(true, QStringLiteral("Connected to %1 @ %2 baud").arg(portName).arg(baudRate));
    return true;
#endif
}

void DataProvider::disconnectSerial()
{
#ifdef HAS_QT_SERIALPORT
    if (m_serial->isOpen())
        m_serial->close();
#endif

    m_rxBuffer.clear();

    if (!m_simulationEnabled) {
        m_snapshot.device.esp32Connected = false;
        m_snapshot.device.rs485Active = false;
        emit connectionChanged(false, QStringLiteral("Serial disconnected"));
    }
}

bool DataProvider::isSerialConnected() const
{
#ifdef HAS_QT_SERIALPORT
    return m_serial->isOpen();
#else
    return false;
#endif
}

QStringList DataProvider::availablePorts() const
{
#ifdef HAS_QT_SERIALPORT
    QStringList ports;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
        ports << info.portName();
    return ports;
#else
    return {};
#endif
}

void DataProvider::refresh()
{
    if (m_simulationEnabled) {
        updateSimulation();
    }
#ifdef HAS_QT_SERIALPORT
    else if (m_serial->isOpen()) {
        const QByteArray request = QByteArray::fromHex("01030000001445C5");
        m_serial->write(request);
    }
#endif
    emitSnapshot();
}

void DataProvider::onSerialReadyRead()
{
#ifdef HAS_QT_SERIALPORT
    m_rxBuffer.append(m_serial->readAll());
    if (m_rxBuffer.size() >= 5)
        parseModbusFrame(m_rxBuffer);
#else
    m_rxBuffer.clear();
#endif
}

void DataProvider::onSimulationTick()
{
    updateSimulation();
    emitSnapshot();
}

void DataProvider::parseModbusFrame(const QByteArray &frame)
{
    if (frame.size() < 45)
        return;

    const auto reg = [&](int index) -> quint16 {
        const int offset = 3 + index * 2;
        return quint8(frame[offset]) << 8 | quint8(frame[offset + 1]);
    };

    m_snapshot.soil.moistureProbe = reg(0) / 10.0;
    m_snapshot.soil.ph = reg(1) / 100.0;
    m_snapshot.soil.ec = reg(2) / 100.0;
    m_snapshot.soil.nitrogen = reg(3);
    m_snapshot.soil.phosphorus = reg(4);
    m_snapshot.soil.potassium = reg(5);
    m_snapshot.soil.moistureCapacitive = reg(6) / 10.0;
    m_snapshot.soil.moistureRs485 = reg(7) / 10.0;
    m_snapshot.weather.temperature = reg(8) / 10.0;
    m_snapshot.weather.humidity = reg(9) / 10.0;
    m_snapshot.weather.rainfall = reg(10) / 10.0;
    m_snapshot.weather.rainRate = reg(11) / 10.0;
    m_snapshot.water.flowRate = reg(12) / 10.0;
    m_snapshot.water.flowTotal = reg(13);
    m_snapshot.air.co2 = reg(14);

    m_rxBuffer.clear();
    emitSnapshot();
}

void DataProvider::updateSimulation()
{
    auto *rng = QRandomGenerator::global();

    m_snapshot.soil.moistureProbe = 35.0 + rng->bounded(200) / 10.0;
    m_snapshot.soil.moistureCapacitive = 32.0 + rng->bounded(180) / 10.0;
    m_snapshot.soil.moistureRs485 = 33.5 + rng->bounded(160) / 10.0;
    m_snapshot.soil.ph = 6.2 + rng->bounded(80) / 100.0;
    m_snapshot.soil.ec = 1.2 + rng->bounded(150) / 100.0;
    m_snapshot.soil.nitrogen = 40 + rng->bounded(30);
    m_snapshot.soil.phosphorus = 18 + rng->bounded(15);
    m_snapshot.soil.potassium = 120 + rng->bounded(40);

    m_snapshot.weather.temperature = 22.0 + rng->bounded(120) / 10.0;
    m_snapshot.weather.humidity = 55.0 + rng->bounded(300) / 10.0;
    if (rng->bounded(100) < 15) {
        m_snapshot.weather.rainRate = rng->bounded(80) / 10.0;
        m_snapshot.weather.rainfall += m_snapshot.weather.rainRate / 30.0;
    } else {
        m_snapshot.weather.rainRate = 0.0;
    }

    m_snapshot.water.flowRate = rng->bounded(250) / 10.0;
    m_flowAccumulator += m_snapshot.water.flowRate / 30.0;
    m_snapshot.water.flowTotal = m_flowAccumulator;

    m_snapshot.air.co2 = 380 + rng->bounded(120);

    m_snapshot.device.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();
}

void DataProvider::emitSnapshot()
{
    m_snapshot.device.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();
    emit dataUpdated(m_snapshot);
}
