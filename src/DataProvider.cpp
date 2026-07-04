#include "DataProvider.h"

#include <QDateTime>
#include <QRandomGenerator>

#ifdef HAS_QT_SERIALPORT
#include <QSerialPort>
#include <QSerialPortInfo>
#endif

#include <cmath>

namespace {

double clamp(double value, double minVal, double maxVal)
{
    return std::max(minVal, std::min(maxVal, value));
}

double smoothStep(double current, double target, double alpha)
{
    return current + (target - current) * alpha;
}

double sensorNoise(double amplitude = 0.15)
{
    auto *rng = QRandomGenerator::global();
    return (rng->generateDouble() - 0.5) * 2.0 * amplitude;
}

} // namespace

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

    m_simulationTimer.setInterval(1000);
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
        initSimulationState();
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

void DataProvider::initSimulationState()
{
    auto *rng = QRandomGenerator::global();
    m_sim = SimulationState{};
    m_sim.soilMoisture = 38.0 + rng->bounded(120) / 10.0;
    m_sim.soilPh = 6.45 + rng->bounded(20) / 100.0;
    m_sim.soilEc = 1.2 + rng->bounded(80) / 100.0;
    m_sim.nitrogen = 42.0 + rng->bounded(18);
    m_sim.phosphorus = 18.0 + rng->bounded(10);
    m_sim.potassium = 125.0 + rng->bounded(25);
    m_sim.co2 = 405.0 + rng->bounded(40);
    m_sim.initialized = true;
    m_snapshot.water.flowTotal = 0.0;
}

void DataProvider::updateSimulation()
{
    if (!m_sim.initialized)
        initSimulationState();

    auto *rng = QRandomGenerator::global();
    const QDateTime now = QDateTime::currentDateTime();
    const double hour = now.time().hour() + now.time().minute() / 60.0 + now.time().second() / 3600.0;
    ++m_sim.tickCount;

    // Diurnal air temperature (greenhouse / open field blend)
    const double tempTarget = 21.0 + 7.5 * std::sin((hour - 9.0) * M_PI / 12.0);
    m_sim.airTemperature = smoothStep(m_sim.airTemperature, tempTarget, 0.08) + sensorNoise(0.08);

    // Rain event state machine
    if (m_sim.raining) {
        --m_sim.rainTicksLeft;
        if (m_sim.rainTicksLeft <= 0) {
            m_sim.raining = false;
            m_sim.rainRate = smoothStep(m_sim.rainRate, 0.0, 0.35);
        } else {
            const double targetRate = 1.5 + rng->bounded(100) / 10.0;
            m_sim.rainRate = smoothStep(m_sim.rainRate, targetRate, 0.12);
        }
    } else {
        m_sim.rainRate = smoothStep(m_sim.rainRate, 0.0, 0.25);
        if (rng->bounded(1000) < 3) {
            m_sim.raining = true;
            m_sim.rainTicksLeft = 120 + rng->bounded(900);
        }
    }

    if (m_sim.rainRate > 0.05) {
        m_sim.rainSessionMm += m_sim.rainRate / 3600.0;
    }

    // Humidity rises with rain and falls on warm afternoons
    double humidityTarget = 88.0 - (m_sim.airTemperature - 18.0) * 1.8;
    if (m_sim.raining)
        humidityTarget += 12.0;
    humidityTarget = clamp(humidityTarget, 38.0, 96.0);
    m_sim.airHumidity = smoothStep(m_sim.airHumidity, humidityTarget, 0.06) + sensorNoise(0.4);

    // Scheduled irrigation: morning and late afternoon when soil is dry
    const bool irrigationWindow = (hour >= 6.0 && hour <= 7.5) || (hour >= 17.0 && hour <= 18.5);
    if (!m_sim.irrigating && irrigationWindow && m_sim.soilMoisture < 40.0 && rng->bounded(100) < 35) {
        m_sim.irrigating = true;
        m_sim.irrigationTicksLeft = 180 + rng->bounded(240);
    }

    if (m_sim.irrigating) {
        --m_sim.irrigationTicksLeft;
        const double targetFlow = 2.2 + rng->bounded(18) / 10.0;
        m_sim.flowRate = smoothStep(m_sim.flowRate, targetFlow, 0.2);
        if (m_sim.irrigationTicksLeft <= 0 || m_sim.soilMoisture > 58.0)
            m_sim.irrigating = false;
    } else {
        m_sim.flowRate = smoothStep(m_sim.flowRate, 0.0, 0.3);
    }

    if (m_sim.flowRate > 0.05)
        m_snapshot.water.flowTotal += m_sim.flowRate / 60.0;

    // Soil water balance
    double moistureDelta = 0.0;
    if (m_sim.rainRate > 0.0)
        moistureDelta += m_sim.rainRate * 0.0045;
    if (m_sim.flowRate > 0.0)
        moistureDelta += m_sim.flowRate * 0.011;

    const double evaporation = clamp((m_sim.airTemperature - 12.0) * 0.0028 - m_sim.airHumidity * 0.0008, 0.0, 0.06);
    const double plantUptake = m_sim.soilMoisture > 30.0 ? 0.0035 : 0.0010;
    moistureDelta += -evaporation - plantUptake;

    m_sim.soilMoisture = clamp(m_sim.soilMoisture + moistureDelta, 18.0, 78.0);

    // pH and EC drift slowly; EC rises slightly during irrigation (dissolved salts)
    m_sim.soilPh = clamp(m_sim.soilPh + sensorNoise(0.004), 5.8, 7.4);
    const double ecTarget = 1.15 + (m_sim.soilMoisture * 0.006) + (m_sim.irrigating ? 0.08 : 0.0);
    m_sim.soilEc = smoothStep(m_sim.soilEc, ecTarget, 0.04) + sensorNoise(0.01);

    // NPK changes very slowly (nutrient depletion + minor noise)
    m_sim.nitrogen = clamp(m_sim.nitrogen - 0.01 + sensorNoise(0.05), 28.0, 72.0);
    m_sim.phosphorus = clamp(m_sim.phosphorus - 0.004 + sensorNoise(0.03), 12.0, 38.0);
    m_sim.potassium = clamp(m_sim.potassium - 0.008 + sensorNoise(0.08), 90.0, 190.0);

    // CO₂: lower midday (photosynthesis), higher night
    const double co2Target = 430.0 - 35.0 * std::sin((hour - 6.0) * M_PI / 12.0);
    m_sim.co2 = smoothStep(m_sim.co2, co2Target, 0.05) + sensorNoise(2.0);
    m_sim.co2 = clamp(m_sim.co2, 380.0, 780.0);

    // Three moisture sensors track the same field with realistic offsets
    m_snapshot.soil.moistureProbe = clamp(m_sim.soilMoisture + 1.2 + sensorNoise(0.25), 0.0, 100.0);
    m_snapshot.soil.moistureCapacitive = clamp(m_sim.soilMoisture - 0.8 + sensorNoise(0.35), 0.0, 100.0);
    m_snapshot.soil.moistureRs485 = clamp(m_sim.soilMoisture + 0.3 + sensorNoise(0.2), 0.0, 100.0);
    m_snapshot.soil.ph = m_sim.soilPh;
    m_snapshot.soil.ec = m_sim.soilEc;
    m_snapshot.soil.nitrogen = m_sim.nitrogen;
    m_snapshot.soil.phosphorus = m_sim.phosphorus;
    m_snapshot.soil.potassium = m_sim.potassium;

    m_snapshot.weather.temperature = m_sim.airTemperature;
    m_snapshot.weather.humidity = m_sim.airHumidity;
    m_snapshot.weather.rainfall = m_sim.rainSessionMm;
    m_snapshot.weather.rainRate = m_sim.rainRate;

    m_snapshot.water.flowRate = m_sim.flowRate;
    m_snapshot.air.co2 = m_sim.co2;

    m_snapshot.device.lastUpdateMs = now.toMSecsSinceEpoch();
}

void DataProvider::emitSnapshot()
{
    m_snapshot.device.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();
    emit dataUpdated(m_snapshot);
}
