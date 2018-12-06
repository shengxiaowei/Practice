// Fill out your copyright notice in the Description page of Project Settings.

#include "PracticeGameModeBase.h"
#include "InterfaceComm/MyInterface.h"
#include "InterfaceComm/SingleInterfaceActor.h"
#include "GameFramework/Actor.h"





void APracticeGameModeBase::BeginPlay()
{
	FTransform Origin;
	ASingleInterfaceActor *TestActor = GetWorld()->SpawnActor<ASingleInterfaceActor>(ASingleInterfaceActor::StaticClass(),Origin);
	if (TestActor->GetClass()->ImplementsInterface(UMyInterface::StaticClass()))
	{
		UE_LOG(LogTemp,Log,TEXT("Has Interface UMyInterface"));
		GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Red, TEXT("Screen Has Interface UMyInterface"));
	}

}
