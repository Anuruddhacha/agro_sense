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
    void emitSnapshot();

    SensorSnapshot m_snapshot;
#ifdef HAS_QT_SERIALPORT
    QSerialPort *m_serial = nullptr;
#endif
    QTimer m_simulationTimer;
    QByteArray m_rxBuffer;
    bool m_simulationEnabled = true;
    double m_flowAccumulator = 0.0;
};
