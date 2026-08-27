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

	void SeedFromLoadedState()
	{
		// kPostLoadGame fires before the player is reliably resolvable, so this polls rather than
		// giving up on one lookup (rule 17). The wait is bounded: a save load that has not produced
		// a usable player within this window has something else wrong with it, and retrying
		// forever would just hide that.
		if (!settings::automation::enableAutoSheathe)
		{
			logger::trace("SeedFromLoadedState: enableAutoSheathe is off; nothing to seed");

			return;
		}

		std::thread([] {
			// Declared inside the thread body rather than captured: fmt takes its arguments by
			// reference, which odr-uses them, and an uncaptured constexpr from the enclosing scope
			// would not survive that.
			constexpr int  kMaxAttempts = 100;
			constexpr auto kRetryInterval = std::chrono::milliseconds(100);

			for (int attempt = 1; attempt <= kMaxAttempts; ++attempt)
			{
				bool resolved = false;

				// The actual read has to happen on the game's own thread, so the poll loop lives
				// here and each check is posted across - the same split ForceSheathe already uses.
				std::atomic_bool done{ false };

				SKSE::GetTaskInterface()->AddTask([&resolved, &done, attempt] {
					RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
					RE::ActorState*      state = player ? player->AsActorState() : nullptr;

					if (!player || !state)
					{
						// Logged once, on the first miss only - a per-attempt line here would bury
						// the log in noise for something that is expected to resolve shortly.
						if (attempt == 1)
						{
							logger::debug("SeedFromLoadedState: player not resolvable yet; retrying");
						}

						done = true;

						return;
					}

					resolved = true;

					if (!state->IsWeaponDrawn())
					{
						logger::trace("SeedFromLoadedState: weapon is not drawn at load; nothing to seed");

						done = true;

						return;
					}

					// This is the whole point of the function: the weapon was already out when the
					// save loaded, so no "WeaponDraw" event was ever going to arrive. Schedule the
					// same delayed check the anim-event path would have.
					logger::info("SeedFromLoadedState: weapon already drawn at load; scheduling the "
								 "delayed sheathe check that no WeaponDraw event would have started");

					ForceSheathe(player);

					done = true;
				});

				while (!done.load())
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}

				if (resolved)
				{
					if (attempt > 1)
					{
						logger::debug("SeedFromLoadedState: player resolved on attempt {}", attempt);
					}

					return;
				}

				std::this_thread::sleep_for(kRetryInterval);
			}

			logger::warn("SeedFromLoadedState: gave up after {} attempts - the player never became "
						 "resolvable, so a weapon drawn at load will not be sheathed until the next "
						 "draw event",
				kMaxAttempts);
		}).detach();
	}
}
