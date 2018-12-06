#pragma once
#include "Interface.h"
#include "KillableInterface.generated.h"

UINTERFACE(meta =(CannotImplementInterfaceInBlueprint))
class PRACTICE_API UKillableInterface :public UInterface
{
	GENERATED_BODY()
};

class PRACTICE_API IKillableInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable,Category = "Kiallable")
	virtual bool IsDead();
	UFUNCTION(BlueprintCallable, Category = "Kiallable")
	virtual void Die();
};

