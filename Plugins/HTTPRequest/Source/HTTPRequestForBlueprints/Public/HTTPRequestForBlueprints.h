// Copyright (c) 2023 King Wai Mark


#pragma once

#include "Modules/ModuleManager.h"

class FHTTPRequestForBlueprintsModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
