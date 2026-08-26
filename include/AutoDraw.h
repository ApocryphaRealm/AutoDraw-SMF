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
}
