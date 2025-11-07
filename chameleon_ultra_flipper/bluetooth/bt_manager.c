#include "bt_manager.h"
#include <furi.h>

#define TAG "BTManager"

// ============================================================================
// BLE Scanner Implementation
// ============================================================================

static int32_t ble_scanner_thread(void* context) {
    BLEScanner* scanner = (BLEScanner*)context;

    FURI_LOG_I(TAG, "Scanner thread started");

    // Initialize Bluetooth
    furi_hal_bt_init();

    // TODO: Implement actual BLE scanning
    // This is a placeholder - real implementation depends on Flipper's BLE API
    while(scanner->scanning) {
        // Simulated scan - in real implementation, this would use furi_hal_bt APIs
        furi_delay_ms(1000);

        // Example: Create fake device for testing
        BLEDevice device = {
            .address = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
            .rssi = -60,
            .is_chameleon = true,
        };
        snprintf(device.name, sizeof(device.name), "Chameleon Ultra");

        if(scanner->on_device_found) {
            scanner->on_device_found(&device, scanner->callback_context);
        }

        // Break after first scan in this example
        break;
    }

    FURI_LOG_I(TAG, "Scanner thread stopped");
    return 0;
}

BLEScanner* ble_scanner_alloc(void) {
    BLEScanner* scanner = malloc(sizeof(BLEScanner));
    scanner->scanning = false;
    scanner->on_device_found = NULL;
    scanner->callback_context = NULL;
    scanner->scan_thread = NULL;
    return scanner;
}

void ble_scanner_free(BLEScanner* scanner) {
    furi_assert(scanner);

    if(scanner->scanning) {
        ble_scanner_stop(scanner);
    }

    free(scanner);
}

bool ble_scanner_start(BLEScanner* scanner, BLEDeviceFoundCallback callback, void* context) {
    furi_assert(scanner);

    if(scanner->scanning) {
        FURI_LOG_W(TAG, "Scanner already running");
        return false;
    }

    scanner->on_device_found = callback;
    scanner->callback_context = context;
    scanner->scanning = true;

    // Create scan thread
    scanner->scan_thread = furi_thread_alloc();
    furi_thread_set_name(scanner->scan_thread, "BLEScanThread");
    furi_thread_set_stack_size(scanner->scan_thread, 2048);
    furi_thread_set_context(scanner->scan_thread, scanner);
    furi_thread_set_callback(scanner->scan_thread, ble_scanner_thread);
    furi_thread_start(scanner->scan_thread);

    FURI_LOG_I(TAG, "BLE scan started");
    return true;
}

void ble_scanner_stop(BLEScanner* scanner) {
    furi_assert(scanner);

    if(!scanner->scanning) {
        return;
    }

    scanner->scanning = false;

    if(scanner->scan_thread) {
        furi_thread_join(scanner->scan_thread);
        furi_thread_free(scanner->scan_thread);
        scanner->scan_thread = NULL;
    }

    FURI_LOG_I(TAG, "BLE scan stopped");
}

// ============================================================================
// SPP Serial Implementation
// ============================================================================

static void spp_serial_rx_callback(uint8_t* data, size_t size, void* context) {
    SPPSerial* spp = (SPPSerial*)context;

    // Store in stream buffer
    furi_stream_buffer_send(spp->rx_stream, data, size, 0);

    // Call user callback
    if(spp->on_data_received) {
        spp->on_data_received(data, size, spp->callback_context);
    }

    FURI_LOG_D(TAG, "SPP RX: %zu bytes", size);
}

SPPSerial* spp_serial_alloc(void) {
    SPPSerial* spp = malloc(sizeof(SPPSerial));

    spp->serial = furi_hal_bt_serial_alloc();
    spp->rx_stream = furi_stream_buffer_alloc(BT_BUFFER_SIZE, 1);
    spp->on_data_received = NULL;
    spp->callback_context = NULL;
    spp->connected = false;

    return spp;
}

void spp_serial_free(SPPSerial* spp) {
    furi_assert(spp);

    if(spp->connected) {
        spp_serial_stop(spp);
    }

    furi_stream_buffer_free(spp->rx_stream);
    furi_hal_bt_serial_free(spp->serial);
    free(spp);
}

bool spp_serial_start(SPPSerial* spp) {
    furi_assert(spp);

    if(spp->connected) {
        FURI_LOG_W(TAG, "SPP already connected");
        return false;
    }

    // Set RX callback
    furi_hal_bt_serial_set_event_callback(
        spp->serial,
        FuriHalBtSerialEventRxData,
        spp_serial_rx_callback,
        spp
    );

    // Start serial profile
    if(!furi_hal_bt_serial_start(spp->serial)) {
        FURI_LOG_E(TAG, "Failed to start SPP");
        return false;
    }

    spp->connected = true;
    FURI_LOG_I(TAG, "SPP started");
    return true;
}

void spp_serial_stop(SPPSerial* spp) {
    furi_assert(spp);

    if(!spp->connected) {
        return;
    }

    furi_hal_bt_serial_stop(spp->serial);
    spp->connected = false;

    FURI_LOG_I(TAG, "SPP stopped");
}

size_t spp_serial_write(SPPSerial* spp, const uint8_t* data, size_t length) {
    furi_assert(spp);
    furi_assert(data);

    if(!spp->connected) {
        FURI_LOG_E(TAG, "SPP not connected");
        return 0;
    }

    size_t sent = furi_hal_bt_serial_tx(spp->serial, data, length);
    FURI_LOG_D(TAG, "SPP TX: %zu/%zu bytes", sent, length);

    return sent;
}

size_t spp_serial_read(SPPSerial* spp, uint8_t* buffer, size_t length) {
    furi_assert(spp);
    furi_assert(buffer);

    size_t received = furi_stream_buffer_receive(spp->rx_stream, buffer, length, 0);

    if(received > 0) {
        FURI_LOG_D(TAG, "SPP RX read: %zu bytes", received);
    }

    return received;
}

void spp_serial_set_callback(SPPSerial* spp, BLEDataReceivedCallback callback, void* context) {
    furi_assert(spp);

    spp->on_data_received = callback;
    spp->callback_context = context;
}

// ============================================================================
// Bluetooth Manager Implementation
// ============================================================================

BTManager* bt_manager_alloc(void) {
    BTManager* manager = malloc(sizeof(BTManager));

    manager->scanner = NULL;
    manager->spp = NULL;
    manager->initialized = false;

    return manager;
}

void bt_manager_free(BTManager* manager) {
    furi_assert(manager);

    if(manager->scanner) {
        ble_scanner_free(manager->scanner);
    }

    if(manager->spp) {
        spp_serial_free(manager->spp);
    }

    free(manager);
}

bool bt_manager_init(BTManager* manager) {
    furi_assert(manager);

    if(manager->initialized) {
        FURI_LOG_W(TAG, "Already initialized");
        return true;
    }

    // Allocate scanner
    manager->scanner = ble_scanner_alloc();
    if(!manager->scanner) {
        FURI_LOG_E(TAG, "Failed to allocate scanner");
        return false;
    }

    // Allocate SPP
    manager->spp = spp_serial_alloc();
    if(!manager->spp) {
        FURI_LOG_E(TAG, "Failed to allocate SPP");
        ble_scanner_free(manager->scanner);
        return false;
    }

    manager->initialized = true;
    FURI_LOG_I(TAG, "Bluetooth Manager initialized");

    return true;
}

bool bt_manager_scan_start(BTManager* manager, BLEDeviceFoundCallback callback, void* context) {
    furi_assert(manager);

    if(!manager->initialized) {
        FURI_LOG_E(TAG, "Not initialized");
        return false;
    }

    return ble_scanner_start(manager->scanner, callback, context);
}

void bt_manager_scan_stop(BTManager* manager) {
    furi_assert(manager);

    if(!manager->initialized) {
        return;
    }

    ble_scanner_stop(manager->scanner);
}

bool bt_manager_connect(BTManager* manager, const char* device_address) {
    furi_assert(manager);
    furi_assert(device_address);

    if(!manager->initialized) {
        FURI_LOG_E(TAG, "Not initialized");
        return false;
    }

    FURI_LOG_I(TAG, "Connecting to %s", device_address);

    // Start SPP serial profile
    if(!spp_serial_start(manager->spp)) {
        return false;
    }

    // TODO: Actual BLE connection logic
    // This would involve pairing and connecting to the specific device

    FURI_LOG_I(TAG, "Connected successfully");
    return true;
}

void bt_manager_disconnect(BTManager* manager) {
    furi_assert(manager);

    if(!manager->initialized) {
        return;
    }

    spp_serial_stop(manager->spp);
    FURI_LOG_I(TAG, "Disconnected");
}

bool bt_manager_is_connected(BTManager* manager) {
    furi_assert(manager);

    if(!manager->initialized || !manager->spp) {
        return false;
    }

    return manager->spp->connected;
}

size_t bt_manager_send(BTManager* manager, const uint8_t* data, size_t length) {
    furi_assert(manager);

    if(!bt_manager_is_connected(manager)) {
        FURI_LOG_E(TAG, "Not connected");
        return 0;
    }

    return spp_serial_write(manager->spp, data, length);
}

size_t bt_manager_receive(BTManager* manager, uint8_t* buffer, size_t length) {
    furi_assert(manager);

    if(!bt_manager_is_connected(manager)) {
        return 0;
    }

    return spp_serial_read(manager->spp, buffer, length);
}
