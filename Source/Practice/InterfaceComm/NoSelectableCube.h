#pragma once

#include "SelectableCube.h"
#include "NoSelectableCube.generated.h"

UCLASS()
class PRACTICE_API ANoSelectableCube :public ASelectableCube
{
	GENERATED_BODY()
public:
	virtual bool IsSelect() override;
	virtual bool TrySelect() override;
	virtual void Deselect() override;
};

