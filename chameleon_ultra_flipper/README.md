# 🦎 Chameleon Ultra - Flipper Zero Integration

Bluetooth connector and full integration with **Chameleon Ultra V2** for Flipper Zero.

## 📋 Features

### Current (v0.1.0 - POC)
- ✅ Bluetooth LE Scanner
- ✅ Serial Profile (SPP) communication
- ✅ Chameleon Ultra protocol parser
- ✅ Basic command support (Get Version, Set Slot, etc.)
- ✅ LF RFID operations (EM410X read/write)
- ✅ HF RFID operations (Mifare Classic read/write)
- ✅ Basic GUI interface

### Planned Features
- 🔄 Complete Chameleon protocol implementation
- 🔄 Kiisu Blocks visual programming interface
- 🔄 Advanced RFID attacks (dictionary, nested, darkside)
- 🔄 Sniffing and protocol analysis
- 🔄 Slot management UI
- 🔄 Tag cloning and emulation

## 🔧 Hardware Requirements

- **Flipper Zero** (with Momentum firmware recommended)
- **Chameleon Ultra V2** device
- Optional: Bluetooth module (if not using built-in BLE)

## 📦 Installation

### Using uFBT (Recommended)

```bash
# Clone the repository
git clone https://github.com/muylder/chameleon-ultra-flipper.git
cd chameleon_ultra_flipper

# Update uFBT SDK
ufbt update

# Build the app
ufbt

# Install to Flipper
ufbt launch
```

### Manual Build

```bash
# Set up Flipper build environment
git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git
cd flipperzero-firmware

# Copy app to applications_user folder
cp -r ../chameleon_ultra_flipper applications_user/

# Build
./fbt fap_chameleon_ultra

# Copy .fap to SD card
cp build/f7-firmware-D/.extapps/chameleon_ultra.fap /path/to/sd/apps/RFID/
```

## 🚀 Usage

### First Connection

1. **Turn on Chameleon Ultra V2**
   - Make sure the device is powered on
   - Enable Bluetooth if using wireless connection

2. **Open the app on Flipper**
   - Navigate to `Apps > RFID > Chameleon Ultra`
   - Press OK to start

3. **Scan for devices**
   - Select "Scan Bluetooth"
   - Wait for Chameleon to appear in the list
   - Select your device

4. **Connect**
   - Select "Connect"
   - Wait for successful connection
   - You should see the version number

### Reading Tags

#### LF Tags (125kHz)
```
1. Select "Read LF Tag"
2. Place tag near Chameleon antenna
3. UID will be displayed
4. Press OK to save to slot
```

#### HF Tags (13.56MHz)
```
1. Select "Read HF Tag"
2. Place tag near Chameleon antenna
3. UID and data will be displayed
4. Press OK to save to slot
```

### Slot Management

The Chameleon Ultra has **16 slots**:
- **Slots 0-7**: LF (125kHz) tags
- **Slots 8-15**: HF (13.56MHz) tags

To switch slots:
1. Go to "Settings"
2. Select "Slot Management"
3. Choose slot number
4. Select LF or HF mode

## 📚 Chameleon Protocol

### Packet Structure

```
┌─────────────┬──────────┬────────┬──────────┬──────────┐
│ Command (2) │ Status(2)│ Len(2) │ Data(N)  │ CRC(2)   │
└─────────────┴──────────┴────────┴──────────┴──────────┘
```

### Example Commands

```c
// Get version
CMD_GET_VERSION (0x1000)
Payload: None
Response: "v2.1.0"

// Read EM410X
CMD_LF_READ_EM410X (0x2000)
Payload: None
Response: 5-byte UID

// Read Mifare block
CMD_MF_READ_BLOCK (0x3101)
Payload: [block_num][key_type][key(6 bytes)]
Response: 16-byte block data
```

## 🛠️ Development

### Project Structure

```
chameleon_ultra_flipper/
├── application.fam           # App manifest
├── chameleon_ultra.c         # Main entry point
├── chameleon_ultra.h         # Main header
├── bluetooth/
│   ├── bt_manager.c          # Bluetooth manager
│   └── bt_manager.h
├── chameleon/
│   ├── protocol.c            # Protocol implementation
│   └── protocol.h
├── gui/
│   ├── scenes/               # App scenes
│   ├── views/                # Custom views
│   └── widgets/              # UI widgets
└── assets/
    └── icons/                # App icons
```

### Adding New Commands

1. Add command enum to `chameleon/protocol.h`:
```c
typedef enum {
    // ...
    CMD_YOUR_COMMAND = 0xXXXX,
} ChameleonCommand;
```

2. Implement handler in `chameleon/protocol.c`:
```c
bool chameleon_your_command(ChameleonDevice* device, ...) {
    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_YOUR_COMMAND, payload, len, &response)) {
        return false;
    }

    return (response->status == STATUS_SUCCESS);
}
```

3. Add to header and use in GUI.

### Debugging

Enable debug logs:
```c
#define FURI_LOG_LEVEL FURI_LOG_LEVEL_DEBUG
```

View logs via serial:
```bash
screen /dev/ttyACM0 115200
```

Or use Flipper CLI:
```
log
```

## 🧪 Testing

### Unit Tests

```bash
# Run tests
ufbt test

# Run specific test
ufbt test_chameleon_protocol
```

### Hardware Testing

1. **BLE Scanner Test**
   - Turn on any BLE device
   - Run scanner
   - Should detect device

2. **Protocol Test**
   - Connect to Chameleon
   - Get version
   - Should return "v2.x.x"

3. **RFID Test**
   - Place EM410X tag near Chameleon
   - Read LF tag
   - Should display UID

## ⚠️ Important Notes

### Legal Disclaimer

This tool is for **educational and authorized testing purposes only**:
- ✅ Use only on devices you own
- ✅ Authorized penetration testing
- ✅ Security research
- ❌ Do NOT clone access cards without permission
- ❌ Do NOT use for illegal activities

### Compatibility

- **Firmware**: Momentum, Unleashed, Official (v0.80.0+)
- **Chameleon**: Ultra V2 (firmware v2.0+)
- **Bluetooth**: BLE 4.0+ required

### Limitations (Current Version)

- Chameleon Tiny not supported (different protocol)
- Some advanced attacks not yet implemented
- GUI is basic (Kiisu Blocks integration coming soon)
- BLE pairing may require manual steps

## 🗺️ Roadmap

See [ROADMAP.md](ROADMAP.md) for detailed development plan.

### Phase 1 (Current) - Foundations ✅
- [x] Project structure
- [x] Bluetooth stack
- [x] Protocol parser
- [x] Basic commands

### Phase 2 (Next) - Full Protocol
- [ ] All Chameleon commands
- [ ] Slot management
- [ ] Tag detection
- [ ] Error handling

### Phase 3 - Kiisu Blocks Integration
- [ ] Visual programming blocks
- [ ] Automated workflows
- [ ] Script library

### Phase 4 - Advanced Features
- [ ] Dictionary attacks
- [ ] Nested attacks
- [ ] Sniffing
- [ ] Protocol analysis

## 🤝 Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see [LICENSE](LICENSE) file.

## 🙏 Credits

- **Chameleon Ultra**: [RRG ChameleonUltra](https://github.com/RfidResearchGroup/ChameleonUltra)
- **Flipper Zero**: [Flipper Devices](https://github.com/flipperdevices)
- **Author**: André De Muylder Oliveira

## 📞 Support

- **Issues**: [GitHub Issues](https://github.com/muylder/chameleon-ultra-flipper/issues)
- **Discussions**: [GitHub Discussions](https://github.com/muylder/chameleon-ultra-flipper/discussions)
- **Email**: andre_muylder@hotmail.com

---

**Made with ❤️ for the Flipper Zero and RFID research community**

Last updated: November 7, 2025
