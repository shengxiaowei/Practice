// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogMacros.h"
#include "LogOutputTest.generated.h"

DEFINE_LOG_CATEGORY(SXWLogPrint);

//DEFINE_LOG_CATEGORY_CLASS
//DEFINE_LOG_CATEGORY_STATIC
//DECLARE_LOG_CATEGORY_EXTERN()


UCLASS()
class PRACTICE_API ALogOutputTest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALogOutputTest();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
	
};
