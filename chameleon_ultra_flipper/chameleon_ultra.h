#pragma once

#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/text_input.h>
#include <gui/modules/popup.h>
#include <notification/notification_messages.h>
#include <dialogs/dialogs.h>
#include <storage/storage.h>

#include "bluetooth/bt_manager.h"
#include "chameleon/protocol.h"

#define TAG "ChameleonUltra"

// App scenes
typedef enum {
    ChameleonSceneStart,
    ChameleonSceneBTScan,
    ChameleonSceneBTConnect,
    ChameleonSceneMain,
    ChameleonSceneSlotSelect,
    ChameleonSceneRFIDRead,
    ChameleonSceneRFIDWrite,
    ChameleonSceneSettings,
    ChameleonSceneCount,
} ChameleonScene;

// App views
typedef enum {
    ChameleonViewSubmenu,
    ChameleonViewDialogEx,
    ChameleonViewTextInput,
    ChameleonViewPopup,
    ChameleonViewMain,
    ChameleonViewBTScan,
} ChameleonView;

// Main application context
typedef struct {
    // GUI
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    NotificationApp* notifications;
    DialogsApp* dialogs;

    // Modules
    Submenu* submenu;
    DialogEx* dialog_ex;
    TextInput* text_input;
    Popup* popup;

    // Bluetooth
    BTManager* bt_manager;
    bool bt_connected;
    char bt_device_name[32];

    // Chameleon
    ChameleonDevice* chameleon;
    uint8_t active_slot;
    bool chameleon_ready;
    char chameleon_version[16];

    // State
    char text_store[128];

} ChameleonApp;

// App lifecycle
ChameleonApp* chameleon_app_alloc();
void chameleon_app_free(ChameleonApp* app);
int32_t chameleon_ultra_app(void* p);
