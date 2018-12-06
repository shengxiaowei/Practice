#pragma once

#include "KillableInterface.h"
#include"DeadInterface.generated.h"

UINTERFACE()
class PRACTICE_API UDeadInterface :public UKillableInterface
{
	GENERATED_BODY()
};

class PRACTICE_API IDeadInterface :public IKillableInterface
{
	GENERATED_BODY()

public:
	virtual bool IsDead() override;
	virtual void Die() override;
	virtual void Flee();

};