// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LogMacros.h"
#include "LogOutputTest.generated.h"


//classA专属的Category
//classA
//{
//	DECLARE_LOG_CATEGORY_CLASS(CategoryName,Log,All);
//}


//如果想定义只在一个CPP文件中使用的Category，不希望被其他类使用，可以定义一个Static的Category：
//DEFINE_LOG_CATEGORY_STATIC

//声明
DECLARE_LOG_CATEGORY_EXTERN(MyCategoryName,Log,All)


UCLASS()
class PRACTICE_API ALogOutputTest : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALogOutputTest();
	ALogOutputTest(FString in) :DisplayImf(in) {};

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	
private:
	FString DisplayImf;
};
