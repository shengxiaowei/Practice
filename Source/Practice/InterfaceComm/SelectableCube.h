#pragma once

#include "PhysicsCube.h"
#include "SelectableInterface.h"
#include "SelectableCube.generated.h"

UCLASS()
class PRACTICE_API ASelectableCube :public APhysicsCube, public ISelectable
{
	GENERATED_BODY()
public:
	ASelectableCube();
	virtual void NotifyHit(class UPrimitiveComponent* MyComp,AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal,FVector NormalImpulse, const FHitResult& Hit) override;

};



