/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#include "AnimGraphNode_DragonFeetSolver.h"



#include "AnimationGraphSchema.h"



void UAnimGraphNode_DragonFeetSolver::CreateOutputPins()
{
	CreatePin(EGPD_Output, UAnimationGraphSchema::PC_Struct, FComponentSpacePoseLink::StaticStruct(), TEXT("Pose"));
}


UAnimGraphNode_DragonFeetSolver::UAnimGraphNode_DragonFeetSolver(const FObjectInitializer & ObjectInitializer)
{
}

FText UAnimGraphNode_DragonFeetSolver::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(FString("Dragon Foot Solver"));
}

FText UAnimGraphNode_DragonFeetSolver::GetTooltipText() const
{
	return FText::FromString(FString("Responsible for handling foot ik towards the terrain hit data . Find more details in the document ."));
}

FString UAnimGraphNode_DragonFeetSolver::GetNodeCategory() const
{
	return FString("Dragon.IK Plugin");
}

FLinearColor UAnimGraphNode_DragonFeetSolver::GetNodeTitleColor() const
{
	return FLinearColor(10.0f / 255.0f, 127.0f / 255.0f, 248.0f / 255.0f);
}
