#pragma once
#include <SimpleIni.h>

class Settings
{
public:
	static inline bool EnableAutoDraw = true;
	static inline bool EnableAutoSheathe = true;
	static inline int ForceSheatheTime = 6000;

	static inline std::string SwitchTranslation = "en";


	static void readSettings();
	static bool SaveSettings(const Settings& s);


private:
	static void ReadBoolSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, bool& a_setting);
	static void ReadFloatSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, float& a_setting);
	static void ReadKeyCodeSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, uint32_t& a_setting);
	static void ReadIntSetting(CSimpleIniA& a_ini, const char* a_sectionName, const char* a_settingName, int& a_setting);
};
