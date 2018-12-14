/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once

#include "CoreMinimal.h"
#include "AnimGraphNode_Base.h"
#include "AnimNode_DragonSpineSolver.h"
#include "AnimGraphNode_DragonSpineSolver.generated.h"

/**
 * 
 */
UCLASS()
class DRAGONIKPLUGINEDITOR_API UAnimGraphNode_DragonSpineSolver : public UAnimGraphNode_Base
{
	GENERATED_BODY()
	
		UPROPERTY(EditAnywhere, Category = Settings)
		FAnimNode_DragonSpineSolver ik_node;
public:
	UAnimGraphNode_DragonSpineSolver(const FObjectInitializer& ObjectInitializer);


	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FString GetNodeCategory() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	
	virtual void CreateOutputPins() override;



};
