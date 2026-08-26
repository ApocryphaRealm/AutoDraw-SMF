#pragma once

// Sink for RE::TESCombatEvent - the trigger for both halves of this mod's automation: an
// instant draw when the player becomes the target of something entering combat, and starting
// the delayed-sheathe check (AutoDrawSheathe::ForceSheathe) when an actor leaves combat.
class CombatEvent final : public RE::BSTEventSink<RE::TESCombatEvent>
{
public:
	static CombatEvent* GetSingleton();

	RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* a_event,
		RE::BSTEventSource<RE::TESCombatEvent>*) override;
};
