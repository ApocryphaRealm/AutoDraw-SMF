#include "Diagnostics.h"

#include "DevBench/DevBenchAPI.h"
#include "Settings.h"
#include "utils/Logger.h"

#include <mutex>

namespace diagnostics
{
	namespace
	{
		using clock = std::chrono::steady_clock;

		std::mutex mtx;

		struct State
		{
			std::uint64_t combatEntries = 0;
			std::uint64_t combatExits = 0;
			std::optional<clock::time_point> lastCombatChange;
			bool lastCombatChangeWasEnter = false;

			std::uint64_t weaponDrawAnimEvents = 0;
			std::optional<clock::time_point> lastWeaponDrawAnimEvent;

			std::uint64_t sheathesScheduled = 0;
			std::optional<clock::time_point> lastSheatheScheduled;
			float lastSheatheDelaySeconds = 0.0F;

			std::uint64_t sheathesDeferred = 0;
			std::optional<clock::time_point> lastSheatheDeferred;
			std::string lastSheatheDeferredReason;

			std::uint64_t sheathesSkippedBoundWeapon = 0;
			std::optional<clock::time_point> lastSheatheSkippedBoundWeapon;

			std::uint64_t sheathesApplied = 0;
			std::optional<clock::time_point> lastSheatheApplied;
		};

		State state;

		// Escapes the handful of characters JSON requires; every string this module puts into
		// a value is either a fixed literal from our own code (a defer reason) or a plugin
		// name, so this is defensive rather than load-bearing, but cheap to get right.
		std::string EscapeJson(std::string_view a_text)
		{
			std::string out;
			out.reserve(a_text.size());

			for (char c : a_text)
			{
				switch (c)
				{
				case '"':
					out += "\\\"";
					break;
				case '\\':
					out += "\\\\";
					break;
				case '\n':
					out += "\\n";
					break;
				default:
					out += c;
					break;
				}
			}

			return out;
		}

		// Renders "field": null or "field": <seconds ago>, so a query can tell "never
		// happened" apart from "happened a long time ago" instead of both looking like a
		// missing/zero field.
		std::string SecondsAgoField(const char* a_name, const std::optional<clock::time_point>& a_when)
		{
			if (!a_when)
			{
				return std::format("\"{}SecondsAgo\": null", a_name);
			}

			const double seconds = std::chrono::duration<double>(clock::now() - *a_when).count();

			return std::format("\"{}SecondsAgo\": {:.1f}", a_name, seconds);
		}

		void StatusTool(void*, const char*, void* a_sink, DevBenchAPI::WriteFn a_write)
		{
			std::string json;

			{
				std::scoped_lock lock(mtx);

				json = std::format(
					"{{"
					"\"settings\":{{"
					"\"enableAutoDraw\":{},"
					"\"enableAutoSheathe\":{},"
					"\"sheatheDelaySeconds\":{:.2f},"
					"\"exemptBoundWeapons\":{}"
					"}},"
					"\"combat\":{{"
					"\"entries\":{},"
					"\"exits\":{},"
					"\"lastChangeWasEnter\":{},"
					"{}"
					"}},"
					"\"weaponDrawAnimEvents\":{{"
					"\"count\":{},"
					"{}"
					"}},"
					"\"sheatheScheduled\":{{"
					"\"count\":{},"
					"\"lastDelaySeconds\":{:.2f},"
					"{}"
					"}},"
					"\"sheatheDeferred\":{{"
					"\"count\":{},"
					"\"lastReason\":\"{}\","
					"{}"
					"}},"
					"\"sheatheSkippedBoundWeapon\":{{"
					"\"count\":{},"
					"{}"
					"}},"
					"\"sheatheApplied\":{{"
					"\"count\":{},"
					"{}"
					"}}"
					"}}",
					settings::automation::enableAutoDraw ? "true" : "false",
					settings::automation::enableAutoSheathe ? "true" : "false",
					settings::automation::sheatheDelaySeconds,
					settings::automation::exemptBoundWeapons ? "true" : "false",
					state.combatEntries,
					state.combatExits,
					state.lastCombatChangeWasEnter ? "true" : "false",
					SecondsAgoField("lastChange", state.lastCombatChange),
					state.weaponDrawAnimEvents,
					SecondsAgoField("last", state.lastWeaponDrawAnimEvent),
					state.sheathesScheduled,
					state.lastSheatheDelaySeconds,
					SecondsAgoField("last", state.lastSheatheScheduled),
					state.sheathesDeferred,
					EscapeJson(state.lastSheatheDeferredReason),
					SecondsAgoField("last", state.lastSheatheDeferred),
					state.sheathesSkippedBoundWeapon,
					SecondsAgoField("last", state.lastSheatheSkippedBoundWeapon),
					state.sheathesApplied,
					SecondsAgoField("last", state.lastSheatheApplied));
			}

			a_write(a_sink, json.c_str());
		}
	}

	void Init(bool a_lastAttempt)
	{
		static bool registered = false;

		if (registered)
		{
			return;
		}

		DevBenchAPI::IDevBenchInterface001* devBench = DevBenchAPI::GetDevBenchInterface001();

		if (!devBench)
		{
			if (a_lastAttempt)
			{
				logger::info("DevBench not detected; skipping the \"autodraw.status\" live-diagnostics tool "
							 "(logging alone still covers this session - see CLAUDE.md rule 31)");
			}
			else
			{
				// Not terminal - devbench's own server can still be finishing startup this
				// soon after kPostLoad (confirmed from a real launch's timestamps: devbench
				// finished ~100ms after kPostLoad fired, which was enough to lose this race).
				// Retried again at the next message point per rule 17.
				logger::debug("DevBench not detected yet; will retry at the next message");
			}

			return;
		}

		constexpr const char* descriptor =
			"{"
			"\"description\":\"Live Auto Draw state: current settings, combat/anim-event "
			"counters, and what the last delayed-sheathe decision did and why.\","
			"\"inputSchema\":{\"type\":\"object\",\"properties\":{}},"
			"\"readOnly\":true"
			"}";

		if (devBench->RegisterTool("autodraw.status", descriptor, &StatusTool, nullptr))
		{
			logger::info("Registered \"autodraw.status\" with DevBench (build {})", devBench->GetBuildNumber());
		}
		else
		{
			logger::warn("DevBench reported \"autodraw.status\" replaced an existing tool of the same name");
		}

		registered = true;
	}

	void RecordCombatStateChange(bool a_enteredCombat)
	{
		std::scoped_lock lock(mtx);

		if (a_enteredCombat)
		{
			++state.combatEntries;
		}
		else
		{
			++state.combatExits;
		}

		state.lastCombatChangeWasEnter = a_enteredCombat;
		state.lastCombatChange = clock::now();
	}

	void RecordWeaponDrawAnimEvent()
	{
		std::scoped_lock lock(mtx);

		++state.weaponDrawAnimEvents;
		state.lastWeaponDrawAnimEvent = clock::now();
	}

	void RecordSheatheScheduled(float a_delaySeconds)
	{
		std::scoped_lock lock(mtx);

		++state.sheathesScheduled;
		state.lastSheatheDelaySeconds = a_delaySeconds;
		state.lastSheatheScheduled = clock::now();
	}

	void RecordSheatheDeferred(std::string_view a_reason)
	{
		std::scoped_lock lock(mtx);

		++state.sheathesDeferred;
		state.lastSheatheDeferredReason = a_reason;
		state.lastSheatheDeferred = clock::now();
	}

	void RecordSheatheSkippedBoundWeapon()
	{
		std::scoped_lock lock(mtx);

		++state.sheathesSkippedBoundWeapon;
		state.lastSheatheSkippedBoundWeapon = clock::now();
	}

	void RecordSheatheApplied()
	{
		std::scoped_lock lock(mtx);

		++state.sheathesApplied;
		state.lastSheatheApplied = clock::now();
	}
}
