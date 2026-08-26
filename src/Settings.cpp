#include "Settings.h"

#define SETTINGFILE_PATH "Data\\SKSE\\Plugins\\AutoDraw.ini"

void Settings::ReadKeyCodeSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<int>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}
void Settings::ReadIntSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, int& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<int>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}
void Settings::ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = static_cast<float>(a_ini.GetDoubleValue(a_sectionName, a_settingName));
	}
}

void Settings::ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting)
{
	const char* bFound = nullptr;
	bFound = a_ini.GetValue(a_sectionName, a_settingName);
	if (bFound) {
		a_setting = a_ini.GetBoolValue(a_sectionName, a_settingName);
	}
}

void Settings::readSettings()
{
	CSimpleIniA ini;
	ini.LoadFile(SETTINGFILE_PATH);

	ReadBoolSetting(ini, "Auto", "EnableAutoDraw", EnableAutoDraw);
	ReadBoolSetting(ini, "Auto", "EnableAutoSheathe", EnableAutoSheathe);
	ReadIntSetting(ini, "Auto", "ForceSheatheTime", ForceSheatheTime);
	if (ForceSheatheTime < 0) {
		ForceSheatheTime = 0;
	}
	if (ForceSheatheTime > 30000) {
		ForceSheatheTime = 30000;
	}

	SwitchTranslation = ini.GetValue("Auto", "SwitchTranslation", "en");
}

bool Settings::SaveSettings(const Settings& s)
{
	CSimpleIniA ini;
	ini.SetUnicode();
	ini.LoadFile(SETTINGFILE_PATH);

	ini.SetBoolValue("Auto", "EnableAutoDraw", s.EnableAutoDraw);
	ini.SetBoolValue("Auto", "EnableAutoSheathe", s.EnableAutoSheathe);
	
	if (s.ForceSheatheTime < 0) {
		s.ForceSheatheTime = 0;
	}
	if (s.ForceSheatheTime > 30000) {
		s.ForceSheatheTime = 30000;
	}
	ini.SetLongValue("Auto", "ForceSheatheTime", s.ForceSheatheTime);

	ini.SetValue("Auto", "SwitchTranslation", s.SwitchTranslation.c_str());
	return ini.SaveFile(SETTINGFILE_PATH) >= 0;
}