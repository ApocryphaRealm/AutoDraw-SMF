#include "AnimEventSink.h"
#include "CombatEvent.h"
#include "Diagnostics.h"
#include "Settings.h"
#include "UI.h"
#include "utils/Logger.h"

void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	if (!a_msg)
	{
		return;
	}

	switch (a_msg->type)
	{
	case SKSE::MessagingInterface::kPostLoad:
		// DevBenchAPI's own contract: the interface can only be requested once SKSE has sent
		// kPostLoad, since that's the earliest point every plugin (DevBench included) has had
		// its own SKSEPluginLoad run.
		logger::debug("kPostLoad received; registering live diagnostics with DevBench if present");
		diagnostics::Init();
		break;

	case SKSE::MessagingInterface::kPostPostLoad:
		// By kPostPostLoad every plugin has finished its own post-load work, so SKSE Menu
		// Framework's module is guaranteed to be in the process if it is installed at all.
		logger::debug("kPostPostLoad received; registering settings page with SKSE Menu Framework");
		UI::Register();

		// Rule-17 retry: a real launch showed devbench's own server can still be finishing
		// startup a moment after kPostLoad fires, which is early enough to lose the race even
		// though kPostLoad is DevBenchAPI's own documented earliest-safe point. Cheap no-op if
		// the kPostLoad attempt already succeeded.
		diagnostics::Init();
		break;

	case SKSE::MessagingInterface::kDataLoaded:
		logger::debug("kDataLoaded received; registering the combat event sink and attaching to the player");

		if (auto* src = RE::ScriptEventSourceHolder::GetSingleton())
		{
			src->AddEventSink(CombatEvent::GetSingleton());
		}
		else
		{
			logger::error("kDataLoaded: RE::ScriptEventSourceHolder::GetSingleton() returned null; "
						   "combat-triggered draw/sheathe will not function this session");
		}

		PlayerAnim::EnsureAttached();

		// Last retry point - if DevBench still isn't found here, conclude it isn't installed
		// and say so, rather than staying silent about it forever.
		diagnostics::Init(/* a_lastAttempt = */ true);
		break;

	case SKSE::MessagingInterface::kPostLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		// A save load or new game gets a fresh player animation graph pointer, so the sink
		// registration from before has to be dropped and re-attached rather than assumed to
		// still be valid.
		logger::debug("{} received; re-attaching to the player's animation graph",
			a_msg->type == SKSE::MessagingInterface::kNewGame ? "kNewGame" : "kPostLoadGame");

		PlayerAnim::g_attached = false;
		PlayerAnim::EnsureAttached();
		break;

	default:
		break;
	}
}
