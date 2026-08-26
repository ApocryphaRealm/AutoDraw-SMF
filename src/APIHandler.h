#pragma once
#include "PrismaUI/PrismaUI_API.h"
#include "Settings.h"

namespace Req_PrismaUI_API
{
	inline PRISMA_UI_API::IVPrismaUI1* PrismaUI = nullptr;
	inline PrismaView gView = 0;
	static inline Settings gCfg;

	void Request_PrismaUI_API();
	void InitializeUI();
	void SendAll();
	void WritePrismaUIConfig();
};