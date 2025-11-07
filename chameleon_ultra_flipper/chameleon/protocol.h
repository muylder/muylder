#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../bluetooth/bt_manager.h"

// Chameleon Ultra V2 Command Definitions
typedef enum {
    // Device Information
    CMD_GET_VERSION = 0x1000,
    CMD_GET_DEVICE_INFO = 0x1001,
    CMD_GET_DEVICE_CHIP_ID = 0x1002,

    // Slot Management
    CMD_GET_ACTIVE_SLOT = 0x1003,
    CMD_SET_ACTIVE_SLOT = 0x1004,
    CMD_GET_SLOT_INFO = 0x1005,
    CMD_SET_SLOT_DATA = 0x1006,
    CMD_DELETE_SLOT_DATA = 0x1007,
    CMD_GET_SLOT_TAG_TYPE = 0x1008,
    CMD_SET_SLOT_TAG_TYPE = 0x1009,

    // LF (125kHz) Operations
    CMD_LF_READ_EM410X = 0x2000,
    CMD_LF_WRITE_EM410X = 0x2001,
    CMD_LF_READ_T55XX = 0x2002,
    CMD_LF_WRITE_T55XX = 0x2003,
    CMD_LF_SNIFF = 0x2004,
    CMD_LF_DETECT = 0x2005,

    // HF (13.56MHz) Operations
    CMD_HF_READ_14A = 0x3000,
    CMD_HF_WRITE_14A = 0x3001,
    CMD_HF_DETECT_14A = 0x3002,
    CMD_HF_SNIFF_14A = 0x3003,

    // Mifare Classic Operations
    CMD_MF_CHECK_KEYS = 0x3100,
    CMD_MF_READ_BLOCK = 0x3101,
    CMD_MF_WRITE_BLOCK = 0x3102,
    CMD_MF_NESTED_ATTACK = 0x3103,
    CMD_MF_DARKSIDE_ATTACK = 0x3104,
    CMD_MF_READ_SECTOR = 0x3105,
    CMD_MF_WRITE_SECTOR = 0x3106,

    // Settings
    CMD_SET_LED = 0x4000,
    CMD_SET_BUZZER = 0x4001,
    CMD_SET_BATTERY_MODE = 0x4002,
    CMD_GET_BATTERY_INFO = 0x4003,

} ChameleonCommand;

// Status codes
typedef enum {
    STATUS_SUCCESS = 0x0000,
    STATUS_INVALID_CMD = 0x0001,
    STATUS_INVALID_PARAM = 0x0002,
    STATUS_DEVICE_MODE_ERROR = 0x0003,
    STATUS_NOT_IMPLEMENTED = 0x0004,
    STATUS_FLASH_ERROR = 0x0005,
    STATUS_NO_TAG_FOUND = 0x0006,
    STATUS_AUTH_FAILED = 0x0007,
    STATUS_TIMEOUT = 0x0008,
} ChameleonStatus;

// Tag types
typedef enum {
    TAG_TYPE_UNKNOWN = 0x00,
    TAG_TYPE_EM410X = 0x01,
    TAG_TYPE_T55XX = 0x02,
    TAG_TYPE_HID_PROX = 0x03,
    TAG_TYPE_MIFARE_CLASSIC_1K = 0x10,
    TAG_TYPE_MIFARE_CLASSIC_4K = 0x11,
    TAG_TYPE_MIFARE_ULTRALIGHT = 0x12,
    TAG_TYPE_NTAG = 0x13,
} ChameleonTagType;

// Packet structure
typedef struct __attribute__((packed)) {
    uint16_t command;
    uint16_t status;
    uint16_t length;
    uint8_t data[];
} ChameleonPacket;

// Slot information
typedef struct {
    uint8_t slot_number;
    ChameleonTagType tag_type;
    bool is_lf; // true = LF, false = HF
    char slot_name[16];
    uint8_t uid[10];
    uint8_t uid_length;
} ChameleonSlot;

// Device information
typedef struct {
    char version[16];
    char hardware[16];
    uint32_t chip_id;
    uint8_t battery_level;
    bool charging;
} ChameleonDeviceInfo;

// Main Chameleon device structure
typedef struct {
    SPPSerial* serial;
    uint32_t timeout_ms;
    ChameleonPacket* last_response;
    ChameleonDeviceInfo device_info;
    ChameleonSlot slots[16]; // 8 LF + 8 HF
    uint8_t active_slot;
    bool connected;
} ChameleonDevice;

// Protocol functions
size_t chameleon_create_packet(
    uint8_t* buffer,
    size_t buffer_size,
    ChameleonCommand cmd,
    const uint8_t* payload,
    size_t payload_length
);

bool chameleon_parse_response(
    const uint8_t* buffer,
    size_t length,
    ChameleonPacket** out_packet
);

uint16_t chameleon_calc_checksum(const uint8_t* data, size_t length);

// Device management
ChameleonDevice* chameleon_device_alloc(SPPSerial* serial);
void chameleon_device_free(ChameleonDevice* device);
bool chameleon_device_connect(ChameleonDevice* device);
void chameleon_device_disconnect(ChameleonDevice* device);

// Command execution
bool chameleon_send_command(
    ChameleonDevice* device,
    ChameleonCommand cmd,
    const uint8_t* payload,
    size_t payload_length,
    ChameleonPacket** response
);

// High-level commands
bool chameleon_get_version(ChameleonDevice* device, char* version, size_t version_size);
bool chameleon_get_device_info(ChameleonDevice* device);
bool chameleon_set_active_slot(ChameleonDevice* device, uint8_t slot_number, bool is_lf);
bool chameleon_get_slot_info(ChameleonDevice* device, uint8_t slot_number, ChameleonSlot* slot);

// LF Operations
bool chameleon_lf_read_em410x(ChameleonDevice* device, uint64_t* uid);
bool chameleon_lf_write_em410x(ChameleonDevice* device, uint64_t uid);
bool chameleon_lf_detect(ChameleonDevice* device, ChameleonTagType* tag_type);

// HF Operations
bool chameleon_hf_detect_14a(ChameleonDevice* device, uint8_t* uid, size_t* uid_length);
bool chameleon_hf_read_mifare(
    ChameleonDevice* device,
    uint8_t block_number,
    const uint8_t* key,
    uint8_t key_type,
    uint8_t* block_data
);
bool chameleon_hf_write_mifare(
    ChameleonDevice* device,
    uint8_t block_number,
    const uint8_t* key,
    uint8_t key_type,
    const uint8_t* block_data
);

// Utility
const char* chameleon_status_to_string(ChameleonStatus status);
const char* chameleon_tag_type_to_string(ChameleonTagType tag_type);
