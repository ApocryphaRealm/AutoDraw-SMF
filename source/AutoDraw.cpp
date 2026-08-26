#include "AutoDraw.h"

#include "Diagnostics.h"
#include "Settings.h"
#include "utils/Logger.h"

namespace AutoDrawSheathe
{
	namespace
	{
		// Bound weapons occupy the same equip slots as ordinary ones, so this checks both
		// hands the same way the game itself resolves what is "drawn" - a dual-wielded actor
		// with a bound weapon in either hand counts.
		bool HasBoundWeaponEquipped(RE::Actor* a_actor)
		{
			for (bool leftHand : { false, true })
			{
				auto* weapon = a_actor->GetEquippedObject(leftHand)
									? a_actor->GetEquippedObject(leftHand)->As<RE::TESObjectWEAP>()
									: nullptr;

				if (weapon && weapon->IsBound())
				{
					logger::trace("HasBoundWeaponEquipped: {} hand holds bound weapon \"{}\"",
						leftHand ? "left" : "right", weapon->GetName());

					return true;
				}
			}

			return false;
		}
	}

	void ForceSheathe(RE::Actor* a_actor)
	{
		if (!a_actor)
		{
			logger::error("ForceSheathe called with a null actor");

			return;
		}

		const float delaySeconds = settings::automation::sheatheDelaySeconds;

		diagnostics::RecordSheatheScheduled(delaySeconds);
		logger::trace("ForceSheathe: scheduling {} in {:.2f}s", a_actor->GetName(), delaySeconds);

		RE::ActorHandle actorHandle = a_actor->CreateRefHandle();

		std::thread([actorHandle, delaySeconds] {
			std::this_thread::sleep_for(std::chrono::duration<float>(delaySeconds));

			SKSE::GetTaskInterface()->AddTask([actorHandle] {
				RE::Actor* actor = actorHandle.get().get();

				if (!actor)
				{
					logger::trace("ForceSheathe: actor no longer exists when the delay elapsed");

					return;
				}

				bool isDodging = false;
				actor->GetGraphVariableBool("bIsdodging", isDodging);

				std::string_view deferReason;

				if (actor->IsInCombat())
				{
					deferReason = "still in combat";
				}
				else if (actor->IsAttacking())
				{
					deferReason = "still attacking";
				}
				else if (actor->IsBlocking())
				{
					deferReason = "still blocking";
				}
				else if (actor->IsInMidair())
				{
					deferReason = "still airborne";
				}
				else if (isDodging)
				{
					deferReason = "still dodging";
				}

				if (!deferReason.empty())
				{
					logger::trace("ForceSheathe: deferring {} ({}); rescheduling", actor->GetName(), deferReason);
					diagnostics::RecordSheatheDeferred(deferReason);

					ForceSheathe(actor);

					return;
				}

				if (!actor->AsActorState()->IsWeaponDrawn())
				{
					logger::trace("ForceSheathe: {} no longer has a weapon drawn; nothing to do", actor->GetName());

					return;
				}

				if (settings::automation::exemptBoundWeapons && HasBoundWeaponEquipped(actor))
				{
					logger::debug("ForceSheathe: skipping {} - bound weapon equipped and exemptBoundWeapons is on",
						actor->GetName());
					diagnostics::RecordSheatheSkippedBoundWeapon();

					return;
				}

				logger::debug("ForceSheathe: sheathing {}", actor->GetName());
				diagnostics::RecordSheatheApplied();

				actor->DrawWeaponMagicHands(false);
			});
		}).detach();
	}
}
