#pragma once

#include <furi.h>
#include <furi_hal_bt.h>
#include <furi_hal_bt_serial.h>

#define BT_BUFFER_SIZE 512

// BLE Device info
typedef struct {
    uint8_t address[6];
    int8_t rssi;
    char name[32];
    bool is_chameleon;
} BLEDevice;

// Callback types
typedef void (*BLEDeviceFoundCallback)(BLEDevice* device, void* context);
typedef void (*BLEDataReceivedCallback)(const uint8_t* data, size_t length, void* context);

// BLE Scanner
typedef struct {
    BLEDeviceFoundCallback on_device_found;
    void* callback_context;
    bool scanning;
    FuriThread* scan_thread;
} BLEScanner;

// Serial Profile (SPP) for Bluetooth
typedef struct {
    FuriHalBtSerial* serial;
    uint8_t rx_buffer[BT_BUFFER_SIZE];
    uint8_t tx_buffer[BT_BUFFER_SIZE];
    FuriStreamBuffer* rx_stream;
    BLEDataReceivedCallback on_data_received;
    void* callback_context;
    bool connected;
} SPPSerial;

// Main Bluetooth Manager
typedef struct {
    BLEScanner* scanner;
    SPPSerial* spp;
    bool initialized;
} BTManager;

// BLE Scanner functions
BLEScanner* ble_scanner_alloc(void);
void ble_scanner_free(BLEScanner* scanner);
bool ble_scanner_start(BLEScanner* scanner, BLEDeviceFoundCallback callback, void* context);
void ble_scanner_stop(BLEScanner* scanner);

// SPP Serial functions
SPPSerial* spp_serial_alloc(void);
void spp_serial_free(SPPSerial* spp);
bool spp_serial_start(SPPSerial* spp);
void spp_serial_stop(SPPSerial* spp);
size_t spp_serial_write(SPPSerial* spp, const uint8_t* data, size_t length);
size_t spp_serial_read(SPPSerial* spp, uint8_t* buffer, size_t length);
void spp_serial_set_callback(SPPSerial* spp, BLEDataReceivedCallback callback, void* context);

// Bluetooth Manager functions
BTManager* bt_manager_alloc(void);
void bt_manager_free(BTManager* manager);
bool bt_manager_init(BTManager* manager);
bool bt_manager_scan_start(BTManager* manager, BLEDeviceFoundCallback callback, void* context);
void bt_manager_scan_stop(BTManager* manager);
bool bt_manager_connect(BTManager* manager, const char* device_address);
void bt_manager_disconnect(BTManager* manager);
bool bt_manager_is_connected(BTManager* manager);
size_t bt_manager_send(BTManager* manager, const uint8_t* data, size_t length);
size_t bt_manager_receive(BTManager* manager, uint8_t* buffer, size_t length);
