// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "MotionControllerComponent.h"
#include "VRIKBody.h"
#include "VRIKSkeletalMeshTranslator.h"
#include "SeekerRole.generated.h"


class UVRIKBody;
class UVRIKSkeletalMeshTranslator;
class UCameraComponent;
class UMotionControllerComponent;

UCLASS()
class VRIKBODYRUNTIME_API ASeekerRole : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASeekerRole();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* CameraRoot;
	
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* VRCamera;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* MotionControllerLeft;
	
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* MotionControllerRight;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* LeftHandMesh;

	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* RightHandMesh;

	UPROPERTY(Category = Character, VisibleAnywhere ,BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UVRIKBody * VRIKBody;

	UPROPERTY(Category = Character, VisibleAnywhere,BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UVRIKSkeletalMeshTranslator *VRIKSkeletalMeshTranslator;
};
