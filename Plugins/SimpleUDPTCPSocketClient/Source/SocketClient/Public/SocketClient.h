// Copyright 2017-2018 David Romanski (Socke). All Rights Reserved.

#pragma once

#include "ModuleManager.h"

#ifndef __SocketClientBPLibrary
#define __SocketClientBPLibrary
#include "SocketClientBPLibrary.h"
#endif

#ifndef __SocketClientHandler
#define __SocketClientHandler
#include "SocketClientHandler.h"
#endif

class FSocketClientModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};