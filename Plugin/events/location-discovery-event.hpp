namespace LocationDiscoveredEvent {
    using EventResult = RE::BSEventNotifyControl;

    class EventHandler : public RE::BSTEventSink<RE::LocationDiscovery::Event> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler singleton;
            return &singleton;
        }

        static void Register() {
            if (const auto event = RE::LocationDiscovery::GetEventSource(); event) {
                event->AddEventSink<RE::LocationDiscovery::Event>(GetSingleton());
                /// logger::info("Registered {} handler", typeid(RE::LocationDiscovery::Event).name());
                logger::info("Registered for location discovery events");
            }
        }

        EventResult ProcessEvent(const RE::LocationDiscovery::Event* a_event, RE::BSTEventSource<RE::LocationDiscovery::Event>*) override;

        // Respond to captured location discovery events
        static void LocationDiscovered(std::string locationName);

    private:
        EventHandler() = default;
        EventHandler(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        ~EventHandler() override = default;

        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
    };

    EventResult EventHandler::ProcessEvent(const RE::LocationDiscovery::Event* a_event, RE::BSTEventSource<RE::LocationDiscovery::Event>*) {
        if (a_event) {
            /// logger::debug("Location discovered event!");
            LocationDiscovered(a_event->mapMarkerData->locationName.GetFullName());
        }
        return EventResult::kContinue;
    }
}

// Method to initialize listening for location discovery events
void InitializeLocationDiscoveryHooking() { LocationDiscoveredEvent::EventHandler::Register(); }

#pragma region How to use

/*

Call InitializeLocationDiscoveryHooking and then employ LocationDiscovered where you want to process captured location discovery events

// Executes when new locations are discovered
void LocationDiscoveredEvent::EventHandler::LocationDiscovered(std::string locationName) {
    logger::debug("new location discovered!");
    // do other stuff
}

*/

#pragma endregion

#pragma region Source Credit

// OnLocationDiscovered event tracking functionality modified from "PapyrusExtenderSSE" by powerof3 (MIT License)
// Links ==> https://github.com/powerof3/PapyrusExtenderSSE; https://www.nexusmods.com/skyrimspecialedition/mods/22854

#pragma endregion