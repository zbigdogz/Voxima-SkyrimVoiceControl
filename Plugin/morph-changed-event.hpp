namespace MorphEvents {
    using EventResult = RE::BSEventNotifyControl;

    class EventHandler : public RE::BSTEventSink<RE::TESSwitchRaceCompleteEvent> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler singleton;
            return std::addressof(singleton);
        }

        static void Register() {
            auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            scriptEventSourceHolder->GetEventSource<RE::TESSwitchRaceCompleteEvent>()->AddEventSink(EventHandler::GetSingleton());
            /// logger::info("Registered {}", typeid(RE::TESSwitchRaceCompleteEvent).name());
            logger::info("Registered for morph change events");
        }

        virtual EventResult ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>* a_eventSource) override;

        // Respond to captured morph change events
        static void MorphChanged();

    private:
        EventHandler() = default;
        EventHandler(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        virtual ~EventHandler() = default;

        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
    };

    EventResult EventHandler::ProcessEvent(const RE::TESSwitchRaceCompleteEvent* a_event, RE::BSTEventSource<RE::TESSwitchRaceCompleteEvent>*) {
        if (a_event) {
            /// logger::debug("Morph change event!");
            MorphChanged();
        }
        return EventResult::kContinue;
    }
}

// Method to initialize listening for spell learned events
void InitializeMorphChangeHooking() { MorphEvents::EventHandler::Register(); }

#pragma region How to use

/*

Call InitializeMorphChangeHooking and then employ MorphChanged where you want to process captured morph change events

// Executes when new spells are learned
void MorphEvents::EventHandler::MorphChanged() {
   logger::debug("player morph changed!");
   // do other stuff
}

*/

#pragma endregion

#pragma region Source Credit

// Morph event tracking functionality modified based reference code from "True Directional Movement" by Ershin (MIT License)
// Links ==> https://github.com/ersh1/TrueDirectionalMovement; https://www.nexusmods.com/skyrimspecialedition/mods/51614

#pragma endregion