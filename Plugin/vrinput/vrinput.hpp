#include "openvr.h"

bool InitializeOpenVR() {
    // Initialize OpenVR system
    vr::EVRInitError error = vr::VRInitError_None;
    auto g_pOpenVRSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None) {
        // Failed to initialize OpenVR system
        return false;
    } else {
        vr::VREvent_t event;
        while (g_pOpenVRSystem->PollNextEvent(&event, sizeof(event))) {
            // Check if the event is a button press event
            if (event.eventType == vr::VREvent_ButtonPress) {
                // Get the controller state for the device that generated the event
                vr::VRControllerState_t state;
                if (g_pOpenVRSystem->GetControllerState(event.trackedDeviceIndex, &state, sizeof(vr::VRControllerState_t))) {
                    
                    // Check if the button that was pressed is the trigger button
                    if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) {
                        // Trigger button was pressed, do something
                        // ...

                        RE::DebugNotification("VR trigger button pressed!!");
                    }

                    //// Check if any button was pressed
                    //if (g_pOpenVRSystem->GetControllerState(event.trackedDeviceIndex, &state, sizeof(vr::VRControllerState_t))) {
                    //    for (vr::EVRButtonId button = vr::k_EButton_System; button <= vr::k_EButton_Max; button = (vr::EVRButtonId)(button + 1)) {
                    //        if (state.ulButtonPressed & vr::ButtonMaskFromId(button)) {
                    //            // Button was pressed, do something
                    //            // ...
                    //        }
                    //    }
                    //}
                }
            }
        }
    }
    return true;
}

class MyOpenVRSystem : public vr::IVRSystem {
public:
    virtual bool GetControllerState(vr::TrackedDeviceIndex_t unControllerDeviceIndex, vr::VRControllerState_t* pControllerState, uint32_t unControllerStateSize) override {
        // Get the controller state from OpenVR
        vr::VRControllerState_t state;
        bool bResult = vr::VRSystem()->GetControllerState(unControllerDeviceIndex, &state, unControllerStateSize);

        // If the controller state was obtained, copy it to the provided state pointer
        if (bResult) {
            memcpy(pControllerState, &state, sizeof(vr::VRControllerState_t));
        }

        return bResult;
    }
};




//#include "openvr.h"
//
//bool InitializeOpenVR() {
//    // Initialize OpenVR system
//    vr::EVRInitError error = vr::VRInitError_None;
//    auto g_pOpenVRSystem = vr::VR_Init(&error, vr::VRApplication_Scene);
//
//    if (error != vr::VRInitError_None) {
//        // Failed to initialize OpenVR system
//        return false;
//    }
//    else
//    {
//        vr::VREvent_t event;
//        while (g_pOpenVRSystem->PollNextEvent(&event, sizeof(event))) {
//            // Check if the event is a button press event
//            if (event.eventType == vr::VREvent_ButtonPress) {
//                // Get the controller state for the device that generated the event
//                vr::VRControllerState_t state;
//                if (g_pOpenVRSystem->GetControllerState(event.trackedDeviceIndex, &state)) {
//                    // Check if the button that was pressed is the trigger button
//                    if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) {
//                        // Trigger button was pressed, do something
//                        // ...
//                    }
//                }
//            }
//        }
//    }
//    return true;
//}
//
//class MyOpenVRSystem : public vr::IVRSystem {
//public:
//    
//    virtual bool PollNextEvent(vr::VREvent_t* pEvent, uint32_t uncbVREvent) override {
//        // Poll the OpenVR event queue for the next event
//        vr::VREvent_t event;
//        bool bResult = vr::VRSystem()->PollNextEvent(&event, uncbVREvent);
//
//        // If an event was found, copy it to the provided event pointer
//        if (bResult) {
//            memcpy(pEvent, &event, sizeof(vr::VREvent_t));
//        }
//
//        return bResult;
//    }
//
//    virtual bool GetControllerState(vr::TrackedDeviceIndex_t unControllerDeviceIndex, vr::VRControllerState_t* pControllerState) {
//        // Get the controller state from OpenVR
//        vr::VRControllerState_t state;
//        bool bResult = vr::VRSystem()->GetControllerState(unControllerDeviceIndex, &state);
//
//        // If the controller state was obtained, copy it to the provided state pointer
//        if (bResult) {
//            memcpy(pControllerState, &state, sizeof(vr::VRControllerState_t));
//        }
//
//        return bResult;
//    }
//};


// void TrackVRButtons() {
//     vr::VREvent_t event;
//     while (g_pOpenVRSystem->PollNextEvent(&event, sizeof(event))) {
//         // Check if the event is a button press event
//         if (event.eventType == vr::VREvent_ButtonPress) {
//             // Get the controller state for the device that generated the event
//             vr::VRControllerState_t state;
//             if (g_pOpenVRSystem->GetControllerState(event.trackedDeviceIndex, &state)) {
//                 // Check if the button that was pressed is the trigger button
//                 if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) {
//                     // Trigger button was pressed, do something
//                     // ...
//                 }
//             }
//         }
//     }
// }