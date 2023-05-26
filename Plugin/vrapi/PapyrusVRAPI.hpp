#pragma once
#include <functional>
#include <vector>

#include "PapyrusVRTypes.hpp"
#include "VRHookAPI.hpp"
#include "VRManagerAPI.hpp"

static const uint32_t kPapyrusVR_Message_Init = 40008;

struct PapyrusVRAPI
{
    // Functions
    // std::function<void(OnPoseUpdateCallback)> RegisterPoseUpdateListener; Discontinued
    std::function<PapyrusVR::VRManagerAPI*(void)> GetVRManager;
    std::function<OpenVRHookManagerAPI*(void)> GetOpenVRHook;
};