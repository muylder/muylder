#include "protocol.h"
#include <string.h>
#include <furi.h>

#define TAG "ChameleonProtocol"

// ============================================================================
// Protocol Helper Functions
// ============================================================================

uint16_t chameleon_calc_checksum(const uint8_t* data, size_t length) {
    uint16_t checksum = 0;
    for(size_t i = 0; i < length; i++) {
        checksum += data[i];
    }
    return checksum;
}

size_t chameleon_create_packet(
    uint8_t* buffer,
    size_t buffer_size,
    ChameleonCommand cmd,
    const uint8_t* payload,
    size_t payload_length
) {
    // Check buffer size
    size_t required_size = sizeof(ChameleonPacket) + payload_length + 2; // +2 for checksum
    if(buffer_size < required_size) {
        FURI_LOG_E(TAG, "Buffer too small: need %zu, have %zu", required_size, buffer_size);
        return 0;
    }

    ChameleonPacket* packet = (ChameleonPacket*)buffer;
    packet->command = cmd;
    packet->status = 0;
    packet->length = payload_length;

    // Copy payload
    if(payload && payload_length > 0) {
        memcpy(packet->data, payload, payload_length);
    }

    // Calculate and append checksum
    size_t packet_length = sizeof(ChameleonPacket) + payload_length;
    uint16_t checksum = chameleon_calc_checksum(buffer, packet_length);
    memcpy(buffer + packet_length, &checksum, 2);

    FURI_LOG_D(TAG, "Created packet: cmd=0x%04X, len=%u, checksum=0x%04X",
        cmd, payload_length, checksum);

    return required_size;
}

bool chameleon_parse_response(
    const uint8_t* buffer,
    size_t length,
    ChameleonPacket** out_packet
) {
    // Minimum packet size: header + checksum
    if(length < sizeof(ChameleonPacket) + 2) {
        FURI_LOG_E(TAG, "Packet too small: %zu bytes", length);
        return false;
    }

    ChameleonPacket* packet = (ChameleonPacket*)buffer;
    size_t expected_length = sizeof(ChameleonPacket) + packet->length + 2;

    if(length < expected_length) {
        FURI_LOG_E(TAG, "Incomplete packet: expected %zu, got %zu", expected_length, length);
        return false;
    }

    // Verify checksum
    size_t packet_length = sizeof(ChameleonPacket) + packet->length;
    uint16_t expected_checksum = chameleon_calc_checksum(buffer, packet_length);
    uint16_t received_checksum;
    memcpy(&received_checksum, buffer + packet_length, 2);

    if(expected_checksum != received_checksum) {
        FURI_LOG_E(TAG, "Checksum mismatch: expected 0x%04X, got 0x%04X",
            expected_checksum, received_checksum);
        return false;
    }

    *out_packet = packet;
    FURI_LOG_D(TAG, "Parsed response: cmd=0x%04X, status=0x%04X, len=%u",
        packet->command, packet->status, packet->length);

    return true;
}

// ============================================================================
// Device Management
// ============================================================================

ChameleonDevice* chameleon_device_alloc(SPPSerial* serial) {
    ChameleonDevice* device = malloc(sizeof(ChameleonDevice));

    device->serial = serial;
    device->timeout_ms = 5000; // 5 second timeout
    device->last_response = NULL;
    device->active_slot = 0;
    device->connected = false;

    memset(&device->device_info, 0, sizeof(ChameleonDeviceInfo));
    memset(device->slots, 0, sizeof(device->slots));

    return device;
}

void chameleon_device_free(ChameleonDevice* device) {
    furi_assert(device);

    if(device->last_response) {
        free(device->last_response);
    }

    free(device);
}

bool chameleon_device_connect(ChameleonDevice* device) {
    furi_assert(device);

    // Try to get version to verify connection
    char version[16];
    if(chameleon_get_version(device, version, sizeof(version))) {
        device->connected = true;
        FURI_LOG_I(TAG, "Connected to Chameleon: %s", version);
        return true;
    }

    FURI_LOG_E(TAG, "Failed to connect to Chameleon");
    return false;
}

void chameleon_device_disconnect(ChameleonDevice* device) {
    furi_assert(device);
    device->connected = false;
    FURI_LOG_I(TAG, "Disconnected from Chameleon");
}

// ============================================================================
// Command Execution
// ============================================================================

bool chameleon_send_command(
    ChameleonDevice* device,
    ChameleonCommand cmd,
    const uint8_t* payload,
    size_t payload_length,
    ChameleonPacket** response
) {
    furi_assert(device);
    furi_assert(device->serial);

    uint8_t tx_buffer[512];
    uint8_t rx_buffer[512];

    // Create packet
    size_t packet_size = chameleon_create_packet(
        tx_buffer,
        sizeof(tx_buffer),
        cmd,
        payload,
        payload_length
    );

    if(packet_size == 0) {
        FURI_LOG_E(TAG, "Failed to create packet");
        return false;
    }

    // Send via Bluetooth
    size_t sent = spp_serial_write(device->serial, tx_buffer, packet_size);
    if(sent != packet_size) {
        FURI_LOG_E(TAG, "Failed to send: sent %zu/%zu bytes", sent, packet_size);
        return false;
    }

    FURI_LOG_D(TAG, "Sent command 0x%04X (%zu bytes)", cmd, packet_size);

    // Wait for response
    uint32_t start_time = furi_get_tick();
    size_t received = 0;

    while((furi_get_tick() - start_time) < device->timeout_ms) {
        size_t available = spp_serial_read(
            device->serial,
            rx_buffer + received,
            sizeof(rx_buffer) - received
        );

        if(available > 0) {
            received += available;

            // Try to parse response
            if(chameleon_parse_response(rx_buffer, received, response)) {
                FURI_LOG_I(TAG, "Received response for 0x%04X (status=0x%04X)",
                    cmd, (*response)->status);
                return true;
            }
        }

        furi_delay_ms(10);
    }

    FURI_LOG_E(TAG, "Timeout waiting for response (waited %lu ms)", device->timeout_ms);
    return false;
}

// ============================================================================
// High-Level Commands
// ============================================================================

bool chameleon_get_version(ChameleonDevice* device, char* version, size_t version_size) {
    furi_assert(device);
    furi_assert(version);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_GET_VERSION, NULL, 0, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        FURI_LOG_E(TAG, "Get version failed: status=0x%04X", response->status);
        return false;
    }

    // Copy version string
    size_t copy_len = response->length < version_size ? response->length : version_size - 1;
    memcpy(version, response->data, copy_len);
    version[copy_len] = '\0';

    // Store in device info
    snprintf(device->device_info.version, sizeof(device->device_info.version), "%s", version);

    return true;
}

bool chameleon_get_device_info(ChameleonDevice* device) {
    furi_assert(device);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_GET_DEVICE_INFO, NULL, 0, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        return false;
    }

    // Parse device info (structure depends on Chameleon firmware)
    // This is a simplified version
    if(response->length >= sizeof(ChameleonDeviceInfo)) {
        memcpy(&device->device_info, response->data, sizeof(ChameleonDeviceInfo));
        return true;
    }

    return false;
}

bool chameleon_set_active_slot(ChameleonDevice* device, uint8_t slot_number, bool is_lf) {
    furi_assert(device);

    if(slot_number >= 8) {
        FURI_LOG_E(TAG, "Invalid slot number: %u (max 7)", slot_number);
        return false;
    }

    // Adjust slot number based on LF/HF
    // LF slots: 0-7, HF slots: 8-15 (internal representation)
    uint8_t actual_slot = is_lf ? slot_number : (slot_number + 8);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_SET_ACTIVE_SLOT, &actual_slot, 1, &response)) {
        return false;
    }

    if(response->status == STATUS_SUCCESS) {
        device->active_slot = actual_slot;
        FURI_LOG_I(TAG, "Active slot set to: %u (%s)", slot_number, is_lf ? "LF" : "HF");
        return true;
    }

    return false;
}

bool chameleon_get_slot_info(ChameleonDevice* device, uint8_t slot_number, ChameleonSlot* slot) {
    furi_assert(device);
    furi_assert(slot);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_GET_SLOT_INFO, &slot_number, 1, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        return false;
    }

    // Parse slot info
    if(response->length >= sizeof(ChameleonSlot)) {
        memcpy(slot, response->data, sizeof(ChameleonSlot));
        return true;
    }

    return false;
}

// ============================================================================
// LF Operations
// ============================================================================

bool chameleon_lf_read_em410x(ChameleonDevice* device, uint64_t* uid) {
    furi_assert(device);
    furi_assert(uid);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_LF_READ_EM410X, NULL, 0, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        if(response->status == STATUS_NO_TAG_FOUND) {
            FURI_LOG_W(TAG, "No tag found");
        }
        return false;
    }

    // EM410X UID is 5 bytes
    if(response->length != 5) {
        FURI_LOG_E(TAG, "Invalid UID length: %u (expected 5)", response->length);
        return false;
    }

    // Convert bytes to uint64_t
    *uid = 0;
    for(int i = 0; i < 5; i++) {
        *uid = (*uid << 8) | response->data[i];
    }

    FURI_LOG_I(TAG, "EM410X UID: %010llX", *uid);
    return true;
}

bool chameleon_lf_write_em410x(ChameleonDevice* device, uint64_t uid) {
    furi_assert(device);

    // Convert UID to 5 bytes
    uint8_t uid_bytes[5];
    for(int i = 4; i >= 0; i--) {
        uid_bytes[i] = uid & 0xFF;
        uid >>= 8;
    }

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_LF_WRITE_EM410X, uid_bytes, 5, &response)) {
        return false;
    }

    if(response->status == STATUS_SUCCESS) {
        FURI_LOG_I(TAG, "EM410X written: %010llX", uid);
        return true;
    }

    return false;
}

bool chameleon_lf_detect(ChameleonDevice* device, ChameleonTagType* tag_type) {
    furi_assert(device);
    furi_assert(tag_type);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_LF_DETECT, NULL, 0, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        return false;
    }

    if(response->length >= 1) {
        *tag_type = (ChameleonTagType)response->data[0];
        FURI_LOG_I(TAG, "Detected LF tag: %s", chameleon_tag_type_to_string(*tag_type));
        return true;
    }

    return false;
}

// ============================================================================
// HF Operations
// ============================================================================

bool chameleon_hf_detect_14a(ChameleonDevice* device, uint8_t* uid, size_t* uid_length) {
    furi_assert(device);
    furi_assert(uid);
    furi_assert(uid_length);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_HF_DETECT_14A, NULL, 0, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        return false;
    }

    // UID can be 4, 7, or 10 bytes
    if(response->length >= 4 && response->length <= 10) {
        memcpy(uid, response->data, response->length);
        *uid_length = response->length;
        FURI_LOG_I(TAG, "Detected HF 14A tag, UID length: %u", response->length);
        return true;
    }

    return false;
}

bool chameleon_hf_read_mifare(
    ChameleonDevice* device,
    uint8_t block_number,
    const uint8_t* key,
    uint8_t key_type,
    uint8_t* block_data
) {
    furi_assert(device);
    furi_assert(key);
    furi_assert(block_data);

    // Prepare payload: block_number + key_type + key (6 bytes)
    uint8_t payload[8];
    payload[0] = block_number;
    payload[1] = key_type; // 0x60 = Key A, 0x61 = Key B
    memcpy(payload + 2, key, 6);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_MF_READ_BLOCK, payload, 8, &response)) {
        return false;
    }

    if(response->status != STATUS_SUCCESS) {
        if(response->status == STATUS_AUTH_FAILED) {
            FURI_LOG_W(TAG, "Authentication failed");
        }
        return false;
    }

    // Mifare block is 16 bytes
    if(response->length != 16) {
        FURI_LOG_E(TAG, "Invalid block data length: %u (expected 16)", response->length);
        return false;
    }

    memcpy(block_data, response->data, 16);
    FURI_LOG_I(TAG, "Read Mifare block %u", block_number);

    return true;
}

bool chameleon_hf_write_mifare(
    ChameleonDevice* device,
    uint8_t block_number,
    const uint8_t* key,
    uint8_t key_type,
    const uint8_t* block_data
) {
    furi_assert(device);
    furi_assert(key);
    furi_assert(block_data);

    // Prepare payload: block_number + key_type + key (6 bytes) + data (16 bytes)
    uint8_t payload[24];
    payload[0] = block_number;
    payload[1] = key_type;
    memcpy(payload + 2, key, 6);
    memcpy(payload + 8, block_data, 16);

    ChameleonPacket* response;

    if(!chameleon_send_command(device, CMD_MF_WRITE_BLOCK, payload, 24, &response)) {
        return false;
    }

    if(response->status == STATUS_SUCCESS) {
        FURI_LOG_I(TAG, "Wrote Mifare block %u", block_number);
        return true;
    }

    return false;
}

// ============================================================================
// Utility Functions
// ============================================================================

const char* chameleon_status_to_string(ChameleonStatus status) {
    switch(status) {
        case STATUS_SUCCESS: return "Success";
        case STATUS_INVALID_CMD: return "Invalid Command";
        case STATUS_INVALID_PARAM: return "Invalid Parameter";
        case STATUS_DEVICE_MODE_ERROR: return "Device Mode Error";
        case STATUS_NOT_IMPLEMENTED: return "Not Implemented";
        case STATUS_FLASH_ERROR: return "Flash Error";
        case STATUS_NO_TAG_FOUND: return "No Tag Found";
        case STATUS_AUTH_FAILED: return "Authentication Failed";
        case STATUS_TIMEOUT: return "Timeout";
        default: return "Unknown Status";
    }
}

const char* chameleon_tag_type_to_string(ChameleonTagType tag_type) {
    switch(tag_type) {
        case TAG_TYPE_EM410X: return "EM410X";
        case TAG_TYPE_T55XX: return "T55XX";
        case TAG_TYPE_HID_PROX: return "HID Prox";
        case TAG_TYPE_MIFARE_CLASSIC_1K: return "Mifare Classic 1K";
        case TAG_TYPE_MIFARE_CLASSIC_4K: return "Mifare Classic 4K";
        case TAG_TYPE_MIFARE_ULTRALIGHT: return "Mifare Ultralight";
        case TAG_TYPE_NTAG: return "NTAG";
        case TAG_TYPE_UNKNOWN:
        default: return "Unknown";
    }
}
