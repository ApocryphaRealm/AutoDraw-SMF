#include "CombatEvent.h"

#include "AnimEventSink.h"
#include "AutoDraw.h"
#include "Diagnostics.h"
#include "Settings.h"
#include "utils/Logger.h"

namespace
{
	// How far from the player a combat-state change is still worth reacting to. Ported from
	// the original mod's own constant, unchanged - this mod only cares about combat close
	// enough to plausibly involve the player either way.
	constexpr float kCombatDetectRange = 1800.0F;
}

CombatEvent* CombatEvent::GetSingleton()
{
	static CombatEvent s;

	return std::addressof(s);
}

RE::BSEventNotifyControl CombatEvent::ProcessEvent(const RE::TESCombatEvent* a_event,
	RE::BSTEventSource<RE::TESCombatEvent>*)
{
	if (!a_event)
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::TESObjectREFR* ref = a_event->actor.get();

	if (!ref)
	{
		logger::trace("CombatEvent: actor handle did not resolve; ignoring this event");

		return RE::BSEventNotifyControl::kContinue;
	}

	RE::Actor* actor = ref->As<RE::Actor>();

	if (!actor)
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	if (!actor->Is3DLoaded())
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();

	if (!pc)
	{
		logger::error("CombatEvent: RE::PlayerCharacter::GetSingleton() returned null");

		return RE::BSEventNotifyControl::kContinue;
	}

	if (pc->GetPosition().GetDistance(actor->GetPosition()) > kCombatDetectRange)
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::Actor* targetActor = nullptr;

	if (RE::TESObjectREFR* targetRef = a_event->targetActor.get())
	{
		targetActor = targetRef->As<RE::Actor>();
	}

	if (a_event->newState == RE::ACTOR_COMBAT_STATE::kCombat)
	{
		diagnostics::RecordCombatStateChange(true);

		if (settings::automation::enableAutoDraw && targetActor && targetActor->IsPlayerRef() &&
			!targetActor->AsActorState()->IsWeaponDrawn())
		{
			logger::debug("CombatEvent: {} entered combat targeting the player; drawing", actor->GetName());

			targetActor->DrawWeaponMagicHands(true);
		}

		AnimEventSink::GetSingleton()->RegisterAnimEventSinkFor(actor);
	}
	else if (a_event->newState == RE::ACTOR_COMBAT_STATE::kNone)
	{
		diagnostics::RecordCombatStateChange(false);

		if (settings::automation::enableAutoSheathe)
		{
			logger::debug("CombatEvent: {} left combat; starting the delayed sheathe check", pc->GetName());

			AutoDrawSheathe::ForceSheathe(pc);
		}

		AnimEventSink::GetSingleton()->UnregisterAnimEventSinkFor(actor);
	}

	return RE::BSEventNotifyControl::kContinue;
}
