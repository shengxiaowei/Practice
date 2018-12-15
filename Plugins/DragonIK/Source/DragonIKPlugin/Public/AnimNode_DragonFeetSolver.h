/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once



#include "DragonIK_Library.h"
#include "CoreMinimal.h"

#include "DragonIKPlugin.h"

#include "Kismet/KismetMathLibrary.h"

#include "CollisionQueryParams.h"

#include "Animation/InputScaleBias.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_DragonFeetSolver.generated.h"
/**
 * 
 */


class USkeletalMeshComponent;




UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class EIK_Type_Plugin : uint8
{
	ENUM_Two_Bone_Ik 	UMETA(DisplayName = "Two Bone IK"),
	ENUM_Single_Bone_Ik 	UMETA(DisplayName = "Single Bone IK")

};



USTRUCT(BlueprintInternalUseOnly)
struct DRAGONIKPLUGIN_API FAnimNode_DragonFeetSolver : public FAnimNode_Base
{
	//	GENERATED_USTRUCT_BODY()
	GENERATED_BODY()



		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InputData, meta = (PinShownByDefault))
		FDragonData_MultiInput dragon_input_data;

	FDragonData_BoneStruct dragon_bone_data;

public:


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		EIK_Type_Plugin ik_type;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
		FComponentSpacePoseLink ComponentPose;

	// Current strength of the skeletal control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
		mutable float Alpha = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
		FInputScaleBias AlphaScaleBias;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Automatic Foot-Knee-Thigh detection", PinHiddenByDefault))
		bool automatic_leg_make = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Enable Solver", PinHiddenByDefault))
		bool enable_solver = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Shift Speed", PinHiddenByDefault))
		float shift_speed = 5;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
		TEnumAsByte<ETraceTypeQuery> Trace_Channel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
		int32 LODThreshold;


	virtual int32 GetLODThreshold() const override { return LODThreshold; }



	UPROPERTY(Transient)
		float ActualAlpha;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EndEffector, meta = (PinShownByDefault))
		FTransform DebugEffectorTransform;


	FBoneContainer* SavedBoneContainer;


	FTransform ChestEffectorTransform = FTransform::Identity;

	FTransform RootEffectorTransform = FTransform::Identity;


	TArray<FBoneReference> feet_bone_array;

	TArray<FTransform> feet_transform_array;


	TArray<TArray<float>> feet_Alpha_array;


	TArray<TArray<FTransform>> feet_mod_transform_array;

	bool atleast_one_hit = false;

	TArray<FHitResult> feet_hit_array;

	bool solve_should_fail = false;





	TArray<FDragonData_SpineFeetPair> spine_Feet_pair;
	TArray<FName> Total_spine_bones;

	bool every_foot_dont_have_child = false;

	FName GetChildBone(FName BoneName, USkeletalMeshComponent* skel_mesh);


	void Leg_ik_Function(FBoneReference ik_footbone, int spine_index, int feet_index, TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms);



	void Leg_Singleik_Function(FBoneReference ik_footbone, int spine_index, int feet_index, TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms);




	void Leg_Full_Function(FName foot_name, int spine_index, int feet_index, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms);

	TArray<FName> BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot = false);


	TArray<FName> BoneArrayMachine_Feet(int32 index, FName starting,FName knee,FName thigh, FName ending, bool is_foot = false);



	bool Check_Loop_Exist(float yaw_offset,float feet_height, FName start_bone, FName knee_bone, FName thigh_bone, FName input_bone, TArray<FName> total_spine_bones);

	TArray<FDragonData_SpineFeetPair> Swap_Spine_Pairs(TArray<FDragonData_SpineFeetPair> test_list);



//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "LineTraceHeight", PinHiddenByDefault))
//		float line_trace_height = 1000;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "Trace Height below feet"))
		float line_trace_down_height = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "Trace Height above feet"))
		float line_trace_upper_height = 350;



//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Rotation_Lerp_Speed", PinHiddenByDefault))
//		float Rotation_Lerp_Speed = 25;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Should Solving Rotate Feet ?", PinHiddenByDefault))
		bool Should_Rotate_Feet = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Use_Feet_Tips(If Exist) ?", PinHiddenByDefault))
		bool Use_Feet_Tips = false;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Show Trace In Game ?", PinHiddenByDefault))
		bool show_trace_in_game = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Automatic Foot Height Detection", PinHiddenByDefault))
		bool Automatic_Foot_Height_Detection = true;



//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Location_Lerp_Speed", PinHiddenByDefault))
//		float Location_Lerp_Speed = 1;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Enable_Pitch", PinHiddenByDefault))
		bool Enable_Pitch = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Enable_Roll", PinHiddenByDefault))
		bool Enable_Roll = true;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Use Ref Pose Rotation as reference", PinHiddenByDefault))
		bool use_ref_rotation = false;

	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (DisplayName = "Maximum Feet-Terrain Distance", PinHiddenByDefault))
		float Vertical_Solver_downward_Scale = 250;



	TArray<FVector> EffectorLocationList;


	TArray<float> Total_spine_heights;
	TArray<FDragonData_HitPairs> spine_hit_pairs;

	TArray<FCompactPoseBoneIndex> Spine_Indices;





	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_Transform_pairs;
	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_AnimatedTransform_pairs;

	TArray<TArray<float>> FeetRootHeights;

	void GetFeetHeights(FComponentSpacePoseContext & Output);


	FCollisionQueryParams getDefaultColliParams(FName name, AActor *me);

	void line_trace_func(USkeletalMeshComponent *skelmesh, FVector startpoint, FVector endpoint, FHitResult RV_Ragdoll_Hit, FName bone_text, FName trace_tag, FHitResult& Output, const FLinearColor& Fdebug_color=FLinearColor::White);


	FComponentSpacePoseContext* saved_pose;

	USkeletalMeshComponent *owning_skel;

	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	int32 tot_len_of_bones;
	void GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);
	TArray<FBoneTransform> BoneTransforms;


	TArray<FCompactPoseBoneIndex> combined_indices;
	void Make_All_Bones(FCSPose<FCompactPose>& MeshBases);


	FRotator BoneRelativeConversion(float feet_data, FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);


	FRotator BoneInverseConversion(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);



	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);


	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;


	virtual void Evaluate_AnyThread(FPoseContext& Output);


protected:
	// Interface for derived skeletal controls to implement
	// use this function to update for skeletal control base
	virtual void UpdateInternal(const FAnimationUpdateContext& Context);
	// use this function to evaluate for skeletal control base
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	// Evaluate the new component-space transforms for the affected bones.
	//	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);


	void LineTraceControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);


	// return true if it is valid to Evaluate
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones);
	// initialize any bone references you have
	virtual void InitializeBoneReferences(FBoneContainer& RequiredBones);

};

