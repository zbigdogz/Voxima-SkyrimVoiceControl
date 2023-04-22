#include "../vrapi/PapyrusVRAPI.hpp"  // Miscellaneous custom functions

PapyrusVRAPI* g_papyrusvr = nullptr;
vr::IVRSystem* g_VRSystem = nullptr;  // only set by new RAW api from Hook Mgr

void OnPapyrusVRMessage(SKSE::MessagingInterface::Message* message) { 
	if (message) {
        if (message->type == kPapyrusVR_Message_Init && message->data) {
            logger::debug("PapyrusVR Init Message received with valid data, waiting for init.");
            g_papyrusvr = (PapyrusVRAPI*)message->data;
		}
	}
}

std::string GetButtonName(vr::EVRButtonId buttonId) {
    std::string name = "";
    switch (buttonId) {
        case vr::k_EButton_System:
            name = "System button was pressed";
            break;
        case vr::k_EButton_ApplicationMenu:
            name = "Application menu button was pressed";
            break;
        case vr::k_EButton_Grip:
            name = "Grip button was pressed";
            break;
        case vr::k_EButton_DPad_Up:
            name = "DPad up button was pressed";
            break;
        case vr::k_EButton_DPad_Down:
            name = "DPad down button was pressed";
            break;
        case vr::k_EButton_DPad_Left:
            name = "DPad left button was pressed";
            break;
        case vr::k_EButton_DPad_Right:
            name = "DPad right button was pressed";
            break;
        case vr::k_EButton_A:
            name = "A button was pressed";
            break;
        case vr::k_EButton_ProximitySensor:
            name = "Proximity sensor button was pressed";
            break;
        case vr::k_EButton_Axis0:
            name = "Touchpad button was pressed";
            break;
        case vr::k_EButton_Axis1:
            name = "Trigger button was pressed";
            break;
        case vr::k_EButton_Axis2:
            name = "Thumbrest button was pressed";
            break;
        case vr::k_EButton_Axis3:
            name = "Thumbstick button was pressed";
            break;
        case vr::k_EButton_Axis4:
            name = "Touchpad button was pressed";
            break;
        default:
            name = "Unknown button was pressed";
            break;
    }
    return name;
}

// New RAW API event handlers
bool OnControllerStateChanged(vr::TrackedDeviceIndex_t unControllerDeviceIndex, const vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize,
                              vr::VRControllerState_t* pOutputControllerState) {
    //static uint64_t lastButtonPressedData[PapyrusVR::VRDevice_LeftController + 1] = {0};  // should be size 3 array at least for all 3 vr devices
    //static_assert(PapyrusVR::VRDevice_LeftController + 1 >= 3, "lastButtonPressedData array size too small!");

    //if (!g_quickslotMgr->IsTrackingDataValid()) {
    //    return false;
    //}

    logger::debug("VR controller state changed!!");

    vr::TrackedDeviceIndex_t leftcontroller = g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_LeftHand);
    vr::TrackedDeviceIndex_t rightcontroller = g_VRSystem->GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole::TrackedControllerRole_RightHand);

    // NOTE: DO NOT check the packetNum on ControllerState, it seems to cause problems (maybe it should only be checked per controllers?) - at any rate, it seems to change every
    // frame anyway.

    PapyrusVR::VRDevice deviceId = PapyrusVR::VRDevice_Unknown;

    if (unControllerDeviceIndex == leftcontroller) {
        deviceId = PapyrusVR::VRDevice_LeftController;
    } else if (unControllerDeviceIndex == rightcontroller) {
        deviceId = PapyrusVR::VRDevice_RightController;
    }

    if (deviceId != PapyrusVR::VRDevice_Unknown) {
        
        RE::DebugNotification("VR button pressed!!");

        //if (pControllerState->ulButtonPressed & PapyrusVR::EVRButtonId::k_EButton_SteamVR_Trigger) {



        //}

        for (vr::EVRButtonId button = vr::k_EButton_System; button <= vr::k_EButton_Max; button = (vr::EVRButtonId)(button + 1)) {
            if (pControllerState->ulButtonPressed & vr::ButtonMaskFromId(button)) {
                // Button was pressed, do something
                // ...

                RE::DebugNotification(GetButtonName(button).c_str());
            }
        }
        
        //// We get the activate button from config now so, getting it by function.
        //const auto buttonId = g_quickslotMgr->GetActivateButton();
        //const uint64_t buttonMask = vr::ButtonMaskFromId((vr::EVRButtonId)buttonId);  // annoying issue where PapyrusVR and openvr enums are not type-compatible..

        //// right now only check for trigger press.  In future support input binding?
        //if (pControllerState->ulButtonPressed & buttonMask && !(lastButtonPressedData[deviceId] & buttonMask)) {
        //    //bool retVal = g_quickslotMgr->ButtonPress(buttonId, deviceId);

        //    //if (retVal)  // mask out input if we touched a quickslot (block the game from receiving it)
        //    //{
        //    //    pOutputControllerState->ulButtonPressed &= ~buttonMask;
        //    //}

        //    logger::debug("Trigger pressed for deviceIndex: %d deviceId: %d", unControllerDeviceIndex, deviceId);
        //} else if (!(pControllerState->ulButtonPressed & buttonMask) && (lastButtonPressedData[deviceId] & buttonMask)) {
        //    ///g_quickslotMgr->ButtonRelease(buttonId, deviceId);

        //    logger::debug("Trigger released for deviceIndex: %d deviceId: %d", unControllerDeviceIndex, deviceId);
        //}

        //// we need to block all inputs when button is held over top of a quickslot (check last button press and if controller is hovering over a quickslot)
        //if (lastButtonPressedData[deviceId] & buttonMask && g_quickslotMgr->FindQuickslotByDeviceId(deviceId)) {
        //    pOutputControllerState->ulButtonPressed &= ~buttonMask;
        //}

        //lastButtonPressedData[deviceId] = pControllerState->ulButtonPressed;
    }

    return true;
}

// Legacy API event handlers
void OnVRButtonEvent(PapyrusVR::VREventType type, PapyrusVR::EVRButtonId buttonId, PapyrusVR::VRDevice deviceId) {
    // Use button presses here
    if (type == PapyrusVR::VREventType_Pressed) {
        logger::debug("VR Button press deviceId: %d buttonId: %d", deviceId, buttonId);
        //g_quickslotMgr->ButtonPress(buttonId, deviceId);

    } else if (type == PapyrusVR::VREventType_Released) {
        logger::debug("VR Button press deviceId: %d buttonId: %d", deviceId, buttonId);
        //g_quickslotMgr->ButtonRelease(buttonId, deviceId);

    }
}