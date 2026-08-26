#pragma once
#include <atomic>
#include <unordered_set>
#include <mutex>

constexpr float kDetectRange = 1800.0f;

class AnimEventSink : public RE::BSTEventSink<RE::BSAnimationGraphEvent>
{
public:
	static AnimEventSink* GetSingleton();

	void RegisterAnimEventSinkFor(RE::Actor* actor);  
	void UnregisterAnimEventSinkFor(RE::Actor* actor);  
	bool prevKillmove = false;

	RE::BSEventNotifyControl ProcessEvent(const RE::BSAnimationGraphEvent* a_event, RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_source) override;


private:
	AnimEventSink() = default;
	AnimEventSink(const AnimEventSink&) = delete;
	AnimEventSink& operator=(const AnimEventSink&) = delete;
};


namespace PlayerAnim
{
	inline std::atomic_bool g_attached{ false };

	bool IsGraphReady(RE::Actor* a);
	void EnsureAttached();
	void Detach();
}


namespace AnimWatch
{
	inline std::unordered_set<RE::FormID> g_attached{};
	inline std::mutex g_mtx{};

	bool IsAttached(RE::Actor* a);
	void MarkAttached(RE::Actor* a);
	void Unmark(RE::FormID id);
	void ClearAll();
}



bool InRangeOfPC(RE::Actor* a);
void EnsureAttachAnimSink(RE::Actor* a, int retry = 2);
void DetachAnimSink(RE::Actor* a);
