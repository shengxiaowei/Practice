#pragma once
#include "ObjectMacros.h"
#include "Interface.h"
#include "MyInterface.generated.h"
/** */
UINTERFACE()

class PRACTICE_API UMyInterface : public UInterface
{
	GENERATED_BODY()
};
/** */
class PRACTICE_API IMyInterface
{
	GENERATED_BODY()
public:
	virtual FString GetTestName();
};