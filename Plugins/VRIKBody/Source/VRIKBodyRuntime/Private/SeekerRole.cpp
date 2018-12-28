// Fill out your copyright notice in the Description page of Project Settings.

#include "SeekerRole.h"
#include "Components/SkeletalMeshComponent.h"
#include "MotionControllerComponent.h"
#include "Components/SceneComponent.h"


// Sets default values
ASeekerRole::ASeekerRole()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CameraRoot = CreateDefaultSubobject<USceneComponent>(FName(TEXT("CamerRoot")));
	VRCamera = CreateDefaultSubobject<UCameraComponent>(FName(TEXT("VRCamera")));
	MotionControllerLeft = CreateDefaultSubobject<UMotionControllerComponent>(FName(TEXT("MotionControllerLeft")));
	MotionControllerRight = CreateDefaultSubobject<UMotionControllerComponent>(FName(TEXT("MotionControllerRight")));
	LeftHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName(TEXT("LeftHandMesh")));
	RightHandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("RightHandMesh"));

	VRIKBody = CreateDefaultSubobject<UVRIKBody>(FName(TEXT("VRIKBody")));
	VRIKSkeletalMeshTranslator = CreateDefaultSubobject<UVRIKSkeletalMeshTranslator>(FName(TEXT("VRIKSkeletalMeshTranslator")));

	CameraRoot->SetupAttachment(GetRootComponent());

	VRCamera->SetupAttachment(CameraRoot);

	MotionControllerLeft->MotionSource = TEXT("Left");
	MotionControllerLeft->MotionSource = TEXT("Right");

	MotionControllerLeft->SetupAttachment(CameraRoot);

	MotionControllerRight->SetupAttachment(CameraRoot);

	LeftHandMesh->SetupAttachment(MotionControllerLeft);
	RightHandMesh->SetupAttachment(MotionControllerRight);


}

// Called when the game starts or when spawned
void ASeekerRole::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ASeekerRole::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ASeekerRole::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

