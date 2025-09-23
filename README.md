# OBD-II K-Line Reader

OBD-II K-Line Reader with ESP32 Support - A comprehensive OBD-II diagnostic tool with real-time monitoring, data logging, and multi-connection support.

## Features

### Phase 1 - Core OBD-II Support
- **OBD-II Protocol Parser**: Supports ISO 9141-2, ISO 14230-4 (KWP2000), ISO 15765-4 (CAN)
- **ESP32 Serial Driver**: Serial, WiFi, and Bluetooth connectivity
- **OBD-II Command Library**: Pre-defined PIDs for various vehicle makes
- **Real-time Dashboard**: Live OBD-II data monitoring with gauges and charts

### Phase 2 - Advanced Features
- **DTC Viewer and Database**: Diagnostic Trouble Code management with descriptions
- **Data Logging System**: CSV/JSON/Binary data logging with compression
- **Connection Manager**: Multi-source connection management (FTDI + ESP32)

### Phase 3 - Polish & Optimization
- **Data Export/Import**: Excel/PDF/HTML/CSV/JSON/XML export formats
- **Settings/Configuration**: Comprehensive configuration management

## Prerequisites

### Windows
- MinGW-w64 or Visual Studio 2019+
- SDL2 development libraries
- OpenGL libraries
- (Optional) vcpkg for package management

### Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install build-essential cmake
sudo apt install libsdl2-dev libgl1-mesa-dev libglu1-mesa-dev
sudo apt install libusb-1.0-0-dev
```

### macOS
```bash
brew install cmake sdl2
brew install libusb
```

## Building

### Method 1: Simple Build (Recommended)
```bash
# Copy the simple CMakeLists.txt
cp CMakeLists_simple.txt CMakeLists.txt

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build .
```

### Method 2: Using vcpkg (Windows)
```bash
# Install vcpkg packages
vcpkg install sdl2:x64-windows
vcpkg install glad:x64-windows
vcpkg install libusb:x64-windows

# Configure with vcpkg
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-windows

# Build
cmake --build build
```

### Method 3: Manual Dependencies
If you have SDL2 and GLAD installed manually:

```bash
# Set environment variables
export SDL2DIR=/path/to/sdl2
export GLADDIR=/path/to/glad

# Configure
cmake -S . -B build

# Build
cmake --build build
```

## Usage

### Basic Usage
```bash
./obd_reader
```

### Configuration
The application will create a `config.ini` file in the current directory with default settings.

### Connection Types
1. **FTDI Serial**: Direct connection to OBD-II adapter
2. **ESP32 Serial**: Connection via ESP32 over USB
3. **ESP32 WiFi**: Connection via ESP32 over WiFi
4. **ESP32 Bluetooth**: Connection via ESP32 over Bluetooth

### Data Export
The application supports multiple export formats:
- CSV (Comma Separated Values)
- JSON (JavaScript Object Notation)
- XML (eXtensible Markup Language)
- Excel (XLSX)
- PDF (Portable Document Format)
- HTML (HyperText Markup Language)

## Project Structure

```
├── include/                 # Header files
│   ├── common.h            # Common definitions
│   ├── ui.h                # UI components
│   ├── worker.h            # Worker thread
│   ├── obd_parser.h        # OBD-II parser
│   ├── esp32_driver.h      # ESP32 driver
│   ├── obd_commands.h      # OBD-II commands
│   ├── dashboard.h         # Dashboard
│   ├── dtc_viewer.h        # DTC viewer
│   ├── data_logger.h       # Data logging
│   ├── connection_manager.h # Connection management
│   ├── data_export.h       # Data export
│   └── settings.h          # Settings management
├── src/                    # Source files
│   ├── main.c             # Main application
│   ├── ui.c               # UI implementation
│   ├── worker.c           # Worker thread
│   ├── ringbuffer.c       # Ring buffer
│   ├── nk_impl.c          # Nuklear implementation
│   ├── ftdi_*.c           # FTDI drivers
│   └── obd_*.c            # OBD-II components
├── third_party/           # Third-party libraries
│   ├── nuklear.h          # Nuklear GUI
│   └── nuklear_sdl_gl3.h  # Nuklear SDL OpenGL3
├── CMakeLists.txt         # CMake configuration
└── README.md             # This file
```

## Troubleshooting

### SDL2 Not Found
```bash
# Ubuntu/Debian
sudo apt install libsdl2-dev

# Windows (vcpkg)
vcpkg install sdl2:x64-windows

# macOS
brew install sdl2
```

### GLAD Not Found
```bash
# Windows (vcpkg)
vcpkg install glad:x64-windows

# Manual installation
# Download GLAD from https://glad.dav1d.de/
# Extract to a directory and set GLADDIR environment variable
```

### OpenGL Not Found
```bash
# Ubuntu/Debian
sudo apt install libgl1-mesa-dev libglu1-mesa-dev

# Windows
# OpenGL is usually included with graphics drivers
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Acknowledgments

- Nuklear GUI library
- SDL2 for cross-platform support
- OpenGL for rendering
- libusb for USB communication