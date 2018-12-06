// Fill out your copyright notice in the Description page of Project Settings.

#include "MyInterfaceGamemode.h"
#include "EngineUtils.h"




void AMyInterfaceGamemode::BeginPlay()
{
	for (TActorIterator<AActor>It(GetWorld(),AActor::StaticClass());It;++It) //Not TIterator
	{
		IMyInterface *MyInterfaceInstance = Cast<IMyInterface>(*It);  //Not cast --upper
		if (MyInterfaceInstance)
		{
			MyInterfaceInstances.Add(MyInterfaceInstance);
		}
	}
	//UE_LOG(LogTemp,Log,TEXT("Num of MyInterface :%d"),MyInterfaceInstances.Num());
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, FString::Printf(TEXT("Num of MyInterface :%d Print in MyInterfaceGamemode"),MyInterfaceInstances.Num()));
	}
	
}
