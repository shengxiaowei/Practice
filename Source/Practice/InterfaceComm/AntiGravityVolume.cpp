// Fill out your copyright notice in the Description page of Project Settings.

#include "AntiGravityVolume.h"


// Sets default values
AAntiGravityVolume::AAntiGravityVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(FName("CollisionComponent"));
	CollisionComponent->SetBoxExtent(FVector(200,200,1000));
	CollisionComponent->SetHiddenInGame(false);
	CollisionComponent->SetVisibility(true);
	SetRootComponent(CollisionComponent);
}

// Called when the game starts or when spawned
void AAntiGravityVolume::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAntiGravityVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAntiGravityVolume::NotifyActorBeginOverlap(AActor* OtherActor)
{
	IAntiGravityInterface *InterfaceActor = Cast<IAntiGravityInterface>(OtherActor);
	if (InterfaceActor)
	{
		InterfaceActor->SetDisableGravity();
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString("AntiGravityInterface BeginOverLap"));
	}
}

void AAntiGravityVolume::NotifyActorEndOverlap(AActor* OtherActor)
{
	IAntiGravityInterface *InterfaceActor = Cast<IAntiGravityInterface>(OtherActor);
	if (InterfaceActor)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Green, FString("AntiGravityInterface BeginEndLap"));
		InterfaceActor->SetEnableGravity();
	}
}

