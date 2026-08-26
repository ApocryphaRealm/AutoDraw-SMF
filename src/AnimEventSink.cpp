#pragma once
#include <atomic>

#include "APIHandler.h"
#include "AnimEventSink.h"
#include "Settings.h"
#include "AutoDraw.h"


AnimEventSink* AnimEventSink::GetSingleton()
{
    static AnimEventSink s;
    return std::addressof(s);
}

RE::BSEventNotifyControl AnimEventSink::ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source) 
	{
		if (!a_event || !a_event->holder) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_source) {}

		const auto& tag = a_event->tag;  
		auto* holder = a_event->holder;  
		auto* actor = holder ? holder->As<RE::Actor>() : nullptr;
		auto* RefActor = const_cast<RE::Actor*>(actor);
		
		using clock = std::chrono::steady_clock;

		if (!RefActor) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (RefActor->IsPlayerRef()) {
			if (tag == "WeaponDraw") {
				if (Settings::EnableAutoSheathe) {
					AutoDrawSheathe::ForceSheathe(RefActor);
				}
			}
		} 
		
		return RE::BSEventNotifyControl::kContinue;
	}

	void AnimEventSink::RegisterAnimEventSinkFor(RE::Actor* actor)
	{
		if (!actor)
			return;

		actor->AddAnimationGraphEventSink(AnimEventSink::GetSingleton());
	}

	void AnimEventSink::UnregisterAnimEventSinkFor(RE::Actor* actor)
	{
		if (!actor)
			return;

		actor->RemoveAnimationGraphEventSink(AnimEventSink::GetSingleton());
	}



namespace PlayerAnim
{
	

	bool IsGraphReady(RE::Actor* a)
	{
		if (!a || !a->Is3DLoaded())
			return false;

		RE::BSAnimationGraphManagerPtr animGraphManager;
		return a->GetAnimationGraphManager(animGraphManager);
	}

	void EnsureAttached()
	{
		auto* pc = RE::PlayerCharacter::GetSingleton();
		if (!pc || g_attached.load())
			return;
		if (!IsGraphReady(pc))
			return;

		AnimEventSink::GetSingleton()->RegisterAnimEventSinkFor(pc);
		g_attached = true;
		//SKSE::log::info("[AnimSink] PC attached");
	}

	void Detach()
	{
		auto* pc = RE::PlayerCharacter::GetSingleton();
		if (!pc || !g_attached.load())
			return;

		AnimEventSink::GetSingleton()->UnregisterAnimEventSinkFor(pc);
		g_attached = false;
		//SKSE::log::info("[AnimSink] PC detached");
	}
}


namespace AnimWatch
{

	bool IsAttached(RE::Actor* a)
	{
		std::scoped_lock lk(g_mtx);
		return a && g_attached.count(a->GetFormID());
	}
	void MarkAttached(RE::Actor* a)
	{
		std::scoped_lock lk(g_mtx);
		if (a)
			g_attached.insert(a->GetFormID());
	}
	void Unmark(RE::FormID id)
	{
		std::scoped_lock lk(g_mtx);
		g_attached.erase(id);
	}
	void ClearAll()
	{
		std::scoped_lock lk(g_mtx);
		g_attached.clear();
	}
}



bool InRangeOfPC(RE::Actor* a)
{
	auto* pc = RE::PlayerCharacter::GetSingleton();
	return pc && a &&
	       pc->GetPosition().GetDistance(a->GetPosition()) <= kDetectRange;
}

void EnsureAttachAnimSink(RE::Actor* a, int retry)
{
	if (!a || !a->Is3DLoaded() || !InRangeOfPC(a))
		return;
	if (!a->IsInCombat())
		return;

	if (AnimWatch::IsAttached(a))
		return;

	RE::BSTSmartPointer<RE::BSAnimationGraphManager> mgr;
	if (!a->GetAnimationGraphManager(mgr) || !mgr) {
		if (retry > 0) {
			RE::ActorHandle h = a->CreateRefHandle();

			SKSE::GetTaskInterface()->AddTask([h, retry]() {
				if (auto ref = h.get()) {
					if (auto* aa = ref.get()->As<RE::Actor>()) {
						EnsureAttachAnimSink(aa, retry - 1);
					}
				}
			});
		}
		return;
	}

	AnimEventSink::GetSingleton()->RegisterAnimEventSinkFor(a);
	AnimWatch::MarkAttached(a);
	//SKSE::log::debug("[AnimSink] attach {:08X} {}", a->GetFormID(), a->GetName());
}

void DetachAnimSink(RE::Actor* a)
{
	if (!a)
		return;
	if (!AnimWatch::IsAttached(a)) {
		AnimWatch::Unmark(a->GetFormID());
		return;
	}
	AnimEventSink::GetSingleton()->UnregisterAnimEventSinkFor(a);
	AnimWatch::Unmark(a->GetFormID());
	//SKSE::log::debug("[AnimSink] detach {:08X} {}", a->GetFormID(), a->GetName());
}

