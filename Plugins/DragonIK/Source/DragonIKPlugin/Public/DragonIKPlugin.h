/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once

//#include "ModuleManager.h"
#include "Modules/ModuleManager.h"


class FDragonIKPluginModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};