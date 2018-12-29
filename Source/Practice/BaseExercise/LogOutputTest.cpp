// Fill out your copyright notice in the Description page of Project Settings.

#include "LogOutputTest.h"
#include "Engine/Engine.h"


// Sets default values
ALogOutputTest::ALogOutputTest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ALogOutputTest::BeginPlay()
{
	Super::BeginPlay();
	TCHAR * MyTChar = TEXT("shengxiaowei");
	UE_LOG(LogTemp, Log, TEXT("%s BeginPlay"), TEXT("MyNameIsShengxiaowei"));
	UE_LOG(LogTemp,Warning,TEXT("%d Beginplay"), MyTChar);


	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Black, FString("Fstring GEngine PrintOn Screen"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Error"));
	}
	
	UE_LOG(SXWLogPrint,Warning,TEXT("shengxiaoweihahahahahahah"));
	
}

// Called every frame
void ALogOutputTest::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

