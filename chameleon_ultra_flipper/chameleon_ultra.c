#include "chameleon_ultra.h"
#include <furi.h>
#include <furi_hal.h>

// ============================================================================
// Scene Handlers (Simplified for POC)
// ============================================================================

void chameleon_scene_on_enter_start(void* context) {
    ChameleonApp* app = context;

    submenu_reset(app->submenu);
    submenu_set_header(app->submenu, "Chameleon Ultra");
    submenu_add_item(app->submenu, "Scan Bluetooth", 0, NULL, app);
    submenu_add_item(app->submenu, "Connect", 1, NULL, app);
    submenu_add_item(app->submenu, "Read LF Tag", 2, NULL, app);
    submenu_add_item(app->submenu, "Read HF Tag", 3, NULL, app);
    submenu_add_item(app->submenu, "Settings", 4, NULL, app);

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewSubmenu);
}

void chameleon_scene_on_exit_start(void* context) {
    ChameleonApp* app = context;
    submenu_reset(app->submenu);
}

// ============================================================================
// View Dispatcher Callbacks
// ============================================================================

bool chameleon_navigation_callback(void* context) {
    ChameleonApp* app = context;
    UNUSED(app);
    return false; // Exit app
}

uint32_t chameleon_previous_callback(void* context) {
    UNUSED(context);
    return VIEW_NONE; // Return to previous view
}

// ============================================================================
// Application Lifecycle
// ============================================================================

ChameleonApp* chameleon_app_alloc() {
    ChameleonApp* app = malloc(sizeof(ChameleonApp));

    // GUI
    app->gui = furi_record_open(RECORD_GUI);
    app->view_dispatcher = view_dispatcher_alloc();
    app->scene_manager = scene_manager_alloc(NULL, app);
    app->notifications = furi_record_open(RECORD_NOTIFICATION);
    app->dialogs = furi_record_open(RECORD_DIALOGS);

    // Modules
    app->submenu = submenu_alloc();
    app->dialog_ex = dialog_ex_alloc();
    app->text_input = text_input_alloc();
    app->popup = popup_alloc();

    // Set up view dispatcher
    view_dispatcher_enable_queue(app->view_dispatcher);
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher,
        chameleon_navigation_callback
    );

    // Add views
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewSubmenu,
        submenu_get_view(app->submenu)
    );
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewDialogEx,
        dialog_ex_get_view(app->dialog_ex)
    );
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewTextInput,
        text_input_get_view(app->text_input)
    );
    view_dispatcher_add_view(
        app->view_dispatcher,
        ChameleonViewPopup,
        popup_get_view(app->popup)
    );

    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);

    // Initialize Bluetooth
    app->bt_manager = bt_manager_alloc();
    if(!bt_manager_init(app->bt_manager)) {
        FURI_LOG_E(TAG, "Failed to initialize Bluetooth");
    }
    app->bt_connected = false;

    // Initialize Chameleon
    app->chameleon = NULL;
    app->active_slot = 0;
    app->chameleon_ready = false;

    FURI_LOG_I(TAG, "Chameleon Ultra app allocated");

    return app;
}

void chameleon_app_free(ChameleonApp* app) {
    furi_assert(app);

    // Free Chameleon
    if(app->chameleon) {
        chameleon_device_free(app->chameleon);
    }

    // Free Bluetooth
    if(app->bt_manager) {
        bt_manager_free(app->bt_manager);
    }

    // Remove views
    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewSubmenu);
    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewDialogEx);
    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewTextInput);
    view_dispatcher_remove_view(app->view_dispatcher, ChameleonViewPopup);

    // Free modules
    submenu_free(app->submenu);
    dialog_ex_free(app->dialog_ex);
    text_input_free(app->text_input);
    popup_free(app->popup);

    // Free GUI
    view_dispatcher_free(app->view_dispatcher);
    scene_manager_free(app->scene_manager);

    // Close records
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_GUI);

    free(app);

    FURI_LOG_I(TAG, "Chameleon Ultra app freed");
}

// ============================================================================
// Main Entry Point
// ============================================================================

int32_t chameleon_ultra_app(void* p) {
    UNUSED(p);

    FURI_LOG_I(TAG, "Chameleon Ultra app starting...");

    ChameleonApp* app = chameleon_app_alloc();

    // Show welcome dialog
    dialog_ex_set_header(app->dialog_ex, "Chameleon Ultra", 64, 10, AlignCenter, AlignTop);
    dialog_ex_set_text(
        app->dialog_ex,
        "Bluetooth connector for\nChameleon Ultra V2\n\nPress OK to continue",
        64,
        32,
        AlignCenter,
        AlignTop
    );
    dialog_ex_set_left_button_text(app->dialog_ex, "Exit");
    dialog_ex_set_right_button_text(app->dialog_ex, "OK");

    view_dispatcher_switch_to_view(app->view_dispatcher, ChameleonViewDialogEx);

    // Wait for user input
    furi_delay_ms(100);

    // Switch to main menu
    chameleon_scene_on_enter_start(app);

    // Run view dispatcher
    view_dispatcher_run(app->view_dispatcher);

    // Cleanup
    chameleon_app_free(app);

    FURI_LOG_I(TAG, "Chameleon Ultra app stopped");

    return 0;
}
