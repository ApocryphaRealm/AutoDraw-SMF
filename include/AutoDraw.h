#pragma once

namespace AutoDrawSheathe
{
	// Schedules a delayed check of a_actor and, if nothing is still deferring it when the
	// delay elapses, sheathes whatever weapon or magic is drawn. Re-schedules itself for as
	// long as the actor keeps deferring (still in combat, attacking, blocking, airborne, or
	// dodging) - ported forward from the original Auto Draw's retry loop, unchanged in shape.
	//
	// New in this fork: if settings::automation::exemptBoundWeapons is on and the actor has a
	// bound (conjured) weapon equipped, the sheathe is skipped entirely rather than deferred -
	// see Settings.h's exemptBoundWeapons comment for why forcing a bound weapon away is
	// undesirable. Both call sites (CombatEvent's kNone transition and AnimEventSink's
	// "WeaponDraw" trigger) funnel through this one function, so gating here covers both.
	void ForceSheathe(RE::Actor* a_actor);

	// Seeds the mod's behaviour from the state the player is ALREADY in, rather than waiting for
	// a transition that will never come.
	//
	// The bug this exists for: AnimEventSink starts the delayed-sheathe check only when it sees a
	// "WeaponDraw" animation event on the player. Loading a save with the weapon already out
	// produces no such event - nothing transitions, because the weapon is already drawn - so
	// ForceSheathe was never scheduled and the mod did nothing until the player manually sheathed
	// and drew again, finally emitting the event it had been waiting for. Re-attaching the
	// animation sink at kPostLoadGame (which was already happening) is not the same as seeding the
	// state: the mod ended up correctly listening for the NEXT transition while never looking at
	// the state it was already in.
	//
	// Call this from kPostLoadGame and kNewGame, after PlayerAnim::EnsureAttached(). It routes
	// through ForceSheathe rather than reimplementing the decision, so every existing exemption
	// (bound weapons, the in-combat/attacking/blocking/airborne/dodging deferrals, and the
	// re-check of IsWeaponDrawn when the delay elapses) applies unchanged - a player who sheathes
	// during the delay is handled for free.
	//
	// The player is frequently not resolvable at the moment kPostLoadGame fires, so per CLAUDE.md
	// rule 17 this retries rather than treating one failed lookup as terminal, and logs only on
	// transitions rather than on every attempt.
	void SeedFromLoadedState();
}
