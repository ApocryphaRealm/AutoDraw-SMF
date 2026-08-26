#include "UI.h"

#include "SKSEMenuFramework.h"

#include "Settings.h"

#include "utils/Logger.h"
#include "utils/Toggle.h"

#include <algorithm>

namespace UI
{
	namespace
	{
		std::string statusMessage;

		// The slider the arrow keys currently drive. Set by clicking one.
		std::string selectedSlider;

		// Mirrors !selectedSlider.empty(), but readable from OnInputEvent below, which the
		// framework runs on its own input thread - selectedSlider itself is a plain
		// std::string with no such guarantee, so this is the thread-safe version of the same
		// fact. Written by NudgeableSlider on the render thread whenever it selects a slider.
		std::atomic<bool> sliderSelected{ false };

		SKSEMenuFramework::Model::InputEvent* inputHook = nullptr;

		constexpr const char* kLogLevelNames[] = { "Trace", "Debug", "Info", "Warning", "Error", "Critical", "Off" };
		constexpr int kLogLevelCount = 7;

		// The framework renders from the renderer's present hook, which is not the thread the
		// game's own systems expect to be talked to from - anything beyond touching this
		// plugin's own settings variables has to be handed to the main thread first.
		void OnMainThread(std::function<void()> a_task)
		{
			if (auto* taskInterface = SKSE::GetTaskInterface())
			{
				taskInterface->AddTask(std::move(a_task));
			}
		}

		// See CompassNavigationOverhaul/source/UI.cpp's identical check for the reasoning:
		// older SMF builds do not export every cimgui function a page needs, and calling
		// through a null function pointer crashes on the first draw rather than failing to
		// register - so every export this page's widgets resolve at runtime is probed here
		// first, by its *resolved* name (varargs widgets resolve to a "...V"-suffixed export).
		bool HasRequiredExports()
		{
			constexpr const char* required[] = {
				"AddSectionItem",
				"igTextV",
				"igTextDisabledV",
				"igTextWrappedV",
				"igSetTooltipV",
				"igSeparatorText",
				"igCombo_Str_arr",
				"igSliderFloat",
				"igIsItemHovered",
				"igButton",
				"igSameLine",
				"igSpacing",
				"igPushItemWidth",
				"igPopItemWidth",
				// Needed by NudgeableSlider's arrow-key nudge (ported from Dragon's Eye
				// Minimap's UI.cpp - CLAUDE.md rule 24).
				"igIsKeyPressed_Bool",
				"igIsItemClicked",
				"igIsItemActive",
				// Needed by utils/Toggle.h's hand-drawn switch - see that file's own header
				// comment for why a checkbox-style widget needs this much lower-level access.
				"igGetCursorScreenPos",
				"igGetWindowDrawList",
				"igGetFrameHeight",
				"igInvisibleButton",
				"igPushID_Str",
				"igPopID",
				"ImDrawList_AddRectFilled",
				"ImDrawList_AddCircleFilled"
			};

			for (const char* name : required)
			{
				if (!GetMenuFrameworkFunction<void*>(name))
				{
					logger::warn("SKSE Menu Framework does not export \"{}\"", name);

					return false;
				}
			}

			return true;
		}

		// Runs on the framework's input thread (see the note on sliderSelected above for why
		// that matters here). Swallows a left/right arrow press while a NudgeableSlider is
		// selected, so it does not also reach whatever else on-screen treats an arrow key as
		// gamepad-equivalent menu navigation. This only touches the RE::InputEvent the game
		// itself sees; ImGui reads its own key state through the framework's separate hook, so
		// NudgeableSlider's own nudge still happens exactly as before regardless of what this
		// returns. Ported from Dragon's Eye Minimap's UI.cpp - CLAUDE.md rule 24.
		bool __stdcall OnInputEvent(RE::InputEvent* a_event)
		{
			if (!sliderSelected.load())
			{
				return false;
			}

			auto* buttonEvent = a_event ? a_event->AsButtonEvent() : nullptr;

			if (!buttonEvent || !buttonEvent->IsPressed())
			{
				return false;
			}

			if (buttonEvent->GetDevice() != RE::INPUT_DEVICE::kKeyboard)
			{
				return false;
			}

			const auto code = static_cast<std::int32_t>(buttonEvent->GetIDCode());

			if (code != RE::BSKeyboardDevice::Keys::kLeft && code != RE::BSKeyboardDevice::Keys::kRight &&
				code != RE::BSKeyboardDevice::Keys::kUp && code != RE::BSKeyboardDevice::Keys::kDown)
			{
				return false;
			}

			logger::trace("OnInputEvent: swallowing arrow key {} - a slider is selected", code);

			return true;
		}

		// A slider that the arrow keys can also nudge, once it has been clicked. Dragging is
		// hopeless for the last decimal place, and the framework does not turn on ImGui's own
		// keyboard navigation, so this tracks the selection itself rather than changing a
		// setting shared with every other mod's page. Ported verbatim from Dragon's Eye
		// Minimap's UI.cpp, which already had this working - see CLAUDE.md rule 24.
		bool NudgeableSlider(const char* a_label, float* a_value, float a_min, float a_max,
							 const char* a_format, float a_step)
		{
			bool changed = ImGuiMCP::SliderFloat(a_label, a_value, a_min, a_max, a_format);

			if (ImGuiMCP::IsItemClicked() || ImGuiMCP::IsItemActive())
			{
				selectedSlider = a_label;
				sliderSelected.store(true);
			}

			if (selectedSlider == a_label)
			{
				float nudge = 0.0F;

				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_LeftArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_DownArrow))
				{
					nudge -= a_step;
				}
				if (ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_RightArrow) || ImGuiMCP::IsKeyPressed(ImGuiMCP::ImGuiKey_UpArrow))
				{
					nudge += a_step;
				}

				if (nudge != 0.0F)
				{
					*a_value = std::clamp(*a_value + nudge, a_min, a_max);
					changed = true;
				}

				ImGuiMCP::SameLine();
				ImGuiMCP::TextDisabled("<-->");
			}

			return changed;
		}

		void HelpMarker(const char* a_description)
		{
			ImGuiMCP::SameLine();
			ImGuiMCP::TextDisabled("(?)");

			if (ImGuiMCP::IsItemHovered())
			{
				ImGuiMCP::SetTooltip("%s", a_description);
			}
		}

		void RenderAutomationSection()
		{
			using namespace settings::automation;

			ImGuiMCP::SeparatorText("Automation");

			ImGuiMCP::Toggle("Auto-draw on entering combat", &enableAutoDraw);
			HelpMarker("Draws your weapon or magic the instant something targets you in combat.");

			ImGuiMCP::Toggle("Auto-sheathe after combat", &enableAutoSheathe);
			HelpMarker("Sheathes your weapon or magic a set delay after you leave combat, or after you draw it manually.");

			NudgeableSlider("Sheathe delay (seconds)", &sheatheDelaySeconds, 0.0F, 30.0F, "%.2f", 0.1F);
			HelpMarker("How long to wait, after leaving combat or drawing manually, before sheathing - reset by attacking, blocking, jumping or dodging.");

			ImGuiMCP::Toggle("Exempt bound weapons", &exemptBoundWeapons);
			HelpMarker("Skips the forced sheathe while a bound (conjured) weapon is drawn, so it isn't dismissed early - it will still end on its own duration or if you sheathe it yourself.");
		}

		void RenderDebugSection()
		{
			using namespace settings;

			ImGuiMCP::SeparatorText("Debug");

			int level = static_cast<int>(debug::logLevel);
			if (ImGuiMCP::Combo("Log level", &level, kLogLevelNames, kLogLevelCount))
			{
				debug::logLevel = static_cast<logger::level>(level);

				OnMainThread([]() { logger::set_level(settings::debug::logLevel, settings::debug::logLevel); });
			}
			HelpMarker("Applies to the log immediately. Ships at Trace by default - see CLAUDE.md rule 31.");
		}

		void RenderButtons()
		{
			if (ImGuiMCP::Button("Save"))
			{
				OnMainThread([]() {
					statusMessage = settings::Save() ? "Settings saved." : "Could not save the INI. See the log for why.";
				});
			}
			HelpMarker("Writes every setting above back to the INI. Comments and unrelated keys are left alone.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Reload from INI"))
			{
				OnMainThread([]() {
					statusMessage = settings::Reload()
										 ? "Settings reloaded from the INI."
										 : "Could not read the INI. See the log for why.";
				});
			}
			HelpMarker("Throws away any change made here since the last save and re-reads the INI from disk. Also picks up edits made to the file by hand.");

			ImGuiMCP::SameLine();

			if (ImGuiMCP::Button("Restore defaults"))
			{
				OnMainThread([]() { settings::RestoreDefaults(); });

				statusMessage = "Defaults restored. Press Save to keep them.";
			}
			HelpMarker("Puts every setting back to the value it has on a fresh install. Nothing is written until you press Save.");

			if (!statusMessage.empty())
			{
				ImGuiMCP::TextWrapped("%s", statusMessage.c_str());
			}

			ImGuiMCP::Spacing();
			ImGuiMCP::TextDisabled("%s", settings::GetIniPath().c_str());
		}
	}

	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled())
		{
			logger::info("SKSE Menu Framework is not installed; settings will be read from the INI only");

			return;
		}

		if (!HasRequiredExports())
		{
			logger::warn("The installed SKSE Menu Framework is older than this plugin's settings "
						 "menu needs. Update it to a newer version to configure Auto Draw in game.");

			return;
		}

		// Keeps an arrow key nudging a selected slider from also reaching the game's own
		// gamepad-equivalent menu navigation underneath - see OnInputEvent's own comment.
		if (GetMenuFrameworkFunction<void*>("RegisterInpoutEvent"))
		{
			inputHook = SKSEMenuFramework::AddInputEvent(OnInputEvent);
		}
		else
		{
			logger::info("SKSE Menu Framework does not export \"RegisterInpoutEvent\"; an arrow "
						 "key nudging a slider may also move menu navigation underneath");
		}

		SKSEMenuFramework::SetSection("Auto Draw");
		SKSEMenuFramework::AddSectionItem("Settings", SettingsPanel::Render);

		logger::info("Registered the settings page with SKSE Menu Framework");
	}

	void __stdcall SettingsPanel::Render()
	{
		ImGuiMCP::TextWrapped("Most settings apply as soon as you make them. Press Save to keep them for the next time you play.");
		ImGuiMCP::Spacing();

		ImGuiMCP::PushItemWidth(260.0F);

		RenderAutomationSection();
		ImGuiMCP::Spacing();

		RenderDebugSection();
		ImGuiMCP::Spacing();

		ImGuiMCP::PopItemWidth();

		RenderButtons();
	}
}
