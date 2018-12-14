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

#include "Animation/InputScaleBias.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_DragonSpineSolver.generated.h"
/**
 * 
 */

 

 
class USkeletalMeshComponent;



UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class ERefPosePluginEnum : uint8
{
	VE_Animated 	UMETA(DisplayName = "Animated Pose"),
	VE_Rest 	UMETA(DisplayName = "Rest Pose")

};


USTRUCT(BlueprintInternalUseOnly)
struct DRAGONIKPLUGIN_API FAnimNode_DragonSpineSolver : public FAnimNode_Base
{
	//	GENERATED_USTRUCT_BODY()
	GENERATED_BODY()



		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InputData, meta = (PinShownByDefault))
		FDragonData_MultiInput dragon_input_data;

	FDragonData_BoneStruct dragon_bone_data;





public:


	/** Tolerance for final tip location delta from EffectorLocation*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float Precision = 0.1f;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MaximumPitch = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MinimumPitch = -30;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MaximumRoll = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		float MinimumRoll = -30;




	/** Maximum number of iterations allowed, to control AdvancedSettings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Solver)
		int32 MaxIterations = 15;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
		FComponentSpacePoseLink ComponentPose;

	// Current strength of the skeletal control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
		mutable float Alpha = 1;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
		mutable float Adaptive_Alpha = 1;




//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (PinShownByDefault))
//		mutable float Vertical_Solver_Upper_Scale = 1;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (PinShownByDefault))
		mutable float Shift_Speed = 10;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
		FInputScaleBias AlphaScaleBias;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings)
		TEnumAsByte<ETraceTypeQuery> Trace_Channel;

	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	* currently transition would be issue and that has to be re-visited
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "LOD Threshold"))
		int32 LODThreshold;


//	virtual int32 GetLODThreshold() const override { return LODThreshold; }

	UPROPERTY(Transient)
		float ActualAlpha = 0;


	int Num_Valid_Spines = 0;



	float component_scale = 1;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Downward Height"))
		float line_trace_downward_height = 350;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Trace Upward Height"))
		float line_trace_upper_height = 350;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Extra dip When going up slopes", PinHiddenByDefault))
		float Slanted_Height_Up_Offset = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Slope Detection Tolerance", PinHiddenByDefault))
		float Slope_Detection_Strength = 25;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Extra dip When going down slopes", PinHiddenByDefault))
		float Slanted_Height_Down_Offset = 0;




	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Slanted height downwards offseting", PinHiddenByDefault))
	//		bool full_extend = true;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Reverse Fabrik", PinHiddenByDefault))
		bool reverse_fabrik = false;



	


//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Upward Push When Side Rotation", PinHiddenByDefault))
		float upward_push_side_rotation = 0;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Calculation Relative To Ref Pose", PinHiddenByDefault))
		bool Calculation_To_RefPose = false;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Extra dip When going up slopes", PinHiddenByDefault))
		float Chest_Slanted_Height_Up_Offset = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Slope Detection Tolerance", PinHiddenByDefault))
		float Chest_Slope_Detection_Strength = 25;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Extra dip When going down slopes", PinHiddenByDefault))
		float Chest_Slanted_Height_Down_Offset = 0;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Z Offset", PinHiddenByDefault))
		float Chest_Base_Offset = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Z Offset", PinHiddenByDefault))
		float Pelvis_Base_Offset = 0;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Body Sideward Rotation Intensity", PinHiddenByDefault))
		float Body_Rotation_Intensity = 0.25f;






	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Alternate Cross Radius", PinHiddenByDefault))
		float virtual_leg_width = 25;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Maximum Dip Height", PinHiddenByDefault))
		float Maximum_Dip_Height = 100;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Extra Forward Trace Offset", PinHiddenByDefault))
		float extra_forward_trace_Offset = 0;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Rotation alpha between end points", PinHiddenByDefault))
		float rotation_power_between = 1;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Automatic Fabrik Selection (Development)", PinHiddenByDefault))
		bool Use_Automatic_Fabrik_Selection = false;



	bool initialize_anim_array = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Forward Direction Vector", PinHiddenByDefault))
		FVector Forward_Direction_Vector = FVector(0,1,0);




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Body Location Lerp Speed", PinHiddenByDefault))
		float Location_Lerp_Speed = 15;






	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Body Rotation Lerp Speed", PinHiddenByDefault))
		float Rotation_Lerp_Speed = 5;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Chest Influence Alpha", PinHiddenByDefault))
		float Chest_Influence_Alpha = 1;





	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Pelvis Forward Rotation Intensity", PinHiddenByDefault))
		float Pelvis_ForwardRotation_Intensity = 0.7f;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Chest Forward Rotation Intensity", PinHiddenByDefault))
		float Chest_ForwardRotation_Intensity = 0.7f;





	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Should solving data persist on fail scenarios", PinHiddenByDefault))
	//		bool should_solving_persist = false;



	//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (DisplayName = "Should move down ?", PinHiddenByDefault))
	//		bool should_move_down = true;


	bool atleast_one_hit = false;

	bool feet_is_empty = true;


/*
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinShownByDefault))
	FTransform DebugEffectorTransform;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinShownByDefault))
	FTransform DebugEffectorTransformTwo;
	*/



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinShownByDefault))
		bool Enable_Solver = true;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = ChestControl, meta = (DisplayName = "Use Alternate Cross-Based Chest Rotation", PinHiddenByDefault))
		bool Use_Fake_Chest_Rotations = false;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PelvisControl, meta = (DisplayName = "Use Alternate Cross-Based Pelvis Rotation", PinHiddenByDefault))
		bool Use_Fake_Pelvis_Rotations = false;


//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		float True_Rotation_Alpha = 1;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool Force_Activation = false;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool accurate_feet_placement = true;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		bool Only_Root_Solve = false;




//	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		float Smooth_Factor = 10;


		bool every_foot_dont_have_child = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AdvancedSettings, meta = (PinHiddenByDefault))
		FVector Overall_PostSolved_Offset = FVector(0,0,0);






	FVector root_location_saved;


	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category = Miscellaneous, meta = (DisplayName = "Solver Reference Pose"))
		ERefPosePluginEnum SolverReferencePose;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Miscellaneous, meta = (DisplayName = "Strict Spine-Feet pair finding", PinHiddenByDefault))
		bool Spine_Feet_Connect = false;





	FBoneContainer* SavedBoneContainer;



	float Root_Roll_Value = 0;

	float Root_Pitch_Value = 0;

	float diff_heights[6];


	TArray<FDragonData_SpineFeetPair> spine_Feet_pair;
	TArray<FHitResult> spine_hit_between;
	TArray<FName> Total_spine_bones;
	TArray<FDragonData_HitPairs> spine_hit_pairs;
	TArray<FHitResult> spine_hit_edges;
	TArray<FVector> spine_vectors_between;
	TArray<FVector> Full_Spine_OriginalLocations;
	TArray<float> Full_Spine_Heights;
	TArray<float> Total_spine_heights;
	TArray<float> Total_spine_alphas;
	TArray<float> Total_spine_angles;
	TArray<FVector> Total_Terrain_Locations;

	TArray<FCompactPoseBoneIndex> Spine_Indices;



	TArray<FCompactPoseBoneIndex> Extra_Spine_Indices;

	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_Transform_pairs;
	TArray<FDragonData_SpineFeetPair_TRANSFORM_WSPACE> spine_AnimatedTransform_pairs;

	TArray<FVector> spine_between_transforms;


	TArray<FVector> spine_between_offseted_transforms;
	TArray<float> spine_between_heights;


	TArray<FVector> snake_spine_positions;
	TArray<FTransform> spine_ChangeTransform_pairs_Obsolete;
	TArray<FVector> spine_LocDifference;
	TArray<FRotator> spine_RotDifference;
	TArray<FBoneTransform> RestBoneTransforms;
	TArray<FBoneTransform> AnimatedBoneTransforms;
	TArray<FBoneTransform> Original_AnimatedBoneTransforms;
	TArray<FBoneTransform> FinalBoneTransforms;
	TArray<FBoneTransform> BoneTransforms;
	TArray<FCompactPoseBoneIndex> combined_indices;



//	TArray<FDragonData_SpineFeetPair_heights> Total_spine_heights;

	float midpoint_height = 0;



	float maximum_spine_length = 0;

	float angle_data = 0;



	FTransform ChestEffectorTransform = FTransform::Identity;

	FTransform RootEffectorTransform = FTransform::Identity;


	int zero_transform_set = 0;

	FTransform Last_RootEffectorTransform = FTransform::Identity;
	FTransform Last_ChestEffectorTransform = FTransform::Identity;





	float spine_median_result = 0;








	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Snake Joint Speed (If snake true)", PinHiddenByDefault))
		float Snake_Joint_Speed = 15;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Is it a snake ?", PinHiddenByDefault))
		bool is_snake = false;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Experimental, meta = (DisplayName = "Use FeetTips(if exist)", PinHiddenByDefault))
		bool Use_FeetTips = false;


	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Maximum Feet-Terrain Fail Distance", PinHiddenByDefault))
		float Maximum_Feet_Distance = 250;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Minimum Terrain-Capsule Activation Distance", PinHiddenByDefault))
		float Minimum_Feet_Distance = 5;




	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = BasicSettings, meta = (DisplayName = "Display LineTracing", PinHiddenByDefault))
		bool DisplayLineTrace = true;




	bool is_single_spine = false;



	FVector SmoothApproach(FVector pastPosition, FVector pastTargetPosition, FVector targetPosition, float speed);


	void Dragon_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases, bool is_reverse);


	void Dragon_VectorCreation(bool isPelvis, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases);


	void Dragon_SingleImpactRotationv2(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases, bool is_reverse);



	FVector RotateAroundPoint(FVector input_point, FVector forward_vector, FVector origin_point, float angle);



	void Snake_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases);


	FName GetChildBone(FName BoneName, USkeletalMeshComponent* skel_mesh);


	TArray<FName> BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot = false);

	bool Check_Loop_Exist(float feet_height, FName start_bone, FName input_bone, TArray<FName>& total_spine_bones);

	TArray<FDragonData_SpineFeetPair> Swap_Spine_Pairs(TArray<FDragonData_SpineFeetPair>& test_list);


	FCollisionQueryParams getDefaultSpineColliParams(FName name, AActor *me, bool debug_mode);

	void line_trace_func(USkeletalMeshComponent& skelmesh, FVector startpoint, FVector endpoint, FHitResult RV_Ragdoll_Hit, FName bone_text, FName trace_tag, FHitResult& Output, FLinearColor debug_color, bool debug_mode = false);


	FComponentSpacePoseContext* saved_pose;

	USkeletalMeshComponent *owning_skel;




	int32 tot_len_of_bones;
	void GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases);
	void GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);



	void Make_All_Bones(FCSPose<FCompactPose>& MeshBases);


	DragonSpineOutput DragonSpineProcessor(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);


	DragonSpineOutput DragonSpineProcessor_Direct(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);



	DragonSpineOutput DragonSpineProcessor_Snake(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);



	DragonSpineOutput Dragon_Transformation(DragonSpineOutput& input, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);

	FRotator BoneRelativeConversion(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);

	FRotator BoneRelativeConversion_LEGACY(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases);


	FVector GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex);



	void FABRIK_BodySystem(FBoneReference TipBone, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms);


	void OrthoNormalize(FVector& Normal, FVector& Tangent);

	FQuat LookRotation(FVector lookAt, FVector upDirection);

	bool solve_should_fail = false;



	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) override;

//	virtual void Evaluate(FPoseContext& Output);

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

