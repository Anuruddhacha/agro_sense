#include "MainWindow.h"

#include <QDateTime>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollArea>
#include <QStyle>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Agriculture Sensor Dashboard"));
    resize(1280, 860);
    setObjectName(QStringLiteral("mainWindow"));

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(20, 20, 20, 20);
    outer->setSpacing(16);

    outer->addWidget(buildHeader());
    outer->addWidget(buildConnectionPanel());

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setObjectName(QStringLiteral("dashboardScroll"));

    auto *content = new QWidget;
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(18);
    contentLayout->addWidget(buildSoilSection());
    contentLayout->addWidget(buildWeatherSection());
    contentLayout->addWidget(buildWaterSection());
    contentLayout->addWidget(buildAirSection());
    contentLayout->addWidget(buildDeviceSection());
    contentLayout->addStretch();

    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    connect(&m_provider, &DataProvider::dataUpdated, this, &MainWindow::onDataUpdated);
    connect(&m_provider, &DataProvider::connectionChanged, this, &MainWindow::onConnectionChanged);
    connect(&m_provider, &DataProvider::errorOccurred, this, [this](const QString &msg) {
        QMessageBox::warning(this, QStringLiteral("Connection Error"), msg);
    });

    refreshPorts();
    onDataUpdated(m_provider.snapshot());
    updateStatusBadge(true);
}

QWidget *MainWindow::buildHeader()
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("headerFrame"));

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(20, 16, 20, 16);

    auto *titleBlock = new QVBoxLayout;
    auto *title = new QLabel(QStringLiteral("Agriculture Sensor Dashboard"));
    title->setObjectName(QStringLiteral("appTitle"));
    auto *subtitle = new QLabel(QStringLiteral(
        "ESP32 + RS485 · Soil · Weather · Irrigation · Air Quality"));
    subtitle->setObjectName(QStringLiteral("appSubtitle"));
    titleBlock->addWidget(title);
    titleBlock->addWidget(subtitle);

    m_connectionBadge = new QLabel(QStringLiteral("● SIMULATION"));
    m_connectionBadge->setObjectName(QStringLiteral("connectionBadge"));
    m_connectionBadge->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_lastUpdateLabel = new QLabel(QStringLiteral("Last update: --"));
    m_lastUpdateLabel->setObjectName(QStringLiteral("lastUpdate"));
    m_lastUpdateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *right = new QVBoxLayout;
    right->addWidget(m_connectionBadge);
    right->addWidget(m_lastUpdateLabel);

    layout->addLayout(titleBlock, 1);
    layout->addLayout(right);

    return frame;
}

QWidget *MainWindow::buildConnectionPanel()
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("connectionPanel"));

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(12);

    auto *label = new QLabel(QStringLiteral("ESP32 Serial Port"));
    label->setObjectName(QStringLiteral("panelLabel"));

    m_portCombo = new QComboBox;
    m_portCombo->setMinimumWidth(140);
    m_portCombo->setObjectName(QStringLiteral("portCombo"));

    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({QStringLiteral("9600"), QStringLiteral("19200"),
                           QStringLiteral("38400"), QStringLiteral("115200")});
    m_baudCombo->setCurrentText(QStringLiteral("9600"));

    auto *refreshBtn = new QPushButton(QStringLiteral("Refresh Ports"));
    refreshBtn->setObjectName(QStringLiteral("secondaryButton"));
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::refreshPorts);

    m_connectBtn = new QPushButton(QStringLiteral("Connect RS485"));
    m_connectBtn->setObjectName(QStringLiteral("primaryButton"));
    connect(m_connectBtn, &QPushButton::clicked, this, &MainWindow::onConnectClicked);

    m_simulationBtn = new QPushButton(QStringLiteral("Simulation: ON"));
    m_simulationBtn->setObjectName(QStringLiteral("toggleButton"));
    m_simulationBtn->setCheckable(true);
    m_simulationBtn->setChecked(true);
    connect(m_simulationBtn, &QPushButton::toggled, this, &MainWindow::onSimulationToggled);

    layout->addWidget(label);
    layout->addWidget(m_portCombo);
    layout->addWidget(new QLabel(QStringLiteral("Baud")));
    layout->addWidget(m_baudCombo);
    layout->addWidget(refreshBtn);
    layout->addStretch();
    layout->addWidget(m_simulationBtn);
    layout->addWidget(m_connectBtn);

    if (!m_provider.isSerialSupported()) {
        m_portCombo->setEnabled(false);
        m_baudCombo->setEnabled(false);
        m_connectBtn->setEnabled(false);
        m_connectBtn->setToolTip(QStringLiteral(
            "Install Qt SerialPort module via Qt Maintenance Tool to enable RS485."));
    }

    return frame;
}

QWidget *MainWindow::buildSection(const QString &title, QWidget *content)
{
    auto *frame = new QFrame;
    frame->setObjectName(QStringLiteral("sectionFrame"));

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 16, 18, 18);
    layout->setSpacing(12);

    auto *sectionTitle = new QLabel(title);
    sectionTitle->setObjectName(QStringLiteral("sectionTitle"));
    layout->addWidget(sectionTitle);
    layout->addWidget(content);

    return frame;
}

QWidget *MainWindow::buildSoilSection()
{
    auto *grid = new QWidget;
    auto *layout = new QGridLayout(grid);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    m_moistureProbeCard = new SensorCard(
        QStringLiteral("Soil Moisture (Probe)"), QStringLiteral("💧"), QStringLiteral("%"));
    m_phCard = new SensorCard(
        QStringLiteral("Soil pH (Industrial Probe)"), QStringLiteral("⚗"), QStringLiteral("pH"));
    m_ecCard = new SensorCard(
        QStringLiteral("EC Sensor"), QStringLiteral("⚡"), QStringLiteral("mS/cm"));
    m_npkCard = new SensorCard(
        QStringLiteral("NPK Sensor"), QStringLiteral("🌱"), QStringLiteral("mg/kg"));
    m_moistureCapCard = new SensorCard(
        QStringLiteral("Soil Moisture (Capacitive)"), QStringLiteral("📡"), QStringLiteral("%"));
    m_moistureRs485Card = new SensorCard(
        QStringLiteral("Soil Moisture (RS485)"), QStringLiteral("🔌"), QStringLiteral("%"));

    layout->addWidget(m_moistureProbeCard, 0, 0);
    layout->addWidget(m_phCard, 0, 1);
    layout->addWidget(m_ecCard, 0, 2);
    layout->addWidget(m_npkCard, 1, 0);
    layout->addWidget(m_moistureCapCard, 1, 1);
    layout->addWidget(m_moistureRs485Card, 1, 2);

    for (int c = 0; c < 3; ++c)
        layout->setColumnStretch(c, 1);

    return buildSection(QStringLiteral("Soil Sensors"), grid);
}

QWidget *MainWindow::buildWeatherSection()
{
    auto *grid = new QWidget;
    auto *layout = new QGridLayout(grid);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    m_sht31Card = new SensorCard(
        QStringLiteral("SHT31 Weather Sensor"), QStringLiteral("🌡"), QStringLiteral("°C"));
    m_rainCard = new SensorCard(
        QStringLiteral("Rain Gauge"), QStringLiteral("🌧"), QStringLiteral("mm"));

    layout->addWidget(m_sht31Card, 0, 0);
    layout->addWidget(m_rainCard, 0, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);

    return buildSection(QStringLiteral("Weather"), grid);
}

QWidget *MainWindow::buildWaterSection()
{
    auto *grid = new QWidget;
    auto *layout = new QGridLayout(grid);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    m_flowCard = new SensorCard(
        QStringLiteral("Flow Sensor"), QStringLiteral("🚿"), QStringLiteral("L/min"));

    layout->addWidget(m_flowCard, 0, 0);
    layout->setColumnStretch(0, 1);

    return buildSection(QStringLiteral("Irrigation"), grid);
}

QWidget *MainWindow::buildAirSection()
{
    auto *grid = new QWidget;
    auto *layout = new QGridLayout(grid);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    m_co2Card = new SensorCard(
        QStringLiteral("CO₂ Sensor"), QStringLiteral("🫁"), QStringLiteral("ppm"));

    layout->addWidget(m_co2Card, 0, 0);
    layout->setColumnStretch(0, 1);

    return buildSection(QStringLiteral("Air Quality"), grid);
}

QWidget *MainWindow::buildDeviceSection()
{
    auto *grid = new QWidget;
    auto *layout = new QGridLayout(grid);
    layout->setSpacing(12);
    layout->setContentsMargins(0, 0, 0, 0);

    m_esp32Card = new SensorCard(
        QStringLiteral("ESP32 + RS485 Module"), QStringLiteral("📟"), QStringLiteral(""));

    layout->addWidget(m_esp32Card, 0, 0);
    layout->setColumnStretch(0, 1);

    return buildSection(QStringLiteral("Device Status"), grid);
}

void MainWindow::onDataUpdated(const SensorSnapshot &s)
{
    m_moistureProbeCard->setValue(QString::number(s.soil.moistureProbe, 'f', 1));
    m_moistureProbeCard->setStatus(
        s.soil.moistureProbe < 25 ? QStringLiteral("DRY") :
        s.soil.moistureProbe > 70 ? QStringLiteral("WET") : QStringLiteral("OK"),
        s.soil.moistureProbe < 25 || s.soil.moistureProbe > 70 ? QStringLiteral("warn") : QStringLiteral("ok"));

    m_phCard->setValue(QString::number(s.soil.ph, 'f', 2));
    m_phCard->setSubValue(QStringLiteral("Optimal range: 6.0 – 7.0"));
    m_phCard->setStatus(
        s.soil.ph < 5.5 || s.soil.ph > 7.5 ? QStringLiteral("CHECK") : QStringLiteral("OK"),
        s.soil.ph < 5.5 || s.soil.ph > 7.5 ? QStringLiteral("warn") : QStringLiteral("ok"));

    m_ecCard->setValue(QString::number(s.soil.ec, 'f', 2));
    m_ecCard->setSubValue(QStringLiteral("Salinity indicator"));

    m_npkCard->setValue(QStringLiteral("N %1").arg(int(s.soil.nitrogen)));
    m_npkCard->setSubValue(QStringLiteral("P %1 · K %2 mg/kg")
                               .arg(int(s.soil.phosphorus))
                               .arg(int(s.soil.potassium)));

    m_moistureCapCard->setValue(QString::number(s.soil.moistureCapacitive, 'f', 1));
    m_moistureRs485Card->setValue(QString::number(s.soil.moistureRs485, 'f', 1));

    m_sht31Card->setValue(QString::number(s.weather.temperature, 'f', 1));
    m_sht31Card->setSubValue(QStringLiteral("Humidity %1% RH")
                                 .arg(QString::number(s.weather.humidity, 'f', 1)));

    m_rainCard->setValue(QString::number(s.weather.rainfall, 'f', 1));
    m_rainCard->setSubValue(QStringLiteral("Rate %1 mm/h")
                                .arg(QString::number(s.weather.rainRate, 'f', 1)));
    m_rainCard->setStatus(
        s.weather.rainRate > 0 ? QStringLiteral("RAINING") : QStringLiteral("DRY"),
        s.weather.rainRate > 0 ? QStringLiteral("info") : QStringLiteral("ok"));

    m_flowCard->setValue(QString::number(s.water.flowRate, 'f', 1));
    m_flowCard->setSubValue(QStringLiteral("Total %1 L").arg(QString::number(s.water.flowTotal, 'f', 1)));
    m_flowCard->setStatus(
        s.water.flowRate > 0 ? QStringLiteral("FLOWING") : QStringLiteral("IDLE"),
        s.water.flowRate > 0 ? QStringLiteral("info") : QStringLiteral("ok"));

    m_co2Card->setValue(QString::number(s.air.co2, 'f', 0));
    m_co2Card->setSubValue(QStringLiteral("Ambient CO₂ level"));
    m_co2Card->setStatus(
        s.air.co2 > 1000 ? QStringLiteral("HIGH") : QStringLiteral("OK"),
        s.air.co2 > 1000 ? QStringLiteral("warn") : QStringLiteral("ok"));

    const QString portInfo = s.device.portName.isEmpty()
                                 ? QStringLiteral("Not connected")
                                 : QStringLiteral("%1 @ %2").arg(s.device.portName).arg(s.device.baudRate);

    m_esp32Card->setValue(s.device.esp32Connected ? QStringLiteral("ONLINE") : QStringLiteral("OFFLINE"));
    m_esp32Card->setSubValue(QStringLiteral("RS485 %1 · %2")
                                 .arg(s.device.rs485Active ? QStringLiteral("Active") : QStringLiteral("Inactive"))
                                 .arg(portInfo));
    m_esp32Card->setStatus(
        s.device.esp32Connected ? QStringLiteral("CONNECTED") : QStringLiteral("DISCONNECTED"),
        s.device.esp32Connected ? QStringLiteral("ok") : QStringLiteral("error"));

    if (s.device.lastUpdateMs > 0) {
        const QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.device.lastUpdateMs);
        m_lastUpdateLabel->setText(QStringLiteral("Last update: %1")
                                     .arg(dt.toString(QStringLiteral("hh:mm:ss"))));
    }
}

void MainWindow::onConnectionChanged(bool connected, const QString &message)
{
    Q_UNUSED(message)
    updateStatusBadge(connected);
}

void MainWindow::onConnectClicked()
{
    if (m_provider.isSerialConnected()) {
        m_provider.disconnectSerial();
        m_connectBtn->setText(QStringLiteral("Connect RS485"));
        if (m_simulationBtn->isChecked())
            m_provider.setSimulationEnabled(true);
        return;
    }

    if (m_portCombo->currentText().isEmpty()) {
        QMessageBox::information(this, QStringLiteral("No Port"),
                               QStringLiteral("No serial port selected. Refresh ports or use simulation mode."));
        return;
    }

    const int baud = m_baudCombo->currentText().toInt();
    if (m_provider.connectSerial(m_portCombo->currentText(), baud)) {
        m_connectBtn->setText(QStringLiteral("Disconnect"));
        m_simulationBtn->setChecked(false);
        m_provider.setSimulationEnabled(false);
    }
}

void MainWindow::onSimulationToggled(bool enabled)
{
    m_simulationBtn->setText(enabled ? QStringLiteral("Simulation: ON")
                                       : QStringLiteral("Simulation: OFF"));
    m_provider.setSimulationEnabled(enabled);
    if (enabled && m_provider.isSerialConnected()) {
        m_provider.disconnectSerial();
        m_connectBtn->setText(QStringLiteral("Connect RS485"));
    }
}

void MainWindow::refreshPorts()
{
    const QString current = m_portCombo->currentText();
    m_portCombo->clear();
    const QStringList ports = m_provider.availablePorts();
    m_portCombo->addItems(ports);
    const int idx = m_portCombo->findText(current);
    if (idx >= 0)
        m_portCombo->setCurrentIndex(idx);
}

void MainWindow::updateStatusBadge(bool connected)
{
    if (m_provider.isSimulationEnabled()) {
        m_connectionBadge->setText(QStringLiteral("● SIMULATION"));
        m_connectionBadge->setProperty("connected", true);
    } else if (connected) {
        m_connectionBadge->setText(QStringLiteral("● CONNECTED"));
        m_connectionBadge->setProperty("connected", true);
    } else {
        m_connectionBadge->setText(QStringLiteral("● OFFLINE"));
        m_connectionBadge->setProperty("connected", false);
    }
    m_connectionBadge->style()->unpolish(m_connectionBadge);
    m_connectionBadge->style()->polish(m_connectionBadge);
}
