#include "AnimEventSink.h"

class CombatEvent final : public RE::BSTEventSink<RE::TESCombatEvent>
{
public:
	static CombatEvent* GetSingleton();
	void OnEnterCombat(RE::Actor* actor);
	void OnLeaveCombat(RE::Actor* actor);
	RE::BSEventNotifyControl ProcessEvent(const RE::TESCombatEvent* e, RE::BSTEventSource<RE::TESCombatEvent>*);
};