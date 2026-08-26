#pragma once

#include <atomic>

// Sink for RE::BSAnimationGraphEvent, registered per-actor by CombatEvent while that actor is
// in combat. Only reacts to the player's own "WeaponDraw" tag - see ProcessEvent for why it is
// still registered on non-player actors regardless (matches the upstream mod's own behavior,
// kept as-is rather than narrowed, since combat-state tracking still needs the registration
// lifecycle either way).
class AnimEventSink final : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
	static AnimEventSink* GetSingleton();

	void RegisterAnimEventSinkFor(RE::Actor* a_actor);
	void UnregisterAnimEventSinkFor(RE::Actor* a_actor);

	RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
		RE::BSTEventSource<RE::BSAnimationGraphEvent>*) override;

private:
	AnimEventSink() = default;
	AnimEventSink(const AnimEventSink&) = delete;
	AnimEventSink& operator=(const AnimEventSink&) = delete;
};

// Keeps the player specifically registered for anim events across game loads, independent of
// combat state - a manual weapon draw outside combat (e.g. via hotkey) still needs to start
// the delayed-sheathe check, and CombatEvent only registers/unregisters around combat.
namespace PlayerAnim
{
	inline std::atomic_bool g_attached{ false };

	bool IsGraphReady(RE::Actor* a_actor);
	void EnsureAttached();
	void Detach();
}
