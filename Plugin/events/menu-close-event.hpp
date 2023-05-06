namespace MenuOpenCloseEvent {
    using EventResult = RE::BSEventNotifyControl;

    class EventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler singleton;
            return std::addressof(singleton);
        }

        static void Register() {
            auto* eventSink = EventHandler::GetSingleton();
            ///auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            RE::UI::GetSingleton()->AddEventSink<RE::MenuOpenCloseEvent>(eventSink);
            logger::info("Registered for menu open/close events");
        }

        virtual EventResult ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override;

        // Respond to captured menu open/close events
        static void MenuOpenClose(const RE::MenuOpenCloseEvent* a_event);

    private:
        EventHandler() = default;
        EventHandler(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        virtual ~EventHandler() = default;

        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
    };

    EventResult EventHandler::ProcessEvent(const RE::MenuOpenCloseEvent* a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) {
        /// logger::debug("Menu open/close event!");
        MenuOpenClose(a_event);
        return EventResult::kContinue;
    }
}

// Method to initialize listening for menu open/close events
void InitializeMenuOpenCloseHooking() { MenuOpenCloseEvent::EventHandler::Register(); }

#pragma region How to use

/*

Call InitializeMenuOpenCloseHooking and then employ MenuOpenClose where you want to process captured menu open/close events

// Executes when a menu opens or closes
void MenuOpenCloseEvent::EventHandler::MenuOpenClose() {
   logger::debug("Menu open/close event!");
   // do other stuff
}

*/

#pragma endregion

#pragma region Source Credit

// Menu open/close event tracking functionality modified based reference code from "True Directional Movement" by Ershin (MIT License)
// Links ==> https://github.com/ersh1/TrueDirectionalMovement; https://www.nexusmods.com/skyrimspecialedition/mods/51614

// Menu open/close event tracking functionality also modified based on reference code from "Events" by MrowrPurr
// Link ==> https://skyrim.dev/skse/events

#pragma endregion