#pragma once

#include "Interface.h"
//#include "AntiGravityInterface..generated.h"
#include "AntiGravityInterface.generated.h"
UINTERFACE()
class PRACTICE_API UAntiGravityInterface :public UInterface
{
	GENERATED_BODY()
};

//UCLASS()
class PRACTICE_API IAntiGravityInterface
{
	GENERATED_BODY()
public:
	virtual void SetEnableGravity();
	virtual void SetDisableGravity();
};
