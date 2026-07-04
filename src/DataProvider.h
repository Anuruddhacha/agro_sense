#pragma once

#include "SensorData.h"

#include <QObject>
#include <QStringList>
#include <QTimer>

#ifdef HAS_QT_SERIALPORT
class QSerialPort;
#endif

class DataProvider : public QObject
{
    Q_OBJECT

public:
    explicit DataProvider(QObject *parent = nullptr);
    ~DataProvider() override;

    SensorSnapshot snapshot() const { return m_snapshot; }

    void setSimulationEnabled(bool enabled);
    bool isSimulationEnabled() const { return m_simulationEnabled; }
    bool isSerialSupported() const;

    bool connectSerial(const QString &portName, int baudRate = 9600);
    void disconnectSerial();
    bool isSerialConnected() const;

    QStringList availablePorts() const;

public slots:
    void refresh();

signals:
    void dataUpdated(const SensorSnapshot &snapshot);
    void connectionChanged(bool connected, const QString &message);
    void errorOccurred(const QString &message);

private slots:
    void onSerialReadyRead();
    void onSimulationTick();

private:
    void parseModbusFrame(const QByteArray &frame);
    void updateSimulation();
    void initSimulationState();
    void emitSnapshot();

    struct SimulationState {
        bool initialized = false;
        double soilMoisture = 42.0;
        double soilPh = 6.55;
        double soilEc = 1.45;
        double nitrogen = 48.0;
        double phosphorus = 22.0;
        double potassium = 135.0;
        double airTemperature = 24.0;
        double airHumidity = 62.0;
        double co2 = 415.0;
        double rainSessionMm = 0.0;
        double rainRate = 0.0;
        double flowRate = 0.0;
        bool raining = false;
        bool irrigating = false;
        int rainTicksLeft = 0;
        int irrigationTicksLeft = 0;
        int tickCount = 0;
    };

    SensorSnapshot m_snapshot;
#ifdef HAS_QT_SERIALPORT
    QSerialPort *m_serial = nullptr;
#endif
    QTimer m_simulationTimer;
    QByteArray m_rxBuffer;
    bool m_simulationEnabled = true;
    SimulationState m_sim;
};
