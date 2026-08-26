#include "CombatEvent.h"
#include "AutoDraw.h"
#include "Settings.h"


	CombatEvent* CombatEvent::GetSingleton()
	{
		static CombatEvent s;
		return std::addressof(s);
	}

	RE::BSEventNotifyControl CombatEvent::ProcessEvent(const RE::TESCombatEvent* e, RE::BSTEventSource<RE::TESCombatEvent>*)
	{
		if (!e)
			return RE::BSEventNotifyControl::kContinue;

		auto* ref = e->actor.get();  // TESObjectREFR*
		auto* targetRef = e->targetActor.get();

		auto* actor = ref->As<RE::Actor>();
		if (!actor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		RE::Actor* targetActor = nullptr;
		if (targetRef) {
			targetActor = targetRef->As<RE::Actor>();
		}
		constexpr float kCombatDetectRange = 1800.0f;

		auto* pc = RE::PlayerCharacter::GetSingleton();

	
		if (!actor || !actor->Is3DLoaded())
			return RE::BSEventNotifyControl::kContinue;
		if (pc->GetPosition().GetDistance(actor->GetPosition()) > kCombatDetectRange) {
			return RE::BSEventNotifyControl::kContinue; 
		}
		

		if (e->newState == RE::ACTOR_COMBAT_STATE::kCombat) {
			if (Settings::EnableAutoDraw) {
				if (targetActor && targetActor->IsPlayerRef()) {
					if (!targetActor->AsActorState()->IsWeaponDrawn()) {
						targetActor->DrawWeaponMagicHands(true);
					}
				}
			}

			AnimEventSink::GetSingleton()->RegisterAnimEventSinkFor(actor);

		} else if (e->newState == RE::ACTOR_COMBAT_STATE::kNone) {	
			if (Settings::EnableAutoSheathe) {
				AutoDrawSheathe::ForceSheathe(pc);
			}

			AnimEventSink::GetSingleton()->UnregisterAnimEventSinkFor(actor);
		}
		return RE::BSEventNotifyControl::kContinue;
	}

