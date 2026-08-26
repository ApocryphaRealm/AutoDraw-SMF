#pragma once

namespace SKSE::log
{
	using level = spdlog::level::level_enum;
}
namespace logger = SKSE::log;

namespace settings
{
	// Reads the INI into the variables below. The values the variables hold when this is
	// called are remembered as the built-in defaults, so RestoreDefaults() can put them back.
	void Init(const std::string& a_iniFileName);

	// Writes every setting below back to the INI that Init() read, leaving the comments and
	// any unrelated keys in that file alone. Returns false if the file could not be written.
	bool Save();

	// Puts every setting back to its built-in default. This only touches the variables;
	// follow it with Save() to persist, and with UI::ApplyLiveSettings() to show it in game.
	void RestoreDefaults();

	// Re-reads the INI that Init() read, discarding any unsaved change made since. Returns
	// false if the file could not be read, leaving the current values alone.
	bool Reload();

	// Full path of the INI Init() read, or an empty string before Init() has run.
	const std::string& GetIniPath();

	namespace debug
	{
		// Ships at trace by default (project standard) so a submitted log carries the detail
		// needed to diagnose a compatibility, timing or stability report without asking the
		// reporter to change anything first - see CLAUDE.md rule 31.
		inline logger::level logLevel = logger::level::trace;
	}

	namespace automation
	{
		inline bool enableAutoDraw = true;
		inline bool enableAutoSheathe = true;

		// Seconds after leaving combat before the weapon is sheathed, if still not needed.
		// Was an int in milliseconds (ForceSheatheTime, default 6000) in the original mod;
		// converted to a float in seconds to match this project's convention for delay
		// settings (see Compass Navigation Overhaul's fWalkingDelayToShow and siblings).
		inline float sheatheDelaySeconds = 6.0F;

		// New in this fork - the original had no bound-weapon awareness at all. Sheathing a
		// bound (conjured) weapon ends its conjuration early, which is surprising behaviour
		// nobody asked for - so this defaults to skipping the forced sheathe for a bound
		// weapon, exactly as it would be dismissed by its own duration or by the player's own
		// choice instead. Off disables the check entirely, restoring the original behaviour.
		inline bool exemptBoundWeapons = true;
	}
}
