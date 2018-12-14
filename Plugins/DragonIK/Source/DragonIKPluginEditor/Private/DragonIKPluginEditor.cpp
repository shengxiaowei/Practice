/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#include "DragonIKPluginEditor.h"

#define LOCTEXT_NAMESPACE "FDragonIKPluginEditorModule"

void FDragonIKPluginEditorModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FDragonIKPluginEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDragonIKPluginEditorModule, MyEditorPlugin)