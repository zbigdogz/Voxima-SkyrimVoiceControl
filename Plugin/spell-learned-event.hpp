namespace SpellLearnedEvent {
    using EventResult = RE::BSEventNotifyControl;

    class EventHandler : public RE::BSTEventSink<RE::SpellsLearned::Event> {
    public:
        static EventHandler* GetSingleton() {
            static EventHandler singleton;
            return &singleton;
        }

        static void Register() {
            if (const auto event = RE::SpellsLearned::GetEventSource(); event) {
                event->AddEventSink<RE::SpellsLearned::Event>(GetSingleton());
                /// logger::info("Registered {} handler", typeid(RE::SpellsLearned::Event).name());
                logger::info("Registered for spell learn events");
            }
        }

        EventResult ProcessEvent(const RE::SpellsLearned::Event* a_event, RE::BSTEventSource<RE::SpellsLearned::Event>*) override;

        // Respond to captured spell learned events
        static void SpellLearned(const RE::SpellItem* const spell);

    private:
        EventHandler() = default;
        EventHandler(const EventHandler&) = delete;
        EventHandler(EventHandler&&) = delete;
        ~EventHandler() override = default;

        EventHandler& operator=(const EventHandler&) = delete;
        EventHandler& operator=(EventHandler&&) = delete;
    };

    EventResult EventHandler::ProcessEvent(const RE::SpellsLearned::Event* a_event, RE::BSTEventSource<RE::SpellsLearned::Event>*) {
        if (a_event) {
            /// logger::debug("Spell learned event!");
            SpellLearned(a_event->spell);
        }
        return EventResult::kContinue;
    }
}

// Method to initialize listening for spell learned events
void InitializeSpellLearnHooking() { SpellLearnedEvent::EventHandler::Register(); }

#pragma region How to use

/*

Call InitializeSpellLearnHooking and then employ SpellLearned where you want to process captured spell learn events

// Executes when new spells are learned
void SpellLearnedEvent::EventHandler::SpellLearned(const RE::SpellItem* const spell) {
    logger::debug("new spell learned!");
    // do other stuff
}

*/

#pragma endregion

#pragma region Source Credit

// OnSpellLearned event tracking functionality modified from "PapyrusExtenderSSE" by powerof3 (MIT License)
// Links ==> https://github.com/powerof3/PapyrusExtenderSSE; https://www.nexusmods.com/skyrimspecialedition/mods/22854

#pragma endregion