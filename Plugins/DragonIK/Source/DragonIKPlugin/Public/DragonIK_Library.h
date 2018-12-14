/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/

#pragma once

#include "CoreMinimal.h"
//#include "Engine.h"
#include "BoneContainer.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "Kismet/KismetSystemLibrary.h"
#include "BoneContainer.h"

#include "UObject/NoExportTypes.h"
#include "DragonIK_Library.generated.h"










struct FDragonData_BoneStruct
{

	FBoneReference Start_Spine;
	FBoneReference Pelvis;


	TArray<FBoneReference> FeetBones;
	TArray<FBoneReference> KneeBones;
	TArray<FBoneReference> ThighBones;

};



struct FDragonData_HitPairs
{
	FHitResult Parent_Spine_Hit;

	FHitResult Parent_Left_Hit;
	FHitResult Parent_Right_Hit;

	FHitResult Parent_Front_Hit;
	FHitResult Parent_Back_Hit;




	TArray<FHitResult> RV_Feet_Hits;

	TArray<FHitResult> RV_FeetFront_Hits;

	TArray<FHitResult> RV_FeetBack_Hits;

	TArray<FHitResult> RV_FeetLeft_Hits;

	TArray<FHitResult> RV_FeetRight_Hits;




	TArray<FHitResult> RV_FeetBalls_Hits;

	TArray<FHitResult> RV_Knee_Hits;



};



struct FDragonData_SpineFeetPair_TRANSFORM_WSPACE
{

	FTransform Spine_Involved = FTransform::Identity;

	FVector last_spine_location = FVector(0,0,0);

	FVector last_target_location = FVector(0, 0, 0);


	float Z_Offset = 0;


	TArray<FTransform> Associated_Feet = TArray<FTransform>();


	TArray<FTransform> Associated_FeetBalls = TArray<FTransform>();


	TArray<FTransform> Associated_Knee = TArray<FTransform>();


};






struct FDragonData_SpineFeetPair_heights
{

	float Spine_Involved = 0;

	TArray<float> Associated_Feet = TArray<float>();

};





/** Transient structure for FABRIK node evaluation */
struct DragonChainLink
{
public:
	/** Position of bone in component space. */
	FVector Position;

	/** Distance to its parent link. */
	float Length;

	/** Bone Index in SkeletalMesh */
	FCompactPoseBoneIndex BoneIndex;

	/** Transform Index that this control will output */
	int32 TransformIndex;

	/** Child bones which are overlapping this bone.
	* They have a zero length distance, so they will inherit this bone's transformation. */
	TArray<int32> ChildZeroLengthTransformIndices;

	DragonChainLink()
		: Position(FVector::ZeroVector)
		, Length(0.f)
		, BoneIndex(INDEX_NONE)
		, TransformIndex(INDEX_NONE)
	{
	}

	DragonChainLink(const FVector& InPosition, const float& InLength, const FCompactPoseBoneIndex& InBoneIndex, const int32& InTransformIndex)
		: Position(InPosition)
		, Length(InLength)
		, BoneIndex(InBoneIndex)
		, TransformIndex(InTransformIndex)
	{
	}
};




struct DragonSpineOutput
{
public:
	/** Position of bone in component space. */
	TArray<DragonChainLink> chain_data;
	TArray<FCompactPoseBoneIndex> BoneIndices;
	TArray<FBoneTransform> temp_transforms;
	FTransform SpineFirstEffectorTransform;
	FTransform PelvicEffectorTransform;
	FVector RootDifference;
	bool is_moved;
	int32 NumChainLinks;
};




struct FDragonData_SpineFeetPair
{

	FBoneReference Spine_Involved;

	TArray<FBoneReference> Associated_Feet;

	TArray<FBoneReference> Associated_Knees;

	TArray<FBoneReference> Associated_Thighs;



	TArray<FBoneReference> Associated_Feet_Tips;


	TArray<float> Feet_Heights;

	TArray<float> feet_yaw_offset;

	int solver_count = 0;

	FVector total_spine_locdata;

	bool operator == (const FDragonData_SpineFeetPair &pair) const
	{

		if (Spine_Involved.BoneIndex == pair.Spine_Involved.BoneIndex && Associated_Feet.Num() == 0)
		{
			return true;
		}
		else
			return false;


	}

};



USTRUCT(Blueprintable, BlueprintType)
struct FDragonData_FootData
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Feet_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Knee_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Thigh_Bone_Name;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		float Feet_Yaw_Offset;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		float Feet_Heights;

};


USTRUCT(Blueprintable, BlueprintType)
struct FDragonData_MultiInput
{
	GENERATED_BODY()

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Start_Spine;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		FName Pelvis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SkeletalControl, meta = (PinHiddenByDefault))
		TArray<FDragonData_FootData> FeetBones;

};


///////////







UCLASS(Blueprintable, BlueprintType)
class DRAGONIKPLUGIN_API UDragonIK_Library : public UObject
{
	GENERATED_BODY()



		static UDragonIK_Library* Constructor();


public :

	UFUNCTION(BlueprintPure, Category = "SolverFunctions")
		static FRotator CustomLookRotation(FVector lookAt, FVector upDirection);


	void OrthoNormalize(FVector& Normal, FVector& Tangent);

};





/**
 * 

UCLASS()
class DRAGONIKPLUGIN_API UDragonIK_Library : public UObject
{
	GENERATED_BODY()
	
	
	
	
};
*/