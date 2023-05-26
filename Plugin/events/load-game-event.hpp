namespace LoadGameEvent
{
    using EventResult = RE::BSEventNotifyControl;

    class EventHandler : public RE::BSTEventSink<RE::TESLoadGameEvent>
    {
    public:
        static EventHandler* GetSingleton()
        {
            static EventHandler singleton;
            return std::addressof(singleton);
        }

        static void Register()
        {
            auto scriptEventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
            scriptEventSourceHolder->GetEventSource<RE::TESLoadGameEvent>()->AddEventSink(EventHandler::GetSingleton());
            /// logger::info("Registered {}", typeid(RE::TESLoadGameEvent).name());
            logger::info("Registered for game load events");
        }

        virtual EventResult ProcessEvent(const RE::TESLoadGameEvent* a_event, RE::BSTEventSource<RE::TESLoadGameEvent>* a_eventSource) override;

        // Respond to captured load game events
        static void GameLoaded();

    private:
        EventHandler() = default;
        EventHandler(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        virtual ~EventHandler() = default;

        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
    };

    EventResult EventHandler::ProcessEvent(const RE::TESLoadGameEvent* a_event, RE::BSTEventSource<RE::TESLoadGameEvent>*)
    {
        if (a_event)
        {
            /// logger::debug("Game loaded!");
            GameLoaded();
        }
        return EventResult::kContinue;
    }
}

// Method to initialize listening for spell learned events
void InitializeLoadGameHooking() { LoadGameEvent::EventHandler::Register(); }

#pragma region How to use

/*

Call InitializeLoadGameHooking and then employ GameLoaded where you want to process captured game load events

// Executes when game loads
void LoadGameEvent::EventHandler::GameLoaded() {
    logger::debug("Game loaded!");
    /// CheckUpdate();  // Call method to check for game data updates
}

*/

#pragma endregion

#pragma region Source Credit

// Game load event tracking functionality modified based reference code from "True Directional Movement" by Ershin (MIT License)
// Links ==> https://github.com/ersh1/TrueDirectionalMovement; https://www.nexusmods.com/skyrimspecialedition/mods/51614

#pragma endregion