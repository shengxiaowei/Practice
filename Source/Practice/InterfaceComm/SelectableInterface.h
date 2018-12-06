#pragma once

#include "Interface.h"
#include"SelectableInterface.generated.h"

UINTERFACE()
class PRACTICE_API USelectable :public UInterface
{
	GENERATED_BODY()
};

class PRACTICE_API ISelectable
{
	GENERATED_BODY()
public:
	virtual bool IsSelect();
	virtual bool TrySelect();
	virtual void Deselect();
};