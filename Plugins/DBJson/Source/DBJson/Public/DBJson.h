// Copyright 2021-2022, DearBing. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FDBJsonModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
