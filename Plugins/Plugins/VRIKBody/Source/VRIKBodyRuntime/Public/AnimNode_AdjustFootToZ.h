// VR IK Body Component
// (c) Yuri N Kalinin, 2017, ykasczc@gmail.com. All right reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_AdjustFootToZ.generated.h"

/* The Two Bone IK control applies an inverse kinematic (IK) solver to a leg 3-joint chain to adjust foot z to desired ground level */
USTRUCT()
struct VRIKBODYRUNTIME_API FAnimNode_AdjustFootToZ : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

public:
	FAnimNode_AdjustFootToZ();

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category = "IK")
	FBoneReference IKBone;

	/** Set to true to disable IK adjustment for ground Z coordinate below current foot Z coordinate ***/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndEffector", meta = (PinShownByDefault))
	bool IgnoreGroundBelowFoot;

	/** Effector Location. Target Location to reach. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EndEffector", meta = (PinShownByDefault))
	float GroundLevelZ;

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
};