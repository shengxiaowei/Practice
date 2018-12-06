#pragma once

#include "Interface.h"
#include"AttackAvoidInter.generated.h"

UINTERFACE()
class PRACTICE_API UAttackAvoidInter :public UInterface
{
	GENERATED_BODY()
};

class PRACTICE_API IAttackAvoidInter
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "AvoidAttack")
		void AttackInComming(AActor *TestActor);
};