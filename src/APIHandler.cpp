#include "APIHandler.h"

#include <cstdio>

namespace Req_PrismaUI_API
{

	void Request_PrismaUI_API()
	{
		if (!PrismaUI) {
			PrismaUI = static_cast<PRISMA_UI_API::IVPrismaUI1*>(PRISMA_UI_API::RequestPluginAPI(PRISMA_UI_API::InterfaceVersion::V1));
			InitializeUI();
		}
	}

	void InitializeUI()
	{
		if (!PrismaUI)
			return;

		gView = PrismaUI->CreateView("AutoDraw/settings.html", [](PrismaView view) -> void {
			logger::info("PrismaUI is load : {}", view);
			WritePrismaUIConfig();
		});

		PrismaUI->Hide(gView);

		PrismaUI->RegisterJSListener(gView, "applySettings", [](const char* arg) -> void {
			try {
				auto j = json::parse(arg);

				gCfg.EnableAutoDraw = j.value("EnableAutoDraw", gCfg.EnableAutoDraw);
				gCfg.EnableAutoSheathe = j.value("EnableAutoSheathe", gCfg.EnableAutoSheathe);
				gCfg.ForceSheatheTime = j.value("ForceSheatheTime", gCfg.ForceSheatheTime);
				gCfg.SwitchTranslation = j.value("SwitchTranslation", gCfg.SwitchTranslation);

				
				gCfg.SaveSettings(gCfg);

				PrismaUI->Invoke(gView, "showMessage('Update Success!')");
				logger::info("Settings updated successfully");
			} catch (const std::exception& e) {
				logger::info("applySettings json {}", arg);
				logger::error("Failed to parse settings: {}", e.what());
				PrismaUI->Invoke(gView, "showMessage('Update Failed!')");
			}
		});
	}

	void SendAll()
	{
		if (!PrismaUI)
			return;

		if (!PrismaUI->IsValid(gView))
			return;

		json j = {
			{ "EnableAutoDraw", gCfg.EnableAutoDraw },
			{ "EnableAutoSheathe", gCfg.EnableAutoSheathe },
			{ "ForceSheatheTime", gCfg.ForceSheatheTime },
			{ "SwitchTranslation", gCfg.SwitchTranslation }
		};
		auto s = j.dump();
		//logger::info("sss {}", s.c_str());

		PrismaUI->InteropCall(gView, "loadSettings", s.c_str());
	}

	void WritePrismaUIConfig()
	{
		std::filesystem::path basePath = std::filesystem::current_path();
		std::filesystem::path targetPath = basePath / "Data" / "PrismaUI" / "PMCM";
		std::filesystem::create_directories(targetPath);
		std::filesystem::path filePath = targetPath / "AutoDraw.txt";

		json jsonContent = {
			{ "id", "AutoDraw" },
			{ "name", "Auto Draw & Sheathe" },
			{ "pid", std::to_string(gView) },
			{ "desc", "Auto Draw & Sheathe settings." }
		};

		std::ofstream ofs(filePath, std::ios::trunc);
		if (ofs.is_open()) {
			ofs << jsonContent;
			ofs.close();
		}
	}
};