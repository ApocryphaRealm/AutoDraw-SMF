#include "AnimEventSink.h"

#include "AutoDraw.h"
#include "Diagnostics.h"
#include "Settings.h"
#include "utils/Logger.h"

AnimEventSink* AnimEventSink::GetSingleton()
{
	static AnimEventSink s;

	return std::addressof(s);
}

RE::BSEventNotifyControl AnimEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event,
	RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
{
	if (!a_event || !a_event->holder)
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	RE::Actor* actor = const_cast<RE::Actor*>(a_event->holder->As<RE::Actor>());

	if (!actor || !actor->IsPlayerRef())
	{
		return RE::BSEventNotifyControl::kContinue;
	}

	if (a_event->tag == "WeaponDraw")
	{
		diagnostics::RecordWeaponDrawAnimEvent();

		if (settings::automation::enableAutoSheathe)
		{
			logger::trace("AnimEventSink: player \"WeaponDraw\" seen; starting the delayed sheathe check");

			AutoDrawSheathe::ForceSheathe(actor);
		}
	}

	return RE::BSEventNotifyControl::kContinue;
}

void AnimEventSink::RegisterAnimEventSinkFor(RE::Actor* a_actor)
{
	if (!a_actor)
	{
		return;
	}

	a_actor->AddAnimationGraphEventSink(AnimEventSink::GetSingleton());
}

void AnimEventSink::UnregisterAnimEventSinkFor(RE::Actor* a_actor)
{
	if (!a_actor)
	{
		return;
	}

	a_actor->RemoveAnimationGraphEventSink(AnimEventSink::GetSingleton());
}

namespace PlayerAnim
{
	bool IsGraphReady(RE::Actor* a_actor)
	{
		if (!a_actor || !a_actor->Is3DLoaded())
		{
			return false;
		}

		RE::BSAnimationGraphManagerPtr animGraphManager;

		return a_actor->GetAnimationGraphManager(animGraphManager);
	}

	void EnsureAttached()
	{
		RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();

		if (!pc)
		{
			logger::error("PlayerAnim::EnsureAttached: RE::PlayerCharacter::GetSingleton() returned null");

			return;
		}

		if (g_attached.load())
		{
			return;
		}

		if (!IsGraphReady(pc))
		{
			logger::trace("PlayerAnim::EnsureAttached: player animation graph not ready yet");

			return;
		}

		AnimEventSink::GetSingleton()->RegisterAnimEventSinkFor(pc);
		g_attached = true;

		logger::debug("PlayerAnim::EnsureAttached: attached to the player's animation graph");
	}

	void Detach()
	{
		RE::PlayerCharacter* pc = RE::PlayerCharacter::GetSingleton();

		if (!pc || !g_attached.load())
		{
			return;
		}

		AnimEventSink::GetSingleton()->UnregisterAnimEventSinkFor(pc);
		g_attached = false;

		logger::debug("PlayerAnim::Detach: detached from the player's animation graph");
	}
}
