// Fill out your copyright notice in the Description page of Project Settings.

#include "PhysicsCube.h"
#include "ConstructorHelpers.h"


// Sets default values
APhysicsCube::APhysicsCube()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	TestMeshCom = CreateDefaultSubobject<UStaticMeshComponent>(FName("TestMeshCom"));
	auto MeshStatic = ConstructorHelpers::FObjectFinder<UStaticMesh>(TEXT("StaticMesh'/Engine/BasicShapes/Cube.Cube'"));
	if (MeshStatic.Object != nullptr)
	{
		TestMeshCom->SetStaticMesh(MeshStatic.Object);
	}
	TestMeshCom->SetMobility(EComponentMobility::Movable);
	TestMeshCom->SetSimulatePhysics(true);
	TestMeshCom->SetEnableGravity(true);
	SetActorEnableCollision(true);

}

// Called when the game starts or when spawned
void APhysicsCube::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APhysicsCube::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

