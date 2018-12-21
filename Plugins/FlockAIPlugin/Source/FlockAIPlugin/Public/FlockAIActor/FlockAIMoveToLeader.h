// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "FlockAIBase.h"
#include "FlockAIMoveToLeader.generated.h"


UCLASS()
class FLOCKAIPLUGIN_API AFlockAIMoveToLeader : public AFlockAIBase
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlockAI|GeneralConfig")
		float distBehindSpeedUpRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlockAI|GeneralConfig")
		TSubclassOf<AFlockAILeader> FlockAILeaderClass;

public:
	AFlockAIMoveToLeader();

	bool SetFlockAILeader();

	//virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:

	AFlockAILeader *MyFlockAILeader;

	virtual void CalcMoveToComponent() override;

	virtual void CalcMoveSpeed(const float DeltaTime) override;
};