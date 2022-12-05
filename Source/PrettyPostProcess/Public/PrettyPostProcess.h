// Copyright 2021 Escape Entertainment & Froyok

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPrettyPostProcessModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
