#include "Settings.h"
#include "CombatEvent.h"
#include "APIHandler.h"
#include "AutoDraw.h"


namespace AutoDraw
{

	void EventCallback(SKSE::MessagingInterface::Message* msg)
	{
		if (msg->type == SKSE::MessagingInterface::kDataLoaded) {
			Settings::readSettings();
			Req_PrismaUI_API::Request_PrismaUI_API();

			if (auto* src = RE::ScriptEventSourceHolder::GetSingleton()) {
				src->AddEventSink(CombatEvent::GetSingleton());
			}

			PlayerAnim::EnsureAttached();		
		}

		if (msg->type == SKSE::MessagingInterface::kPostLoadGame) {		
			Req_PrismaUI_API::SendAll();

			AnimWatch::ClearAll();
			PlayerAnim::g_attached = false;
			PlayerAnim::EnsureAttached();
		}

		if (msg->type == SKSE::MessagingInterface::kNewGame) {
			Req_PrismaUI_API::SendAll();

			AnimWatch::ClearAll();
			PlayerAnim::g_attached = false;
			PlayerAnim::EnsureAttached();
		}
		
		if (msg->type == SKSE::MessagingInterface::kPreLoadGame) {

		}


	}

	
}
