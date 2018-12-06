// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "InterfaceComm/MyInterface.h"
#include "MyInterfaceGamemode.generated.h"

/**
 * 
 */
UCLASS()
class PRACTICE_API AMyInterfaceGamemode : public AGameMode
{
	GENERATED_BODY()
public:
	void BeginPlay() override;
public:
	//????UPROPERTY() different to ¡¶UE4 C++ cookbook¡·
	TArray<IMyInterface *> MyInterfaceInstances;
	
};
