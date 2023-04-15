#include <cmath>

namespace GamepadInputEvent {

    using Event = RE::InputEvent*;

    ///static bool isWatching = false;

    class GamepadInputHandler : public RE::BSTEventSink<Event> {
        using InputSink = RE::BSTEventSink<Event>;
        using InputSource = RE::BSTEventSource<Event>;
        // using InputSource = RE::BSTEventSource<RE::InputEvent>;

        public:
        static GamepadInputHandler* GetSingleton() {
            static GamepadInputHandler singleton;
            return &singleton;
        }

        static void Register() {
            ///logger::info("This task was implemented with a free function!");

            ///if (isWatching) return;

            /*
            auto sourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            if (sourceHolder) {
                sourceHolder->AddEventSink(MagicEffectApplyEventHandler::GetSingleton());
                isWatching = true;
            }

            return;
            //*/

            // auto inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
            static RE::BSInputDeviceManager* inputDeviceManager = nullptr;
            if (!inputDeviceManager) {
                inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
            }

            if (inputDeviceManager && inputDeviceManager->sinks.size() >= 3) {
                auto mySingle = GamepadInputHandler::GetSingleton();

                inputDeviceManager->AddEventSink(mySingle);
                ///isWatching = true;
                logger::info("Registered for gamepad input events");
            }
        } 

        auto ProcessEvent(const Event* a_event, InputSource* a_eventSource) -> RE::BSEventNotifyControl override {
            if (a_event) {
                /// logger::debug("Gamepad input event!");
                RE::BSInputDeviceManager* inputMngr = RE::BSInputDeviceManager::GetSingleton();
                if (inputMngr) {
                    RE::BSWin32GamepadDevice* gamePad = (RE::BSWin32GamepadDevice*)inputMngr->GetGamepad();
                    if (gamePad)
                        GamepadEvent(gamePad);
                }
                
            }
            
            /////static RE::TESDataHandler* dataHandler = nullptr;
            //static RE::BSInputDeviceManager* inputMngr = nullptr;

            //if (!dataHandler) {
            //    dataHandler = RE::TESDataHandler::GetSingleton();
            //    inputMngr = RE::BSInputDeviceManager::GetSingleton();
            //}

            //if (inputMngr) {
            //    auto gamePad = (RE::BSWin32GamepadDevice*)(inputMngr->GetGamepad());
            //    if (gamePad) {
            //        /*auto rTrigger = RE::BSWin32GamepadDevice::Keys::kRightTrigger;
            //        if (gamePad->IsPressed(rTrigger)) RE::DebugNotification("right trigger!");*/

            //    }
            //}
            return RE::BSEventNotifyControl::kContinue;
        }

        // Respond to captured gamepad events
        static void GamepadEvent(RE::BSWin32GamepadDevice* gamePad);
        
    protected:
        GamepadInputHandler() = default;
        GamepadInputHandler(const GamepadInputHandler&) = delete;
        GamepadInputHandler(GamepadInputHandler&&) = delete;
        virtual ~GamepadInputHandler() = default;

        auto operator=(const GamepadInputHandler&) -> GamepadInputHandler& = delete;
        auto operator=(GamepadInputHandler&&) -> GamepadInputHandler& = delete;
    };
}

void InitializeGamepadHooking() { GamepadInputEvent::GamepadInputHandler::Register(); }

//------------------------------------

//namespace GamepadInputEvent {
//    using EventResult = RE::BSEventNotifyControl;
//
//    static bool isWatching = false;
//
//    class InputDirectionEventHandler : public RE::BSTEventSink<RE::InputEvent*> {
//        using InputSink = RE::BSTEventSink<RE::InputEvent*>;
//	    using InputSource = RE::BSTEventSource<RE::InputEvent*>;
//    
//    public:
//        static InputDirectionEventHandler* GetSingleton() {
//            static InputDirectionEventHandler singleton;
//            return &singleton;
//        }
//        
//        static void Register() {
//
//            if (isWatching) 
//                return;
//
//            static RE::BSInputDeviceManager* inputDeviceManager = nullptr;
//            if (!inputDeviceManager) {
//                inputDeviceManager = RE::BSInputDeviceManager::GetSingleton();
//            }
//
//            if (inputDeviceManager && inputDeviceManager->sinks.size() >= 3) {
//                auto mySingle = InputDirectionEventHandler::GetSingleton();
//
//                inputDeviceManager->AddEventSink(mySingle);
//                logger::info("InputEventHandler initialized");
//                isWatching = true;
//            }
//
//
//
//            /*auto* eventSink = EventHandler::GetSingleton();
//            RE::BSTEventSink<RE::BSGamepadEvent>(GetSingleton())
//
//            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);
//
//            RE::BSTEventSource<RE::BSGamepadEvent>::AddEventSink(RE::BSTEventSink<RE::BSGamepadEvent>(GetSingleton()));
//
//            if (const auto event = RE::BSTEventSource<RE::BSGamepadEvent>::get; event) {
//                event->AddEventSink<RE::BSGamepadEvent>(GetSingleton());
//
//
//
//                /// logger::info("Registered {} handler", typeid(RE::BSGamepadEvent).name());
//                logger::info("Registered for spell learn events");
//            }*/
//        }
//
//        EventResult ProcessEvent(const RE::BSGamepadEvent* a_event, RE::BSTEventSource<RE::BSGamepadEvent>*) override;
//
//        // Respond to captured spell learned events
//        static void GamepadInput(const RE::SpellItem* const spell);
//
//    protected:
//        InputDirectionEventHandler() = default;
//        InputDirectionEventHandler(const InputDirectionEventHandler&) = delete;
//        InputDirectionEventHandler(InputDirectionEventHandler&&) = delete;
//        virtual ~InputDirectionEventHandler() = default;
//
//        auto operator=(const InputDirectionEventHandler&) -> InputDirectionEventHandler& = delete;
//        auto operator=(InputDirectionEventHandler&&) -> InputDirectionEventHandler& = delete;
//    };
//
//    EventResult EventHandler::ProcessEvent(const RE::BSGamepadEvent* a_event, RE::BSTEventSource<RE::BSGamepadEvent>*) {
//        if (a_event) {
//            /// logger::debug("Spell learned event!");
//            GamepadInput(a_event->spell);
//        }
//        return EventResult::kContinue;
//    }
//}
//
//// Method to initialize listening for spell learned events
//void InitializeGamepadHooking() { GamepadInputEvent::EventHandler::Register(); }
//
//#pragma region How to use
//
///*
//
//Call InitializeGamepadHooking and then employ GamepadInput where you want to process captured spell learn events
//
//// Executes when new spells are learned
//void GamepadInputEvent::EventHandler::GamepadInput(const RE::SpellItem* const spell) {
//    logger::debug("new spell learned!");
//    // do other stuff
//}
//
//*/
//
//#pragma endregion

#pragma region Source Credit

// OnGamepadEvent tracking functionality modified from source by Noah Boddie (shared directly via Discord)

#pragma endregion