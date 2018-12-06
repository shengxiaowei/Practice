// Fill out your copyright notice in the Description page of Project Settings.


#include "CoreMinimal.h"
#include "MyInterface.h"
#include "SingleInterfaceActor.generated.h"

UCLASS()
class PRACTICE_API ASingleInterfaceActor : public AActor ,public IMyInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASingleInterfaceActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	FString GetTestName() override;
	
	
};
