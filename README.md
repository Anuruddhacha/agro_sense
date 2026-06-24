# Agriculture Sensor Dashboard

Qt 6 C++ dashboard for monitoring agriculture sensors via **ESP32 + RS485 (Modbus RTU)**.

## Sensors

| Sensor | Display |
|--------|---------|
| Soil moisture (probe) | % VWC |
| Soil pH (industrial probe) | pH |
| EC sensor | mS/cm |
| NPK sensor | N, P, K (mg/kg) |
| Soil moisture (capacitive) | % |
| Soil moisture (RS485) | % |
| SHT31 weather sensor | Temperature °C, Humidity % RH |
| Rain gauge | mm total, mm/h rate |
| Flow sensor | L/min, session total L |
| CO₂ sensor | ppm |
| ESP32 + RS485 module | Connection & bus status |

## Features

- Live dashboard with grouped sensor cards (Soil, Weather, Irrigation, Air, Device)
- **Simulation mode** (default) — realistic demo data every 2 seconds
- **Serial / RS485** — connect to ESP32 COM port; parses Modbus holding registers
- Status indicators (OK / WARN / RAIN / FLOWING, etc.)
- Dark agriculture-themed UI

## Requirements

- CMake 3.16+
- Qt 6 (Widgets, SerialPort)
- C++17 compiler (MSVC, MinGW, or Clang)

## Build (Windows)

### Quick start (scripts)

```powershell
# Build
.\build.bat
# or
.\scripts\build.ps1

# Run (builds first if needed, deploys Qt DLLs)
.\run.bat
# or
.\scripts\run.ps1
```

**Script options:**

| Script | Options |
|--------|---------|
| `build.ps1` | `-Config Release\|Debug`, `-Rebuild` (clean build) |
| `run.ps1` | `-Config Release\|Debug`, `-Deploy`, `-NoBuild` |

If Qt is not auto-detected, set your kit path:

```powershell
$env:QT_DIR = "C:\Qt\6.8.0\msvc2019_64"
```

Or copy `.qt-path.example` to `.qt-path` and paste your Qt path on one line.

### Manual build

```powershell
cd C:\Users\MSI\Desktop\CPP_Projects\QT_Dashboard
cmake -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.x.x\msvc2019_64"
cmake --build build --config Release
```

Adjust `CMAKE_PREFIX_PATH` to your Qt installation.

Run:

```powershell
.\build\Release\AgricultureSensorDashboard.exe
```

## ESP32 Modbus Register Map

When using real hardware, firmware should expose these **holding registers** (slave address 1):

| Register | Value | Scale |
|----------|-------|-------|
| 0 | Soil moisture probe | ÷ 10 → % |
| 1 | Soil pH | ÷ 100 |
| 2 | EC | ÷ 100 → mS/cm |
| 3 | Nitrogen | mg/kg |
| 4 | Phosphorus | mg/kg |
| 5 | Potassium | mg/kg |
| 6 | Capacitive moisture | ÷ 10 → % |
| 7 | RS485 moisture | ÷ 10 → % |
| 8 | SHT31 temperature | ÷ 10 → °C |
| 9 | SHT31 humidity | ÷ 10 → % |
| 10 | Rainfall total | ÷ 10 → mm |
| 11 | Rain rate | ÷ 10 → mm/h |
| 12 | Flow rate | ÷ 10 → L/min |
| 13 | Flow total | L |
| 14 | CO₂ | ppm |

Default baud rate: **9600**, 8N1.

## Project Structure

```
QT_Dashboard/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── MainWindow.{h,cpp}
│   ├── SensorCard.{h,cpp}
│   ├── SensorData.{h,cpp}
│   └── DataProvider.{h,cpp}
└── resources/
    ├── resources.qrc
    └── styles/dashboard.qss
```

## License

MIT — use freely for your agriculture sensor project.
