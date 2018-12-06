// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PracticeGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class PRACTICE_API APracticeGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
public:
	void BeginPlay() override;
	
	
};
