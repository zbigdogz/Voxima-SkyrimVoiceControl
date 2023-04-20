namespace DeviceInputEvent {

    using Event = RE::InputEvent*;
    using EventType = RE::INPUT_EVENT_TYPE;
	using DeviceType = RE::INPUT_DEVICE;

    class DeviceInputHandler : public RE::BSTEventSink<Event> {
        using InputSource = RE::BSTEventSource<Event>;

        public:
        static DeviceInputHandler* GetSingleton() {
            static DeviceInputHandler singleton;
            return &singleton;
        }
        
        static const int kKeyboardOffset = 0;
        static const int kMouseOffset = 256;
        static const int kGamepadOffset = 266;
        static const uint32_t kInvalid = static_cast<std::uint32_t>(-1);
        static enum InputDeviceType { Keyboard, Mouse, Gamepad };

        // Register for device input events
        static void Register() {
            static RE::BSInputDeviceManager* inputDeviceManager = nullptr;
            if (!inputDeviceManager) {
                inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
            }
            if (inputDeviceManager && inputDeviceManager->sinks.size() >= 3) {
                auto mySingle = DeviceInputHandler::GetSingleton();

                inputDeviceManager->AddEventSink(mySingle);
                logger::info("Registered for device input events");
            }
        } 

        // Process device input events
        auto ProcessEvent(const Event* a_event, InputSource* a_eventSource) -> RE::BSEventNotifyControl override {
            /// logger::debug("Device input event!");
            if (a_event) {
                for (auto event = *a_event; event; event = event->next) {
                    if (event->eventType != RE::INPUT_EVENT_TYPE::kButton) continue;
                    InputDeviceType deviceType;
                    auto button = static_cast<RE::ButtonEvent*>(event);
                    if (button->IsDown()) {
                        auto key = button->idCode;
                        switch (button->device.get()) {
                            case DeviceType::kMouse:
                                deviceType = InputDeviceType::Mouse;
                                key += kMouseOffset;
                                break;
                            case DeviceType::kKeyboard:
                                deviceType = InputDeviceType::Keyboard;
                                key += kKeyboardOffset;
                                break;
                            case DeviceType::kGamepad:
                                deviceType = InputDeviceType::Gamepad;
                                key = GetGamepadIndex((RE::BSWin32GamepadDevice::Key)key);
                                break;
                            default:
                                continue;
                        }
                        /*string result = deviceType + " " + std::to_string(key) + " pressed";
                        RE::DebugNotification(result.c_str());*/
                        FlatrimInputDeviceEvent(button, key);
                    }
                }
            }
            return RE::BSEventNotifyControl::kContinue;
        }

        // Calculate equivalent keycode for gamepad button
        static std::uint32_t GetGamepadIndex(RE::BSWin32GamepadDevice::Key a_key) {
            using Key = RE::BSWin32GamepadDevice::Key;

            std::uint32_t index;
            switch (a_key) {
                case Key::kUp:
                    index = 0;
                    break;
                case Key::kDown:
                    index = 1;
                    break;
                case Key::kLeft:
                    index = 2;
                    break;
                case Key::kRight:
                    index = 3;
                    break;
                case Key::kStart:
                    index = 4;
                    break;
                case Key::kBack:
                    index = 5;
                    break;
                case Key::kLeftThumb:
                    index = 6;
                    break;
                case Key::kRightThumb:
                    index = 7;
                    break;
                case Key::kLeftShoulder:
                    index = 8;
                    break;
                case Key::kRightShoulder:
                    index = 9;
                    break;
                case Key::kA:
                    index = 10;
                    break;
                case Key::kB:
                    index = 11;
                    break;
                case Key::kX:
                    index = 12;
                    break;
                case Key::kY:
                    index = 13;
                    break;
                case Key::kLeftTrigger:
                    index = 14;
                    break;
                case Key::kRightTrigger:
                    index = 15;
                    break;
                default:
                    index = kInvalid;
                    break;
            }
            return index != kInvalid ? index + kGamepadOffset : kInvalid;
        }

        // Respond to input device events
        static void FlatrimInputDeviceEvent(RE::ButtonEvent* button, uint32_t keyCode);
        
    protected:
        DeviceInputHandler() = default;
        DeviceInputHandler(const DeviceInputHandler&) = delete;
        DeviceInputHandler(DeviceInputHandler&&) = delete;
        virtual ~DeviceInputHandler() = default;

        auto operator=(const DeviceInputHandler&) -> DeviceInputHandler& = delete;
        auto operator=(DeviceInputHandler&&) -> DeviceInputHandler& = delete;
    };
}

void InitializeDeviceInputHooking() { DeviceInputEvent::DeviceInputHandler::Register(); }

#pragma region How to use

/*

Call InitializeDeviceInputHooking and then employ FlatrimInputDeviceEvent where you want to process captured device input events

// Executes when device input events are received
void DeviceInputEvent::EventHandler::FlatrimInputDeviceEvent(RE::ButtonEvent* button, uint32_t keyCode) {
    logger::debug("device input received!");
    // do other stuff
}

*/

#pragma endregion

#pragma region Source Credit

// OnDeviceInputEvent tracking functionality modified from source by Noah Boddie (shared directly via Discord)
// Additional OnDeviceInputEvent tracking code modified from "Precision" by ersh1 (MIT License)
    // Links ==> https://github.com/ersh1/Precision; https://www.nexusmods.com/skyrimspecialedition/mods/72347

#pragma endregion