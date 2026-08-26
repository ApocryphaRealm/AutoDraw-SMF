#pragma once

// Backs the "autodraw.status" DevBench tool - see CLAUDE.md rule 31 (every mod's first
// version ships with live-queryable state, not just logs reconstructed after the fact).
//
// Every Record* function here is called from the main thread, at the exact point a decision
// is made, and only ever writes a mutex-guarded snapshot. The DevBench tool handler runs on
// devbench's own listener thread and only ever reads that snapshot - it never reaches back
// into game state itself.
namespace diagnostics
{
	// Looks up the DevBench interface (present only if the DevBench plugin is installed) and
	// registers "autodraw.status". Safe to call repeatedly - a real launch showed devbench's
	// own server can still be finishing startup a moment after SKSE sends kPostLoad (its own
	// documented earliest-safe point), so this is a rule-17 retry, not a one-shot lookup: call
	// it again at kPostPostLoad and kDataLoaded too. Every call after the first successful one
	// is a cheap no-op; only the final call (a_lastAttempt = true) logs that DevBench was never
	// found, so the "not installed" conclusion isn't reported before every retry is exhausted.
	void Init(bool a_lastAttempt = false);

	// e->newState transitions seen by CombatEvent::ProcessEvent, for the player specifically -
	// this mod only acts on the player's own weapon, so tracking any other actor's combat
	// state would just be noise a query has to filter back out.
	void RecordCombatStateChange(bool a_enteredCombat);

	// AnimEventSink saw the player's own "WeaponDraw" animation event tag.
	void RecordWeaponDrawAnimEvent();

	// AutoDrawSheathe::ForceSheathe was called (from either CombatEvent's kNone transition or
	// the WeaponDraw anim event) and queued its delayed check.
	void RecordSheatheScheduled(float a_delaySeconds);

	// The delayed check ran and found a reason to defer (still in combat, attacking, blocking,
	// airborne, or dodging) and rescheduled itself - a_reason names which one.
	void RecordSheatheDeferred(std::string_view a_reason);

	// The delayed check ran, nothing was deferring it, but it skipped the actual sheathe
	// because the drawn weapon is bound and settings::automation::exemptBoundWeapons is on.
	void RecordSheatheSkippedBoundWeapon();

	// The delayed check ran and actually called DrawWeaponMagicHands(false).
	void RecordSheatheApplied();
}
