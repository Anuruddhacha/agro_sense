#pragma once

#include <QtGlobal>
#include <QString>

struct SoilReadings {
    double moistureProbe = 0.0;      // % VWC
    double moistureCapacitive = 0.0;   // %
    double moistureRs485 = 0.0;        // %
    double ph = 0.0;                   // pH
    double ec = 0.0;                   // mS/cm
    double nitrogen = 0.0;             // mg/kg
    double phosphorus = 0.0;           // mg/kg
    double potassium = 0.0;            // mg/kg
};

struct WeatherReadings {
    double temperature = 0.0;   // °C
    double humidity = 0.0;     // % RH
    double rainfall = 0.0;     // mm (session total)
    double rainRate = 0.0;     // mm/h
};

struct WaterReadings {
    double flowRate = 0.0;     // L/min
    double flowTotal = 0.0;    // L (session total)
};

struct AirReadings {
    double co2 = 0.0;          // ppm
};

struct DeviceStatus {
    bool esp32Connected = false;
    bool rs485Active = false;
    QString portName;
    int baudRate = 9600;
    int modbusAddress = 1;
    qint64 lastUpdateMs = 0;
};

struct SensorSnapshot {
    SoilReadings soil;
    WeatherReadings weather;
    WaterReadings water;
    AirReadings air;
    DeviceStatus device;
};
