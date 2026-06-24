#pragma once

#include "DataProvider.h"
#include "SensorCard.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QWidget>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onDataUpdated(const SensorSnapshot &snapshot);
    void onConnectionChanged(bool connected, const QString &message);
    void onConnectClicked();
    void onSimulationToggled(bool enabled);
    void refreshPorts();

private:
    QWidget *buildHeader();
    QWidget *buildConnectionPanel();
    QWidget *buildSection(const QString &title, QWidget *content);
    QWidget *buildSoilSection();
    QWidget *buildWeatherSection();
    QWidget *buildWaterSection();
    QWidget *buildAirSection();
    QWidget *buildDeviceSection();

    void updateStatusBadge(bool connected);

    DataProvider m_provider;

    QLabel *m_connectionBadge = nullptr;
    QLabel *m_lastUpdateLabel = nullptr;
    QComboBox *m_portCombo = nullptr;
    QComboBox *m_baudCombo = nullptr;
    QPushButton *m_connectBtn = nullptr;
    QPushButton *m_simulationBtn = nullptr;

    SensorCard *m_moistureProbeCard = nullptr;
    SensorCard *m_phCard = nullptr;
    SensorCard *m_ecCard = nullptr;
    SensorCard *m_npkCard = nullptr;
    SensorCard *m_moistureCapCard = nullptr;
    SensorCard *m_moistureRs485Card = nullptr;
    SensorCard *m_sht31Card = nullptr;
    SensorCard *m_rainCard = nullptr;
    SensorCard *m_flowCard = nullptr;
    SensorCard *m_co2Card = nullptr;
    SensorCard *m_esp32Card = nullptr;
};
