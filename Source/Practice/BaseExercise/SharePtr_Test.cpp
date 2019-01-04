// Fill out your copyright notice in the Description page of Project Settings.

#include "SharePtr_Test.h"
#include<iostream>
#include "LogOutputTest.h"
#include <memory>
using namespace std;
// Sets default values
ASharePtr_Test::ASharePtr_Test()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ASharePtr_Test::BeginPlay()
{
	Super::BeginPlay();
	std::shared_ptr<std::string> logTest(new std::string("SharePtr"));
}

// Called every frame
void ASharePtr_Test::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

