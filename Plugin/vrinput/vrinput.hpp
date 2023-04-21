#include "openvr.h"

vr::IVRSystem* g_pOpenVRSystem = nullptr;

bool InitializeOpenVR() {
    // Initialize OpenVR system
    vr::EVRInitError error = vr::VRInitError_None;
    g_pOpenVRSystem = vr::VR_Init(&error, vr::VRApplication_Scene);

    if (error != vr::VRInitError_None) {
        // Failed to initialize OpenVR system
        return false;
    } else
        return true;
}