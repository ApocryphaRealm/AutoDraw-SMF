#include "AutoDraw.h"
#include "Settings.h"

namespace AutoDrawSheathe
{

	void ForceSheathe(RE::Actor* actor){

		RE::ActorHandle actorH = RE::ActorHandle(actor);
		std::thread([actorH] {
			std::this_thread::sleep_for(std::chrono::milliseconds(Settings::ForceSheatheTime));

			SKSE::GetTaskInterface()->AddTask([actorH] {
				RE::Actor* actor = actorH.get().get();
				
				bool bIsdodging = false;
				actor->GetGraphVariableBool("bIsdodging", bIsdodging);
				if (actor->IsInCombat() || actor->IsAttacking() || actor->IsBlocking() || actor->IsInMidair() || bIsdodging) {
					ForceSheathe(actor);
					return;
				} else {
					if (actor->AsActorState()->IsWeaponDrawn()) {
						actor->DrawWeaponMagicHands(false);
					}
				}
			});
		}).detach();
	}
}

