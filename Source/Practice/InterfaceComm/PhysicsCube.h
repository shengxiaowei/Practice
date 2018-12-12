// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AntiGravityInterface.h"
#include "PhysicsCube.generated.h"

UCLASS()
class PRACTICE_API APhysicsCube : public AActor ,public IAntiGravityInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APhysicsCube();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
public:
	UPROPERTY()
	UStaticMeshComponent *TestMeshCom;
	
	
};