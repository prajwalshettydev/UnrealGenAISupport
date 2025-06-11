// Copyright (c) 2025 Prajwal Shetty. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the root directory of this
// source tree or http://opensource.org/licenses/MIT.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/**
 *
 */
class GENERATIVEAISUPPORTEDITOR_API FGenerativeAISupportEditorModule : public IModuleInterface
{
  public:
	FGenerativeAISupportEditorModule();

	// IModuleInterface implementation
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

  private:
	void RegisterSettings();
	void UnregisterSettings();
	bool HandleSettingsSaved();
	void RegisterMenuExtension();
	void UnregisterMenuExtension();
	void OnEditorWindowMenuClicked();

	bool bSettingsRegistered;
	TSharedPtr<FUICommandList> PluginCommands;
};