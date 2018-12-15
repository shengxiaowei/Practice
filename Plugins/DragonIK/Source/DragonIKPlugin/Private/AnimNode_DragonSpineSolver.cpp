/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/


#include "AnimNode_DragonSpineSolver.h"
#include "DragonIKPlugin.h"
#include "Animation/AnimInstanceProxy.h"

#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"






// Initialize the component pose as well as defining the owning skeleton
void FAnimNode_DragonSpineSolver::Initialize_AnyThread(const FAnimationInitializeContext & Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	ComponentPose.Initialize(Context);
	owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
	//	dragon_bone_data.Start_Spine = FBoneReference(dragon_input_data.Start_Spine);
}



// Cache the bones . Thats it !!
void FAnimNode_DragonSpineSolver::CacheBones_AnyThread(const FAnimationCacheBonesContext & Context)
{
	FAnimNode_Base::CacheBones_AnyThread(Context);
	ComponentPose.CacheBones(Context);
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
}


// Main update function . Do not perform any change !!
void FAnimNode_DragonSpineSolver::Update_AnyThread(const FAnimationUpdateContext & Context)
{
	ComponentPose.Update(Context);
	ActualAlpha = 0.f;
	if (IsLODEnabled(Context.AnimInstanceProxy))
	{
		EvaluateGraphExposedInputs.Execute(Context);
		// Apply the skeletal control if it's valid
		ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
		if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Context.AnimInstanceProxy->GetSkeleton(), Context.AnimInstanceProxy->GetRequiredBones()))
		{
			UpdateInternal(Context);
		}
	}
}







void FAnimNode_DragonSpineSolver::Make_All_Bones(FCSPose<FCompactPose>& MeshBases)
{


	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

	TArray<FCompactPoseBoneIndex> BoneIndices;

	{
		const FCompactPoseBoneIndex RootIndex = spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(BoneContainer);
		FCompactPoseBoneIndex BoneIndex = spine_Feet_pair[spine_Feet_pair.Num()-1].Spine_Involved.GetCompactPoseIndex(BoneContainer);
		do
		{
			BoneIndices.Insert(BoneIndex, 0);
			BoneIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
		} while (BoneIndex != RootIndex);
		BoneIndices.Insert(BoneIndex, 0);
	}

	combined_indices = BoneIndices;
}




// Store the zero pose transform data
void FAnimNode_DragonSpineSolver::GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases)
{
//	Make_All_Bones(MeshBases);

	RestBoneTransforms.Reset();

	// Gather transforms
	int32 const NumTransforms = combined_indices.Num();
	RestBoneTransforms.AddUninitialized(NumTransforms);
	//	AnimatedBoneTransforms.AddUninitialized(NumTransforms - 1);

	//	Original_AnimatedBoneTransforms.AddUninitialized(NumTransforms - 1);

	if(initialize_anim_array==false)
	AnimatedBoneTransforms.AddUninitialized(NumTransforms);

	Original_AnimatedBoneTransforms.AddUninitialized(NumTransforms);



	for (int i = 0; i < NumTransforms; i++)
	{
		RestBoneTransforms[i] = (FBoneTransform(combined_indices[i], MeshBases.GetComponentSpaceTransform(combined_indices[i])));

		if ((i<NumTransforms - 1 ) && initialize_anim_array==false)
			AnimatedBoneTransforms[i] = RestBoneTransforms[i];
	}

	initialize_anim_array = true;

}


// Store the animated and calculated pose transform data
void FAnimNode_DragonSpineSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	int32 const NumTransforms = combined_indices.Num();
	OutBoneTransforms = TArray<FBoneTransform>();



	for (int i=0;i<Extra_Spine_Indices.Num();i++)
	{

		if (Extra_Spine_Indices[i].GetInt() < Spine_Indices[0].GetInt())
		{

			FTransform updated_transform = MeshBases.GetComponentSpaceTransform(combined_indices[0]);
			FQuat rotDiff = (AnimatedBoneTransforms[0].Transform.Rotator().Quaternion()*updated_transform.Rotator().Quaternion().Inverse()).GetNormalized();
			FVector delta_pos_diff = updated_transform.GetLocation() - AnimatedBoneTransforms[0].Transform.GetLocation();
			FTransform original_transform = MeshBases.GetComponentSpaceTransform(Extra_Spine_Indices[i]);
			original_transform.SetLocation(original_transform.GetLocation() - delta_pos_diff + Overall_PostSolved_Offset);
			original_transform.SetRotation((rotDiff*original_transform.Rotator().Quaternion()));



			OutBoneTransforms.Add(FBoneTransform(Extra_Spine_Indices[i], original_transform));
		}

	}

	for (int i = 0; i < NumTransforms-0; i++)
	{


		if (atleast_one_hit == true)
			Total_spine_alphas[i] = UKismetMathLibrary::FInterpTo(Total_spine_alphas[i], 1, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()),Shift_Speed);
		else
			Total_spine_alphas[i] = UKismetMathLibrary::FInterpTo(Total_spine_alphas[i], 0, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()),Shift_Speed);



//		Total_spine_alphas[i] = 1;

		Total_spine_alphas[i] = FMath::Clamp<float>(Total_spine_alphas[i],0,1);

		FTransform updated_transform = MeshBases.GetComponentSpaceTransform(combined_indices[i]);

		
		if (!RestBoneTransforms[i].Transform.ContainsNaN() && !AnimatedBoneTransforms[i].Transform.ContainsNaN() && (RestBoneTransforms[i].Transform.GetLocation() - updated_transform.GetLocation()).Size()<200 * component_scale && (AnimatedBoneTransforms[i].Transform.GetLocation() - updated_transform.GetLocation()).Size()<10000*component_scale)
		{

			FQuat rotDiff = (updated_transform.Rotator().Quaternion()*RestBoneTransforms[i].Transform.Rotator().Quaternion().Inverse()).GetNormalized();


			FVector delta_pos_diff = updated_transform.GetLocation() - RestBoneTransforms[i].Transform.GetLocation();



			if (Calculation_To_RefPose == false)
			{
				updated_transform.SetRotation(AnimatedBoneTransforms[i].Transform.Rotator().Quaternion());
				updated_transform.SetLocation(AnimatedBoneTransforms[i].Transform.GetLocation() + Overall_PostSolved_Offset);
			}
			else

			{
				updated_transform.SetRotation((rotDiff*AnimatedBoneTransforms[i].Transform.Rotator().Quaternion()));
				updated_transform.SetLocation(delta_pos_diff + AnimatedBoneTransforms[i].Transform.GetLocation() + Overall_PostSolved_Offset);
			}

		}
		
		if (is_single_spine)
		{
			//			if(i<NumTransforms-1)

			if (i == 0)
				OutBoneTransforms.Add(FBoneTransform(combined_indices[i], UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(combined_indices[i]), updated_transform, Total_spine_alphas[i])));
		}
		else
		{
			//			if (i<NumTransforms - 1)
			//			if (i == 0)

			if (Only_Root_Solve)
			{
				if (i == 0)
					OutBoneTransforms.Add(FBoneTransform(combined_indices[i], UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(combined_indices[i]), updated_transform, Total_spine_alphas[i])));
			}
			else
			{
				OutBoneTransforms.Add(FBoneTransform(combined_indices[i], UKismetMathLibrary::TLerp(MeshBases.GetComponentSpaceTransform(combined_indices[i]), updated_transform, Total_spine_alphas[i])));
			}


		}


	}

}


void FAnimNode_DragonSpineSolver::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext & Output)
{



	// Apply the skeletal control if it's valid
	if ((FVector(0, 0, 0) - Output.AnimInstanceProxy->GetActorTransform().GetScale3D()).Size()>0 && spine_Feet_pair.Num()>0 && FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones())  && Output.ContainsNaN() == false)
	{


		ComponentPose.EvaluateComponentSpace(Output);


		for (int i=1;i<combined_indices.Num()-1;i++)
		{

			const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = combined_indices[i];
			FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);

			spine_between_transforms[i-1] = ((ComponentBoneTransform_Local_i)*owning_skel->GetComponentTransform() ).GetLocation();


			FTransform RoottraceTransform = FTransform::Identity;

//			FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), Output.Pose, bonetraceTransform, FCompactPoseBoneIndex(spine_Feet_pair[i].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);

			FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), Output.Pose, RoottraceTransform, ModifyBoneIndex_Local_i, EBoneControlSpace::BCS_WorldSpace);


			float chest_distance = FMath::Abs(spine_between_transforms[i - 1].Z - RoottraceTransform.GetLocation().Z);

			spine_between_heights[i - 1] = chest_distance;

		}




		for (int i = 0; i < spine_Feet_pair.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = spine_Feet_pair[i].Spine_Involved.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
			FVector lerp_data_local_i = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_i.GetLocation());

			spine_AnimatedTransform_pairs[i].Spine_Involved = (ComponentBoneTransform_Local_i)*owning_skel->GetComponentTransform();

			spine_AnimatedTransform_pairs[i].Spine_Involved.SetRotation(owning_skel->GetComponentToWorld().GetRotation()*ComponentBoneTransform_Local_i.GetRotation());
			FVector back_to_front_dir = ((Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation() - (Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer()))).GetLocation()).GetSafeNormal();

			FTransform ComponentBoneTransform_Temp = owning_skel->GetComponentToWorld()*ComponentBoneTransform_Local_i;
			//			FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), Output.Pose, ComponentBoneTransform_Temp, FCompactPoseBoneIndex(spine_Feet_pair[i].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);

			FVector dir_world = owning_skel->GetComponentToWorld().TransformVector(back_to_front_dir);



			//			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " spine rot 2 : " + FRotator(ComponentBoneTransform.GetRotation()).ToString());


			for (int j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
			{


				const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = spine_Feet_pair[i].Associated_Feet[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_j.GetLocation());

				spine_AnimatedTransform_pairs[i].Associated_Feet[j] = (ComponentBoneTransform_Local_j)*owning_skel->GetComponentTransform();



				const FCompactPoseBoneIndex ModifyBoneIndex_Knee_j = spine_Feet_pair[i].Associated_Knees[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Knee_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_j);
//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

				spine_AnimatedTransform_pairs[i].Associated_Knee[j] = (ComponentBoneTransform_Knee_j)*owning_skel->GetComponentTransform();

				
				
				if (every_foot_dont_have_child == false)
				{
					if (spine_Feet_pair[i].Associated_Feet_Tips[j].IsValidToEvaluate())
					{
						const FCompactPoseBoneIndex ModifyBoneIndex_FeetBalls_k = spine_Feet_pair[i].Associated_Feet_Tips[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
						FTransform ComponentBoneTransform_FeetBalls_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_FeetBalls_k);
						//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

						spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j] = (ComponentBoneTransform_FeetBalls_j)*owning_skel->GetComponentTransform();
					}
				}

			}
		}


		if(Calculation_To_RefPose==true)
		Output.ResetToRefPose();


		bool fresh = true;

		if (Total_spine_heights.Num() > 0)
			fresh = false;

		//		Total_spine_heights.Empty();
		//		if (Total_spine_heights.Num() == 0)
		{

			for (int i = 0; i < spine_Feet_pair.Num(); i++)
			{

				FTransform bonetraceTransform = Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[i].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));
				FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());
				FBoneReference root_bone_ref = FBoneReference(owning_skel->GetBoneName(0));
				root_bone_ref.Initialize(*SavedBoneContainer);

				FTransform RoottraceTransform = Output.Pose.GetComponentSpaceTransform(root_bone_ref.GetCompactPoseIndex(*SavedBoneContainer));


				RoottraceTransform.SetLocation(FVector(0, 0, 0));

				FVector root_lerp_data = owning_skel->GetComponentTransform().TransformPosition(RoottraceTransform.GetLocation());

				root_location_saved = root_lerp_data;

				FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), Output.Pose, bonetraceTransform, FCompactPoseBoneIndex(spine_Feet_pair[i].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);

				FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), Output.Pose, RoottraceTransform, FCompactPoseBoneIndex(spine_Feet_pair[i].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);



				float chest_distance = FMath::Abs(bonetraceTransform.GetLocation().Z - RoottraceTransform.GetLocation().Z);





				if (fresh)
				{
					Total_spine_heights.Add(chest_distance);

					spine_LocDifference[i] = lerp_data;
				}
				else
					Total_spine_heights[i] = chest_distance;



			}
		}

		//	ComponentPose.EvaluateComponentSpace(Output);

		LineTraceControl_AnyThread(Output, BoneTransforms);

		//	Output.ResetToRefPose();








		EvaluateComponentSpaceInternal(Output);
		USkeletalMeshComponent* Component = Output.AnimInstanceProxy->GetSkelMeshComponent();
//		AnimatedBoneTransforms.Reset(AnimatedBoneTransforms.Num());

		Original_AnimatedBoneTransforms.Reset(AnimatedBoneTransforms.Num());

		//		FinalBoneTransforms.Reset(FinalBoneTransforms.Num());


		//Get Initial Pose data
		GetResetedPoseInfo(Output.Pose);
		//Reset Bone Transform array
		BoneTransforms.Reset(BoneTransforms.Num());
		saved_pose = &Output;



		EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);

		ComponentPose.EvaluateComponentSpace(Output);

		GetAnimatedPoseInfo(Output.Pose, FinalBoneTransforms);
		checkSlow(!ContainsNaN(FinalBoneTransforms));

		if (FinalBoneTransforms.Num() > 0)
		{

			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
			Output.Pose.LocalBlendCSBoneTransforms(FinalBoneTransforms, BlendWeight);
		}
	}
	else
	{
		ComponentPose.EvaluateComponentSpace(Output);
	}
}

void FAnimNode_DragonSpineSolver::Evaluate_AnyThread(FPoseContext & Output)
{
}



//Perform update operations inside this
void FAnimNode_DragonSpineSolver::UpdateInternal(const FAnimationUpdateContext & Context)
{
	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " spine size : " + FString::SanitizeFloat(spine_Feet_pair.Num()));

	Shift_Speed = FMath::Clamp<float>(Shift_Speed,0,100);

	Location_Lerp_Speed = FMath::Clamp<float>(Location_Lerp_Speed, 0, 100);



	component_scale = owning_skel->GetComponentTransform().GetScale3D().Z;


//	if (is_snake)
//		GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " is snake spine.. ");



	if (spine_Transform_pairs.Num() > 0)
	{

//		if(is_snake)
//			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " spine size : " + FString::SanitizeFloat(spine_Feet_pair.Num()));



		maximum_spine_length = (spine_Transform_pairs[0].Spine_Involved.GetLocation() - spine_Transform_pairs[(spine_hit_pairs).Num() - 1].Spine_Involved.GetLocation()).Size();

		TArray<FVector> feet_mid_points;
		feet_mid_points.AddUninitialized(spine_hit_pairs.Num());


		//FVector dir_forward = -1 * (spine_Transform_pairs[0].Spine_Involved.GetLocation() - spine_Transform_pairs[spine_Transform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal();


		//FVector dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);




		if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
		{
		//	dir_forward = -1 * (spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() - spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal();
		}

		//dir_forward = Forward_Direction_Vector.GetSafeNormal();
		

		FVector dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);

//		dir_forward = owning_skel->GetComponentTransform().TransformVector(dir_forward);


		FVector feet_mid_point = FVector::ZeroVector;
		FVector back_feet_mid_point = FVector::ZeroVector;


		if (is_single_spine == false)
		{
			for (int j = 0; j < spine_Feet_pair[1].Associated_Feet.Num(); j++)
			{
				feet_mid_point += spine_Transform_pairs[1].Associated_Feet[j].GetLocation();
			}

			feet_mid_point = feet_mid_point / 2;
			feet_mid_point.Z = spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation().Z;



			for (int j = 0; j < spine_Feet_pair[0].Associated_Feet.Num(); j++)
			{
				back_feet_mid_point += spine_Transform_pairs[0].Associated_Feet[j].GetLocation();
			}


			back_feet_mid_point = back_feet_mid_point / 2;
			back_feet_mid_point.Z = spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation().Z;

		}
		else
		{

			feet_mid_point = spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation();
			back_feet_mid_point = spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation();
		}



		FVector end_point_sum = spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() + spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation();



		FVector f_offset_local = dir_forward * extra_forward_trace_Offset;

		//			if (i < spine_hit_pairs.Num() - 1)
		//				f_offset = f_offset * 0;


		feet_mid_point = feet_mid_point + f_offset_local;


		for (int i = 0; i < spine_hit_between.Num(); i++)
		{

			FString string_between = "SpineBetween";

			string_between.AppendInt(i);

			


			line_trace_func(*owning_skel
				, spine_between_transforms[i] + FVector(0, 0, line_trace_upper_height * component_scale),
				spine_between_transforms[i] - FVector(0, 0, line_trace_downward_height* component_scale),
				spine_hit_between[i],
				FName(*string_between),
				FName(*string_between),
				spine_hit_between[i], FLinearColor::Blue,
				true);



			/*
			if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
			{


				if (i == 0)
					line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, -0.2f), (UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, -0.2f)) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
				else
					if (i == 1)
						line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
					else
						if (i == 2)
							line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.2f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.20f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
						else
							if (i == 3)
								line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.4f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.4f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
							else
								if (i == 4)
									line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.6f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.6f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
								else if (i == 5)
									line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.8f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.8f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
								else
									if (i == 6)
										line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
									else
										if (i == 7)
											line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1.2f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1.2f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);



			}
			else
			{

				if (i == 0)
					line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, -0.2f), (UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, -0.2f)) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
				else
					if (i == 1)
						line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
					else
						if (i == 2)
							line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.2f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.20f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
						else
							if (i == 3)
								line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.4f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.4f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
							else
								if (i == 4)
									line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.6f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.6f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
								else if (i == 5)
									line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.8f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 0.8f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
								else
									if (i == 6)
										line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);
									else
										if (i == 7)
											line_trace_func(*owning_skel, UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1.2f), UKismetMathLibrary::VLerp(feet_mid_point, back_feet_mid_point, 1.2f) - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_between[i], "", "", spine_hit_between[i], FLinearColor::Blue);



			}
			*/



		}





		for (int32 i = 0; i < spine_hit_pairs.Num(); i++)
		{




			//	Total_spine_heights.Add(chest_distance);


			//if (spine_Transform_pairs[i].Associated_Feet.Num() > 0 && spine_hit_pairs[i].RV_Feet_Hits.Num() > 0)
			{
				for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
				{


					/*
					FVector tr_x = spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation();



					FVector f_offset_j = dir_forward * extra_forward_trace_Offset;

					if (i < spine_hit_pairs.Num() - 1)
						f_offset_j = f_offset_j * 0;

					FVector foot_location = spine_Transform_pairs[i].Associated_Feet[j].GetLocation() + f_offset_j;

					if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
					{
						foot_location = spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation() + f_offset_j;
					}
					*/


//					for (int j = 0; j<spine_Feet_pair.Num(); j++)
//						for (int i = 0; i<spine_Feet_pair[j].Associated_Feet.Num(); i++)
					
					
				//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Child Bone " + GetChildBone(spine_Feet_pair[i].Associated_Feet[j].BoneName, owning_skel).ToString());



					//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Child Bone " + GetChildBone(spine_Feet_pair[0].Associated_Feet[0].BoneName, owning_skel));

					//	owning_skel->BoneIsChildOf

					/*

					FVector feet_direction = (spine_AnimatedTransform_pairs[i].Associated_Knee[j].GetLocation()  - spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation() ).GetSafeNormal();


					line_trace_func(*owning_skel
						, foot_location + (feet_direction*line_trace_upper_height * component_scale),
						foot_location - (feet_direction*line_trace_downward_height* component_scale),
						spine_hit_pairs[i].RV_Feet_Hits[j],
						spine_Feet_pair[i].Associated_Feet[j].BoneName,
						spine_Feet_pair[i].Associated_Feet[j].BoneName
						, spine_hit_pairs[i].RV_Feet_Hits[j], FLinearColor::Red);
						*/

					if (every_foot_dont_have_child == false)
					{
						line_trace_func(*owning_skel
							, spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j].GetLocation() + FVector(0, 0, line_trace_upper_height * component_scale),
							spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j].GetLocation() - FVector(0, 0, line_trace_downward_height* component_scale),
							spine_hit_pairs[i].RV_FeetBalls_Hits[j],
							spine_Feet_pair[i].Associated_Feet_Tips[j].BoneName,
							spine_Feet_pair[i].Associated_Feet_Tips[j].BoneName
							, spine_hit_pairs[i].RV_FeetBalls_Hits[j], FLinearColor::Red,true);
							
					}



					line_trace_func(*owning_skel
						, spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation() + FVector(0, 0, line_trace_upper_height * component_scale),
						spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation() - FVector(0, 0, line_trace_downward_height* component_scale),
						spine_hit_pairs[i].RV_Feet_Hits[j],
						spine_Feet_pair[i].Associated_Feet[j].BoneName,
						spine_Feet_pair[i].Associated_Feet[j].BoneName
						, spine_hit_pairs[i].RV_Feet_Hits[j], FLinearColor::Red, true);

					if (spine_hit_pairs[i].RV_Feet_Hits[j].bBlockingHit)
					{
						feet_mid_points[i] = spine_hit_pairs[i].RV_Feet_Hits[j].ImpactPoint;
					}
				}
			}




			FVector feet_centre_point = FVector(0, 0, 0);

			for (int j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
			{
				if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
					feet_centre_point += spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation();
				else
					feet_centre_point += spine_Transform_pairs[i].Associated_Feet[j].GetLocation();
			}


			feet_centre_point /= spine_AnimatedTransform_pairs[i].Associated_Feet.Num();


			if (is_snake)
			{
//				if (is_snake)

				feet_centre_point = spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation();



//				GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " spine transform : " + feet_centre_point.ToString());

			}


		//	if (i == 0)
			{


				FVector f_offset_i = -dir_forward * 1;

				//				if (i != 0)
				//					f_offset = dir_forward * 1;


				if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
				{
					//					FVector feet_centre_point = (spine_AnimatedTransform_pairs[i].Associated_Feet[0].GetLocation()+ spine_AnimatedTransform_pairs[i].Associated_Feet[1].GetLocation())/2;





					line_trace_func(*owning_skel, feet_centre_point + FVector(0, 0, line_trace_upper_height * component_scale), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green, true);


					//					line_trace_func(*owning_skel, spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height * component_scale), spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() + f_offset_i - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green);


					FVector forward_dir = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector).GetSafeNormal();

					FVector right_dir = UKismetMathLibrary::GetRightVector(forward_dir.Rotation());



					//FVector right_dir = UKismetMathLibrary::GetRightVector((spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() - spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal().Rotation())*-1;

					//FVector forward_dir = spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() - spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal();

					if (every_foot_dont_have_child == false)
					{

					

					line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (right_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Left_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Left_Hit, FLinearColor::Yellow, true);
					line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (right_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Right_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Right_Hit, FLinearColor::Yellow, true);



					line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (forward_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (forward_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Front_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Front_Hit, FLinearColor::Green, true);
					line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (forward_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (forward_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Back_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Back_Hit, FLinearColor::Green, true);

					}

					//					line_trace_func(*owning_skel, spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (right_dir * component_scale * virtual_leg_width), spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Left_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Left_Hit, FLinearColor::Green);
					//					line_trace_func(*owning_skel, spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (right_dir * component_scale * virtual_leg_width), spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Right_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Right_Hit, FLinearColor::Green);

				}
				else
				{

					//FVector feet_centre_point = (spine_Transform_pairs[i].Associated_Feet[0].GetLocation() + spine_Transform_pairs[i].Associated_Feet[1].GetLocation()) / 2;


					line_trace_func(*owning_skel, feet_centre_point + FVector(0, 0, line_trace_upper_height * component_scale), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green, true);


					//					line_trace_func(*owning_skel, spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height * component_scale), spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green);


					//FVector right_dir = UKismetMathLibrary::GetRightVector((spine_Transform_pairs[0].Spine_Involved.GetLocation() - spine_Transform_pairs[spine_Transform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal().Rotation())*-1;


					FVector forward_dir = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector).GetSafeNormal();

					FVector right_dir = UKismetMathLibrary::GetRightVector(forward_dir.Rotation());




					if (every_foot_dont_have_child == false)
					{
						line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (right_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Left_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Left_Hit, FLinearColor::Yellow, true);
						line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (right_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Right_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Right_Hit, FLinearColor::Yellow, true);

						line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (forward_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (forward_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Front_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Front_Hit, FLinearColor::Green, true);
						line_trace_func(*owning_skel, feet_centre_point + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (forward_dir * component_scale * virtual_leg_width), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (forward_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Back_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Back_Hit, FLinearColor::Green, true);
					}


					//line_trace_func(*owning_skel, spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) + (right_dir * component_scale * virtual_leg_width), spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i - FVector(0, 0, line_trace_downward_height * 1 * component_scale) + (right_dir * component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Left_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Left_Hit, FLinearColor::Green);
					//line_trace_func(*owning_skel, spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i + FVector(0, 0, line_trace_upper_height*component_scale) - (right_dir * component_scale * virtual_leg_width), spine_Transform_pairs[i].Spine_Involved.GetLocation() + f_offset_i - FVector(0, 0, line_trace_downward_height * 1 * component_scale) - (right_dir*component_scale * virtual_leg_width), spine_hit_pairs[i].Parent_Right_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Right_Hit, FLinearColor::Green);

				}

			}
		/*	else
			{

				feet_centre_point += dir_forward * extra_forward_trace_Offset;



				if (SolverReferencePose == ERefPosePluginEnum::VE_Animated)
				{



					line_trace_func(*owning_skel, feet_centre_point + FVector(0, 0, line_trace_upper_height * component_scale), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green);

				}
				else
				{

					line_trace_func(*owning_skel, feet_centre_point + FVector(0, 0, line_trace_upper_height * component_scale), feet_centre_point - FVector(0, 0, line_trace_downward_height * 1 * component_scale), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit, FLinearColor::Green);


				}
				*/

//			}

		}

	}

//	for (int j = 0; j<spine_Feet_pair.Num(); j++)
//	for(int i=0;i<spine_Feet_pair[j].Associated_Feet.Num();i++)
//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Child Bone " + GetChildBone(spine_Feet_pair[j].Associated_Feet[i].BoneName, owning_skel).ToString());


	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Child Bone " + GetChildBone(spine_Feet_pair[0].Associated_Feet[0].BoneName, owning_skel));

//	owning_skel->BoneIsChildOf

}






FName FAnimNode_DragonSpineSolver::GetChildBone(FName BoneName, USkeletalMeshComponent* skel_mesh)
{

	FName child_bone = skel_mesh->GetBoneName(skel_mesh->GetBoneIndex(BoneName) +1);
		
	return child_bone;
	
}


//Nothing would be needed here
void FAnimNode_DragonSpineSolver::EvaluateComponentSpaceInternal(FComponentSpacePoseContext & Context)
{
}



void FAnimNode_DragonSpineSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	if (spine_hit_pairs.Num() > 0)
	{




		atleast_one_hit = false;



		for (int k = 0; k < spine_hit_between.Num(); k++)
		{
			//-2.5f

			//			if ((spine_hit_between[k].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > 0* FMath::Abs(Vertical_Solver_downward_Scale)*component_scale)




			if ((spine_hit_between[k].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) >  -Minimum_Feet_Distance *component_scale)
				//			if(spine_hit_between[k].bBlockingHit)
			{
				atleast_one_hit = true;


			}
		}


		for (int k = 0; k < spine_hit_pairs.Num(); k++)
		{



			//			if ((spine_hit_pairs[k].Parent_Spine_Hit.ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > 0* FMath::Abs(Vertical_Solver_downward_Scale)*component_scale)




			if ((spine_hit_pairs[k].Parent_Spine_Hit.ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) >  -Minimum_Feet_Distance *component_scale)
				//			if(spine_hit_pairs[k].Parent_Spine_Hit.bBlockingHit)
			{
				atleast_one_hit = true;


			}

			for (int i = 0; i < spine_hit_pairs[k].RV_Feet_Hits.Num(); i++)
			{

				//				if ((spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > 0 * FMath::Abs(Vertical_Solver_downward_Scale)*component_scale)




				if ((spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > -Minimum_Feet_Distance *component_scale)
				{
					atleast_one_hit = true;
				}

				if (every_foot_dont_have_child == false)
				{
					if ((spine_hit_pairs[k].RV_FeetBalls_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > -Minimum_Feet_Distance * component_scale)
					{
						atleast_one_hit = true;
					}
				}

			}
		}


		for (int k = 0; k < spine_hit_pairs.Num(); k++)
		{
			for (int i = 0; i < spine_hit_pairs[k].RV_Feet_Hits.Num(); i++)
			{
				//				if (FMath::Abs(spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > Total_spine_heights[k] * Vertical_Solver_downward_Scale*component_scale)




				if (FMath::Abs(spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > Maximum_Feet_Distance*component_scale)
				{
					atleast_one_hit = false;
				}

			}
		}


		if (Force_Activation)
			atleast_one_hit = true;


		FABRIK_BodySystem(spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved, Output.Pose, OutBoneTransforms);

	}


}

bool LineTraceInitialized = false;

void FAnimNode_DragonSpineSolver::LineTraceControl_AnyThread(FComponentSpacePoseContext & Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	LineTraceInitialized = false;

	if (spine_hit_pairs.Num() > 0)
	{

		for (int32 i = 0; i < spine_Feet_pair.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = spine_Feet_pair[i].Spine_Involved.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform_Local_i;
			ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
			FVector lerp_data_local_i = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_i.GetLocation());


			spine_Transform_pairs[i].Spine_Involved = FTransform(lerp_data_local_i);

			for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = spine_Feet_pair[i].Associated_Feet[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Local_j;
				ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_j.GetLocation());
				spine_Transform_pairs[i].Associated_Feet[j] = FTransform(lerp_data_local_j);





				const FCompactPoseBoneIndex ModifyBoneIndex_Knee_j = spine_Feet_pair[i].Associated_Knees[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Knee_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_j);
				//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

				spine_Transform_pairs[i].Associated_Knee[j] = (ComponentBoneTransform_Knee_j)*owning_skel->GetComponentTransform();


				if (every_foot_dont_have_child == false)
				{

					if ((&spine_Feet_pair[i].Associated_Feet_Tips[j]) != nullptr)
						if (spine_Feet_pair[i].Associated_Feet_Tips[j].IsValidToEvaluate())
						{
							const FCompactPoseBoneIndex ModifyBoneIndex_FeetBalls_k = spine_Feet_pair[i].Associated_Feet_Tips[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
							FTransform ComponentBoneTransform_FeetBalls_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_FeetBalls_k);
							//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

							spine_Transform_pairs[i].Associated_FeetBalls[j] = (ComponentBoneTransform_FeetBalls_j)*owning_skel->GetComponentTransform();
						}
				}




				//	if (j == spine_Feet_pair[i].Associated_Feet.Num() - 1)
				{
					LineTraceInitialized = true;
				}

			}

		}
	}
}



bool FAnimNode_DragonSpineSolver::IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones)
{

	bool feet_is_true = true;

	for (int i = 0; i < dragon_bone_data.FeetBones.Num(); i++)
	{
		if (dragon_bone_data.FeetBones.Num() == dragon_input_data.FeetBones.Num())
			if (FBoneReference(dragon_bone_data.FeetBones[i]).IsValidToEvaluate(RequiredBones) == false)
			{
				feet_is_true = false;
			}

	}

	if (is_snake)
		feet_is_true = true;


//	return true;

	

	return (RequiredBones.IsValid()&&solve_should_fail == false && feet_is_true &&
		dragon_bone_data.Start_Spine.IsValidToEvaluate(RequiredBones) &&
		dragon_bone_data.Pelvis.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.BoneIsChildOf(FBoneReference(dragon_bone_data.Start_Spine).BoneIndex, FBoneReference(dragon_bone_data.Pelvis).BoneIndex));


	//return false;

}







void FAnimNode_DragonSpineSolver::InitializeBoneReferences(FBoneContainer & RequiredBones)
{
	
	SavedBoneContainer = &RequiredBones;

	Total_spine_bones.Empty();

	spine_Feet_pair.Empty();

	spine_hit_pairs.Empty();

	spine_Transform_pairs.Empty();

	spine_AnimatedTransform_pairs.Empty();


	spine_LocDifference.Empty();

	spine_RotDifference.Empty();


	Total_spine_heights.Empty();

	Total_spine_alphas.Empty();

	spine_hit_between.Empty();

	spine_hit_edges.Empty();

	Total_spine_bones = BoneArrayMachine(0,dragon_input_data.Start_Spine, dragon_input_data.Pelvis, false);

	Algo::Reverse(Total_spine_bones);

	solve_should_fail = false;

	//	spine_Feet_pair.AddDefaulted(Total_spine_bones.Num());





	for (int32 i = 0; i < dragon_input_data.FeetBones.Num(); i++)
	{

	for (int32 j = 0; j < dragon_input_data.FeetBones.Num(); j++)
	{
	if (i != j)
	{
	if (dragon_input_data.FeetBones[i].Feet_Bone_Name == dragon_input_data.FeetBones[j].Feet_Bone_Name)
	{
	solve_should_fail = true;
	}
	}
	}

	BoneArrayMachine(i,dragon_input_data.FeetBones[i].Feet_Bone_Name, dragon_input_data.Pelvis, true);

	}
	Spine_Indices.Empty();




	Total_spine_angles.Empty();
	Total_Terrain_Locations.Empty();

	const TArray<FDragonData_SpineFeetPair> const_feet_pair = spine_Feet_pair;

	Total_spine_alphas.AddDefaulted(const_feet_pair.Num());

	for (int32 i = 0; i < const_feet_pair.Num(); i++)
	{
	Total_spine_alphas[i] = 0;

	if (const_feet_pair[i].Associated_Feet.Num() == 0 && i < const_feet_pair.Num())
	spine_Feet_pair.Remove(const_feet_pair[i]);



	}

	spine_Feet_pair.Shrink();

	if (spine_Feet_pair.Num() == 1)
	{

	FDragonData_SpineFeetPair data = FDragonData_SpineFeetPair();
	data.Spine_Involved = FBoneReference(Total_spine_bones[Total_spine_bones.Num() - 1]);
	data.Spine_Involved.Initialize(*SavedBoneContainer);

	spine_Feet_pair.Add(data);

	is_single_spine = true;

	//		GEngine->AddOnScreenDebugMessage(-1, 5.01f, FColor::Red, "Is single spine");


	bool is_swapped = false;

	do
	{
	is_swapped = false;

	const TArray< FDragonData_SpineFeetPair > spine_Feet_pair_const = spine_Feet_pair;


	for (int32 j = 0; j < spine_Feet_pair.Num(); j++)
	{

	for (int32 i = 1; i < spine_Feet_pair[j].Associated_Feet.Num(); i++)
	{
	if (spine_Feet_pair[j].Associated_Feet[i - 1].BoneIndex < spine_Feet_pair[j].Associated_Feet[i].BoneIndex)
	{
	FBoneReference temp = spine_Feet_pair[j].Associated_Feet[i - 1];
	spine_Feet_pair[j].Associated_Feet[i - 1] = spine_Feet_pair[j].Associated_Feet[i];
	spine_Feet_pair[j].Associated_Feet[i] = temp;

	is_swapped = true;

	}
	}
	}

	} while (is_swapped == true);

	}
	else
	{
	is_single_spine = false;
	spine_Feet_pair = Swap_Spine_Pairs(spine_Feet_pair);

	}


	dragon_bone_data.Start_Spine = FBoneReference(dragon_input_data.Start_Spine);
	dragon_bone_data.Start_Spine.Initialize(RequiredBones);



	dragon_bone_data.Pelvis = FBoneReference(dragon_input_data.Pelvis);
	dragon_bone_data.Pelvis.Initialize(RequiredBones);



	if (spine_Feet_pair.Num() == 0)
	{

		if (is_snake && dragon_bone_data.Pelvis.IsValidToEvaluate() && dragon_bone_data.Start_Spine.IsValidToEvaluate() )
		{

			spine_Feet_pair.AddDefaulted(2);

			spine_Feet_pair[0].Spine_Involved = dragon_bone_data.Pelvis;

			if (spine_Feet_pair.Num() > 1)
				spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved = dragon_bone_data.Start_Spine;

			/*
			FDragonData_SpineFeetPair pelvisdata = FDragonData_SpineFeetPair();
			pelvisdata.Spine_Involved = FBoneReference(dragon_input_data.Pelvis);
			pelvisdata.Spine_Involved.Initialize(*SavedBoneContainer);
			if(pelvisdata.Spine_Involved.IsValidToEvaluate())
			spine_Feet_pair.Add(pelvisdata);


			FDragonData_SpineFeetPair Chestdata = FDragonData_SpineFeetPair();
			Chestdata.Spine_Involved = FBoneReference(dragon_input_data.Start_Spine);
			Chestdata.Spine_Involved.Initialize(*SavedBoneContainer);
			if (Chestdata.Spine_Involved.IsValidToEvaluate())
				spine_Feet_pair.Add(Chestdata);
				*/

		}

	}


	spine_hit_pairs.AddDefaulted(spine_Feet_pair.Num());
	spine_Transform_pairs.AddDefaulted(spine_Feet_pair.Num());

	spine_AnimatedTransform_pairs.AddDefaulted(spine_Feet_pair.Num());


	Total_spine_angles.AddDefaulted(spine_Feet_pair.Num());
	Total_Terrain_Locations.AddDefaulted(spine_Feet_pair.Num());
	spine_LocDifference.AddDefaulted(spine_Feet_pair.Num());
	spine_RotDifference.AddDefaulted(spine_Feet_pair.Num());

	//	Total_spine_alphas.AddDefaulted(spine_Feet_pair.Num());



//	spine_hit_between.AddDefaulted(7);










	for (int32 i = 0; i < spine_Feet_pair.Num(); i++)
	{

		spine_Feet_pair[i].Associated_Feet_Tips.AddUninitialized(spine_Feet_pair[i].Associated_Feet.Num());

		spine_Feet_pair[i].Associated_Knees.AddUninitialized(spine_Feet_pair[i].Associated_Feet.Num());




	if (spine_Feet_pair[i].Associated_Feet.Num() % 2 != 0)
	{
	if(is_snake==false)
	solve_should_fail = true;
	}


	every_foot_dont_have_child = false;
	feet_is_empty = true;



//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Child Bone " + GetChildBone(spine_Feet_pair[i].Associated_Feet[j].BoneName, owning_skel).ToString());

	for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
	{

		FName child_name = GetChildBone(spine_Feet_pair[i].Associated_Feet[j].BoneName, owning_skel);

		if (owning_skel->BoneIsChildOf(child_name,spine_Feet_pair[i].Associated_Feet[j].BoneName))
		{

			FBoneReference FootBall_Involved = FBoneReference(child_name);
			FootBall_Involved.Initialize(*SavedBoneContainer);

			spine_Feet_pair[i].Associated_Feet_Tips[j] = FootBall_Involved;
		}
		else
		{
			spine_Feet_pair[i].Associated_Feet_Tips[j] = spine_Feet_pair[i].Associated_Feet[j];

			every_foot_dont_have_child = true;
		}

		FBoneReference Knee_Involved = FBoneReference(owning_skel->GetParentBone( spine_Feet_pair[i].Associated_Feet[j].BoneName)  );
		Knee_Involved.Initialize(*SavedBoneContainer);

		spine_Feet_pair[i].Associated_Knees[j] = Knee_Involved;


	}


	spine_Feet_pair[i].Feet_Heights.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

	spine_hit_pairs[i].RV_Feet_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
	spine_hit_pairs[i].RV_FeetBalls_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
	spine_hit_pairs[i].RV_Knee_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());



	spine_Transform_pairs[i].Associated_Feet.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

	spine_Transform_pairs[i].Associated_Knee.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

	spine_Transform_pairs[i].Associated_FeetBalls.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());



	spine_AnimatedTransform_pairs[i].Associated_Feet.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

	spine_AnimatedTransform_pairs[i].Associated_Knee.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

	spine_AnimatedTransform_pairs[i].Associated_FeetBalls.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());



	 if (spine_Feet_pair[i].Associated_Feet.Num() > 0)
	 {
		 feet_is_empty = false;
	 }
	
	}



	


	if (dragon_input_data.Start_Spine == dragon_input_data.Pelvis)
	solve_should_fail = true;

	if (spine_Feet_pair.Num() > 0)
	{
	if (spine_Feet_pair[0].Spine_Involved.BoneIndex > spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.BoneIndex)
	solve_should_fail = true;
	}



	initialize_anim_array = false;


	Extra_Spine_Indices.Empty();


	if (spine_Feet_pair.Num() > 0 && Spine_Feet_Connect == false)
	{
		spine_Feet_pair[0].Spine_Involved = dragon_bone_data.Pelvis;

		if (spine_Feet_pair.Num() > 1)
			spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved = dragon_bone_data.Start_Spine;
	}




	for (int32 i = 0; i < Total_spine_bones.Num(); i++)
	{
		FBoneReference bone_ref = FBoneReference(Total_spine_bones[i]);
		bone_ref.Initialize(RequiredBones);

		if (spine_Feet_pair.Num() > 0)
		{
			if (bone_ref.BoneIndex >= spine_Feet_pair[0].Spine_Involved.BoneIndex && bone_ref.BoneIndex <= spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.BoneIndex)
				Spine_Indices.Add(bone_ref.GetCompactPoseIndex(RequiredBones));
			else
			{
				Extra_Spine_Indices.Add(bone_ref.GetCompactPoseIndex(RequiredBones));

//				GEngine->AddOnScreenDebugMessage(-1, 4.01f, FColor::Red, "Extra Bone " + FString::SanitizeFloat(Extra_Spine_Indices[i].GetInt()) );

			}


		}
		else
		{
			Spine_Indices.Add(bone_ref.GetCompactPoseIndex(RequiredBones));

		}

	}



	spine_between_transforms.Empty();

	spine_between_heights.Empty();

	combined_indices.Empty();
   
	if (spine_Feet_pair.Num() > 1)
	{

		TArray<FCompactPoseBoneIndex> BoneIndices;

		{
			const FCompactPoseBoneIndex RootIndex = spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer);
			FCompactPoseBoneIndex BoneIndex = spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer);
			do
			{
				BoneIndices.Insert(BoneIndex, 0);

				//owning_skel->GetBoneIndex( owning_skel->GetParentBone(owning_skel->GetBoneName(BoneIndex.GetInt())));
				BoneIndex = FCompactPoseBoneIndex(owning_skel->GetBoneIndex(owning_skel->GetParentBone(owning_skel->GetBoneName(BoneIndex.GetInt()))));
			} while (BoneIndex != RootIndex);
			BoneIndices.Insert(BoneIndex, 0);
		}



		combined_indices = BoneIndices;
		spine_between_transforms.AddDefaulted(combined_indices.Num()-2);
		spine_hit_edges.AddDefaulted(combined_indices.Num()-2);
		spine_between_offseted_transforms.AddDefaulted(combined_indices.Num()-2);

		spine_between_heights.AddDefaulted(combined_indices.Num() - 2);

		snake_spine_positions.AddDefaulted(combined_indices.Num());

	}

	
	spine_hit_between.AddDefaulted(spine_between_transforms.Num());



	//	dragon_bone_data.FeetBones.Add(dragon_input_data.FeetBones.Num() );
	dragon_bone_data.FeetBones.Empty();

	for (int i = 0; i < dragon_input_data.FeetBones.Num(); i++)
	{
	//dragon_bone_data.FeetBones[i] = FBoneReference(dragon_input_data.FeetBones[i]);
	dragon_bone_data.FeetBones.Add(FBoneReference(dragon_input_data.FeetBones[i].Feet_Bone_Name));
	dragon_bone_data.FeetBones[i].Initialize(RequiredBones);

	if(dragon_bone_data.FeetBones[i].IsValidToEvaluate(RequiredBones))
	feet_is_empty = false;


	}

}


TArray<FDragonData_SpineFeetPair> FAnimNode_DragonSpineSolver::Swap_Spine_Pairs(TArray<FDragonData_SpineFeetPair>& test_list)
{



	bool is_swapped = false;

	do
	{
		is_swapped = false;

		for (int32 j = 1; j < test_list.Num(); j++)
		{

			for (int32 i = 1; i < test_list[j].Associated_Feet.Num(); i++)
			{
				if (test_list[j].Associated_Feet[i - 1].BoneIndex < test_list[j].Associated_Feet[i].BoneIndex)
				{
					FBoneReference temp = test_list[j].Associated_Feet[i - 1];
					test_list[j].Associated_Feet[i - 1] = test_list[j].Associated_Feet[i];
					test_list[j].Associated_Feet[i] = temp;

					is_swapped = true;
				}
			}
		}

	} while (is_swapped == true);


	return test_list;

}


TArray<FName> FAnimNode_DragonSpineSolver::BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot)
{

	TArray<FName> spine_bones;

	int iteration_count = 0;

	spine_bones.Add(starting);

	if (is_foot == false)
	{
		FDragonData_SpineFeetPair data = FDragonData_SpineFeetPair();
		data.Spine_Involved = FBoneReference(starting);
		data.Spine_Involved.Initialize(*SavedBoneContainer);
		spine_Feet_pair.Add(data);



	}


	bool finish = false;

	do
	{


		if (is_foot)
		{
			if (Check_Loop_Exist(dragon_input_data.FeetBones[index].Feet_Heights,starting, spine_bones[spine_bones.Num() - 1], Total_spine_bones))
			{
				return spine_bones;
			}
		}



		iteration_count++;
		spine_bones.Add(owning_skel->GetParentBone(spine_bones[iteration_count - 1]));


		if (is_foot == false)
		{

			FDragonData_SpineFeetPair data = FDragonData_SpineFeetPair();
			data.Spine_Involved = FBoneReference(spine_bones[spine_bones.Num() - 1]);
			data.Spine_Involved.Initialize(*SavedBoneContainer);
			spine_Feet_pair.Add(data);


		}


		if (spine_bones[spine_bones.Num() - 1] == ending && is_foot == false)
		{

			return spine_bones;
		}

	} while (iteration_count < 50 && finish == false);


	return spine_bones;


}


bool FAnimNode_DragonSpineSolver::Check_Loop_Exist(float feet_height,FName start_bone, FName input_bone, TArray<FName>& total_spine_bones)
{

	for (int32 i = 0; i<total_spine_bones.Num(); i++)
	{
		if (input_bone.ToString().TrimStartAndEnd().Equals(total_spine_bones[i].ToString().TrimStartAndEnd()))
		{

			{
				FDragonData_SpineFeetPair data = FDragonData_SpineFeetPair();
				data.Spine_Involved = FBoneReference(total_spine_bones[i]);
				data.Spine_Involved.Initialize(*SavedBoneContainer);

				FBoneReference foot_bone = FBoneReference(start_bone);
				foot_bone.Initialize(*SavedBoneContainer);
				data.Associated_Feet.Add(foot_bone);

				spine_Feet_pair[i].Spine_Involved = data.Spine_Involved;
				spine_Feet_pair[i].Associated_Feet.Add(foot_bone);
				spine_Feet_pair[i].Feet_Heights.Add(feet_height);


				return true;
			}
		}
	}

	return false;
}







FCollisionQueryParams FAnimNode_DragonSpineSolver::getDefaultSpineColliParams(FName name, AActor *me, bool debug_mode)
{

	const FName TraceTag(name);

	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, me);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bTraceAsyncScene = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;
	RV_TraceParams.TraceTag = TraceTag;

	//	if(debug_mode)
	//	me->GetWorld()->DebugDrawTraceTag = TraceTag;


	return RV_TraceParams;
}


void FAnimNode_DragonSpineSolver::line_trace_func(USkeletalMeshComponent &skelmesh, FVector startpoint, FVector endpoint, FHitResult RV_Ragdoll_Hit, FName bone_text, FName trace_tag, FHitResult& Output, FLinearColor debug_color, bool debug_mode)
{

	/*
	skelmesh->GetWorld()->LineTraceSingleByChannel(RV_Ragdoll_Hit,        //result
	startpoint,    //start
	endpoint, //end
	ECC_WorldStatic, //collision channel
	getDefaultParams(trace_tag, skelmesh->GetOwner()));

	TArray<AActor*> selfActor;
	//	selfActor.Add(skelmesh->GetOwner());

	FCollisionShape shape = FCollisionShape();

	shape.SetBox(FVector(10,10,5));

	skelmesh->GetWorld()->SweepSingleByChannel(RV_Ragdoll_Hit,        //result
	startpoint,    //start
	endpoint, //end
	FRotator(0,0,0).Quaternion(),
	ECC_WorldStatic, //collision channel
	shape,
	getDefaultParams(trace_tag, skelmesh->GetOwner()));
	*/


	TArray<AActor*> ignoreActors;

	if (owning_skel->GetOwner())
	{
		ignoreActors.Add(owning_skel->GetOwner());
	}


	if (owning_skel->GetWorld()->IsGameWorld())
	{
//		if(DisplayLineTrace)
//			UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::ForOneFrame, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.01f);
//		else
//		UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::None, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.01f);

		UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::None, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.2f);

	}
	else
	{
		if (DisplayLineTrace)
		UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::ForOneFrame, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.2f);
		else
			UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::None, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.2f);

	}



	//UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::None, RV_Ragdoll_Hit, true, debug_color, FLinearColor::Yellow, 0.01f);



	/*
	skelmesh->GetWorld()->LineTraceSingleByChannel(RV_Ragdoll_Hit,        //result
	startpoint,    //start
	endpoint, //end
	ECC_WorldStatic, //collision channel
	getDefaultParams(trace_tag, skelmesh->GetOwner(), debug_mode));
	*/

	Output = RV_Ragdoll_Hit;

}











void FAnimNode_DragonSpineSolver::FABRIK_BodySystem(FBoneReference TipBone, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{





	/*
	for (int i = 1; i < spine_hit_between.Num(); i++)
	{


		diff_heights[i - 1] = spine_hit_between[i].ImpactPoint.Z - spine_hit_between[i - 1].ImpactPoint.Z;

		diff_heights[i - 1] = FMath::Abs(diff_heights[i - 1]);

	}
	*/


	/*
	bool are_distances_swapped = false;

	do
	{
		are_distances_swapped = false;

		for (int i = 1; i < spine_hit_between.Num() - 1; i++)
		{
			if (diff_heights[i - 1] > diff_heights[i])
			{
				float temp = diff_heights[i - 1];
				diff_heights[i - 1] = diff_heights[i];
				diff_heights[i] = temp;

				are_distances_swapped = true;
			}

		}
	} while (are_distances_swapped);
	*/

//	spine_median_result = UKismetMathLibrary::FInterpTo_Constant(spine_median_result, FMath::Abs(diff_heights[5] - ((diff_heights[2] + diff_heights[3]) / 2.0) * 2) / component_scale, owning_skel->GetWorld()->DeltaTimeSeconds, 50);


	spine_median_result = 10;



	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Median = " + FString::SanitizeFloat(spine_median_result)+" for scale "+FString::SanitizeFloat(owning_skel->GetComponentTransform().GetScale3D().Z));



	if (is_snake ==false)
	{

		if (is_single_spine)
		{
			Dragon_ImpactRotation(0, RootEffectorTransform, MeshBases, false);
		}
		else
			Dragon_ImpactRotation(0, RootEffectorTransform, MeshBases, false);
	}
	else
	{
		Snake_ImpactRotation(0,RootEffectorTransform,MeshBases);
	}


	if (is_snake ==false)
	{
		if (is_single_spine)
		{
			Dragon_ImpactRotation(spine_hit_pairs.Num() - 1, ChestEffectorTransform, MeshBases, false);
		}
		else
			Dragon_ImpactRotation(spine_hit_pairs.Num() - 1, ChestEffectorTransform, MeshBases, false);	
	}
	else
	{
		Snake_ImpactRotation(spine_hit_pairs.Num() - 1, ChestEffectorTransform, MeshBases);
	}


//	1,2,3,4,5,6,7
//	2,3,4,5,6


	for (int i = 0; i < combined_indices.Num(); i++)
	{

		if (i > 0 && i < combined_indices.Num()-1)
		{

			FTransform result = FTransform::Identity;
			FVector new_location = (spine_hit_between[i-1].ImpactPoint + FVector(0, 0, spine_between_heights[i-1]));
			FTransform bonetraceTransform = MeshBases.GetComponentSpaceTransform(combined_indices[i]);
			FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());


			FRotator bone_rotation = FRotator(owning_skel->GetComponentRotation());

			if (owning_skel->GetOwner() != nullptr)
			{
				bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
			}


			FRotator new_rotator = FRotator(0, bone_rotation.Yaw, 0);
			result.SetLocation(lerp_data);



			if (spine_hit_between[i-1].bBlockingHit)
				result.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_between[i-1].ImpactPoint.Z + spine_between_heights[i - 1]));





			spine_between_offseted_transforms[i-1] = owning_skel->GetComponentTransform().InverseTransformPosition( result.GetLocation() );

		}

	}


	/*
	if(spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation().Z!=0)
	RootEffectorTransform.SetLocation(spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation());

	//	if (DebugEffectorTransform.GetLocation().Z > 0 && DebugEffectorTransform.ContainsNaN() == false)
	//		RootEffectorTransform.SetLocation(DebugEffectorTransform.GetLocation());

	RootEffectorTransform.SetRotation(FRotator(0, RootEffectorTransform.Rotator().Yaw,0).Quaternion());



	if (spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num()-1].Spine_Involved.GetLocation().Z != 0)
	ChestEffectorTransform.SetLocation(spine_AnimatedTransform_pairs[spine_AnimatedTransform_pairs.Num()-1].Spine_Involved.GetLocation());

	//	if (DebugEffectorTransformTwo.GetLocation().Z > 0 && DebugEffectorTransformTwo.ContainsNaN() == false)
	//		ChestEffectorTransform.SetLocation(DebugEffectorTransformTwo.GetLocation());


	ChestEffectorTransform.SetRotation(FRotator(0, ChestEffectorTransform.Rotator().Yaw, 0).Quaternion());
	*/

	DragonSpineOutput output_test;



	if (is_snake==false)
	{

		if (is_single_spine == false)
		{

			//if (Use_Fake_Rotations)

			if (Use_Automatic_Fabrik_Selection == false)
			{
				if (reverse_fabrik)
					output_test = DragonSpineProcessor(RootEffectorTransform, MeshBases, OutBoneTransforms);
				else
					output_test = DragonSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
			}
			else
			{
				if (spine_hit_pairs[0].Parent_Spine_Hit.ImpactPoint.Z > spine_hit_pairs[spine_hit_pairs.Num() - 1].Parent_Spine_Hit.ImpactPoint.Z)
					output_test = DragonSpineProcessor(RootEffectorTransform, MeshBases, OutBoneTransforms);
				else
					output_test = DragonSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
			}

		}
		else
		{
			output_test = DragonSpineProcessor_Direct(ChestEffectorTransform, MeshBases, OutBoneTransforms);
		}

	}
	else
	{
		output_test = DragonSpineProcessor_Snake(ChestEffectorTransform, MeshBases, OutBoneTransforms);
		
	}

	output_test.SpineFirstEffectorTransform = ChestEffectorTransform;

	output_test.PelvicEffectorTransform = RootEffectorTransform;



	TArray<DragonChainLink> Chain = output_test.chain_data;

	int32 NumChainLinks = output_test.NumChainLinks;

	TArray<FCompactPoseBoneIndex> BoneIndices = output_test.BoneIndices;




	output_test = Dragon_Transformation(output_test, MeshBases, OutBoneTransforms);



	TArray<FBoneTransform> finalised_bone_transforms;

	TArray<FCompactPoseBoneIndex> finalised_bone_indices;


	//	for (int32 LinkIndex = 0; LinkIndex < output_test.NumChainLinks - 1; LinkIndex++)
	for (int32 LinkIndex = 0; LinkIndex < output_test.NumChainLinks; LinkIndex++)
	{
		DragonChainLink const & CurrentLink = output_test.chain_data[LinkIndex];

		FTransform const &CurrentBoneTransform = output_test.temp_transforms[CurrentLink.TransformIndex].Transform;

//		FTransform LerpBoneTransform = CurrentBoneTransform;
	
//		LerpBoneTransform.SetLocation(UKismetMathLibrary::VInterpTo(AnimatedBoneTransforms[CurrentLink.TransformIndex].Transform.GetLocation(), CurrentBoneTransform.GetLocation(), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()),Location_Lerp_Speed));

		//	const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);




		AnimatedBoneTransforms[CurrentLink.TransformIndex].Transform = CurrentBoneTransform;

		Original_AnimatedBoneTransforms[CurrentLink.TransformIndex].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);



		// Update zero length children if any
		int32 const NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
		for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
		{

			//			DragonChainLink const & CurrentLink = output_test.chain_data[LinkIndex];

			//			FTransform& CurrentBoneTransform = output_test.temp_transforms[CurrentLink.TransformIndex].Transform;


			FTransform& ChildBoneTransform = output_test.temp_transforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform;
			ChildBoneTransform.NormalizeRotation();



			//FTransform LerpBoneTransform = CurrentBoneTransform;

			//LerpBoneTransform.SetLocation(UKismetMathLibrary::VInterpTo(AnimatedBoneTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform.GetLocation(), CurrentBoneTransform.GetLocation(), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed));


			AnimatedBoneTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform = CurrentBoneTransform;

			Original_AnimatedBoneTransforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);


		}


	}



}





void FAnimNode_DragonSpineSolver::Dragon_VectorCreation(bool isPelvis, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases)
{

	int spine_feet_index = 0;

	if (isPelvis == false)
		spine_feet_index = spine_Feet_pair.Num() - 1;

	FTransform bonetraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[spine_feet_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));
	FVector bone_world_location = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());

	FRotator bone_rotation = owning_skel->GetComponentRotation();

	if (owning_skel->GetOwner())
	{
		bone_rotation = owning_skel->GetOwner()->GetActorRotation();
	}


	FTransform final_output_transform = FTransform::Identity;

	final_output_transform.SetLocation(bone_world_location);

	final_output_transform.SetRotation(FRotator(0, bone_rotation.Yaw, 0).Quaternion());



	if (atleast_one_hit)
	{
		if (spine_hit_pairs[spine_feet_index].RV_Feet_Hits.Num() % 2 == 0 && spine_hit_pairs[spine_feet_index].RV_Feet_Hits.Num() > 0)
		{

			FVector lowest_leg_hit = FVector(0, 0, 0);


			//lowest_leg_hit = spine_hit_pairs[spine_feet_index].RV_Feet_Hits[0].ImpactPoint;

			lowest_leg_hit = spine_hit_pairs[spine_feet_index].Parent_Spine_Hit.ImpactPoint;



			//			if(isPelvis==false)
			//				GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Chest : " + FString::SanitizeFloat(Total_spine_heights[spine_feet_index]));


			FVector lifted_position = lowest_leg_hit + FVector(0, 0, Total_spine_heights[spine_feet_index]);


			//			FVector lifted_position = lowest_leg_hit;

			if (isPelvis == false)
			{
				//				lifted_position = DebugEffectorTransform.GetLocation();

				//	final_output_transform.SetLocation(DebugEffectorTransform.GetLocation());
			}

			//			if (isPelvis == false)
			//				lifted_position.Z = lowest_leg_hit.Z;


			final_output_transform.SetLocation(FVector(final_output_transform.GetLocation().X, final_output_transform.GetLocation().Y, lifted_position.Z));


		}
	}




	OutputTransform = final_output_transform;


}








void FAnimNode_DragonSpineSolver::Snake_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases)
{

	FTransform result = FTransform::Identity;

	FVector new_location = (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint + FVector(0, 0, Total_spine_heights[origin_point_index]));

	FTransform bonetraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[origin_point_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));

	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());




	FRotator bone_rotation = FRotator(owning_skel->GetComponentRotation());

	if (owning_skel->GetOwner() != nullptr)
	{
		bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
	}


	FRotator final_rot = FRotator(0, bone_rotation.Yaw, 0);



	//	result.SetLocation(lerp_data);









	FVector pointing_to_transformpoint = (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint + FVector(0, 0, Total_spine_heights[origin_point_index]));

	FRotator position_Based_rot = final_rot;

	FVector Feet_Mid_Point = spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint;


	if (origin_point_index == 0)
	{




		FVector forward_direction = FVector();
		FVector right_direction = FVector();
		float Intensifier_forward = 0;

		float Intensifier_side = 0;


		if (owning_skel->GetOwner())
		{


			//					Intensifier_forward = FMath::Clamp<float>((Feet_Mid_Point.Z - (Feet_Opposite_Mid_Point.Z)) * Pelvis_ForwardRotation_Intensity, -10000, 10000)  *-1;

			//					Intensifier_forward = FMath::Lerp<float>(Intensifier_forward, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));



				if (spine_hit_pairs[origin_point_index].Parent_Front_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Back_Hit.bBlockingHit)
					Intensifier_forward = (spine_hit_pairs[origin_point_index].Parent_Front_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Back_Hit.ImpactPoint.Z)* Pelvis_ForwardRotation_Intensity;
				else
					Intensifier_forward = 0;
			

			forward_direction = owning_skel->GetOwner()->GetActorForwardVector() * Intensifier_forward;


			forward_direction.Z = 0;






			//					Intensifier_side = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z - spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)*direction* Body_Rotation_Intensity *-1;



				if (spine_hit_pairs[origin_point_index].Parent_Left_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Right_Hit.bBlockingHit)
					Intensifier_side = (spine_hit_pairs[origin_point_index].Parent_Left_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Right_Hit.ImpactPoint.Z)* Body_Rotation_Intensity;
				else
					Intensifier_side = 0;



			right_direction = owning_skel->GetOwner()->GetActorRightVector()*Intensifier_side;



		}



		//final_transformation.TransformVector(owning_skel->GetForwardVector() )
		FVector relativeUp = (owning_skel->GetForwardVector()*-1);

		FVector relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + forward_direction)).GetSafeNormal();

		//				FVector relativePos = (lerp_data - DebugEffectorTransform.GetLocation()).GetSafeNormal();



		FRotator look_rot = FRotator(UDragonIK_Library::CustomLookRotation(relativePos, relativeUp));

		position_Based_rot = look_rot;


		position_Based_rot.Yaw = final_rot.Yaw;


		Root_Roll_Value = position_Based_rot.Roll;

		Root_Pitch_Value = position_Based_rot.Pitch;


		FVector dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);


		FVector Roll_relativeUp = (owning_skel->GetForwardVector()*-1);


		FVector Roll_relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + right_direction)).GetSafeNormal();


		//FVector Roll_relativePos = (pointing_to_transformpoint - DebugEffectorTransform.GetLocation()).GetSafeNormal();



		FRotator Roll_look_rot = FRotator(UDragonIK_Library::CustomLookRotation(Roll_relativePos, Roll_relativeUp));


		position_Based_rot.Roll = Roll_look_rot.Roll;


		spine_RotDifference[origin_point_index].Yaw = position_Based_rot.Yaw;




		if (atleast_one_hit)
		{

			if (atleast_one_hit && position_Based_rot.Pitch < MaximumPitch && position_Based_rot.Pitch > MinimumPitch)
				spine_RotDifference[origin_point_index].Pitch = position_Based_rot.Pitch;
			else if (position_Based_rot.Pitch > MaximumPitch)
			{
				spine_RotDifference[origin_point_index].Pitch = MaximumPitch;
			}
			else if (position_Based_rot.Pitch < MinimumPitch)
			{
				spine_RotDifference[origin_point_index].Pitch = MinimumPitch;
			}




			if (atleast_one_hit && position_Based_rot.Roll < MaximumRoll && position_Based_rot.Roll > MinimumRoll)
				spine_RotDifference[origin_point_index].Roll = position_Based_rot.Roll;
			else if (position_Based_rot.Roll > MaximumRoll)
			{

				spine_RotDifference[origin_point_index].Roll = MaximumRoll;
			}
			else if (position_Based_rot.Roll < MinimumRoll)
			{
				spine_RotDifference[origin_point_index].Roll = MinimumRoll;
			}



		}



		//				OutputTransform.SetRotation(UKismetMathLibrary::RLerp(FRotator(OutputTransform.Rotator().Pitch, position_Based_rot.Yaw, OutputTransform.Rotator().Roll), spine_RotDifference[origin_point_index], owning_skel->GetWorld()->DeltaTimeSeconds * Body_Rotation_Interpolation_Speed, true).Quaternion());


	}
	else if (origin_point_index == spine_Transform_pairs.Num() - 1)
	{
		FVector forward_direction = FVector();

		FVector right_direction = FVector();


		float forward_intensity = 0;

		float right_intensity = 0;



		if (owning_skel->GetOwner())
		{



				if (spine_hit_pairs[origin_point_index].Parent_Front_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Back_Hit.bBlockingHit)
					forward_intensity = (spine_hit_pairs[origin_point_index].Parent_Front_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Back_Hit.ImpactPoint.Z)* Chest_ForwardRotation_Intensity;
				else
					forward_intensity = 0;




			forward_direction = owning_skel->GetOwner()->GetActorForwardVector() * forward_intensity;





				if (spine_hit_pairs[origin_point_index].Parent_Left_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Right_Hit.bBlockingHit)
					right_intensity = (spine_hit_pairs[origin_point_index].Parent_Left_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Right_Hit.ImpactPoint.Z)* Body_Rotation_Intensity;
				else
					right_intensity = 0;


			//					right_direction = owning_skel->GetComponentToWorld().InverseTransformVector(owning_skel->GetRightVector()) ;

			right_direction = owning_skel->GetOwner()->GetActorRightVector()*right_intensity;



			forward_direction.Z = 0;
		}



		FVector relativeUp = (owning_skel->GetForwardVector()*-1);
		FVector relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + forward_direction)).GetSafeNormal();


		//FVector relativePos = (pointing_to_transformpoint - DebugEffectorTransform.GetLocation()).GetSafeNormal();


		FRotator look_rot = FRotator(UDragonIK_Library::CustomLookRotation(relativePos, relativeUp));

		position_Based_rot = look_rot;

		position_Based_rot.Yaw = final_rot.Yaw;
		//				position_Based_rot.Roll = final_rot.Roll;



		FVector Roll_relativeUp = (owning_skel->GetForwardVector()*-1);

		FVector Roll_relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + right_direction)).GetSafeNormal();



		FRotator Roll_look_rot = FRotator(UDragonIK_Library::CustomLookRotation(Roll_relativePos, Roll_relativeUp));


		position_Based_rot.Roll = Roll_look_rot.Roll;


		spine_RotDifference[origin_point_index].Yaw = position_Based_rot.Yaw;
		//			position_Based_rot = UKismetMathLibrary::RLerp(FRotator(0, final_rot.Yaw, 0), position_Based_rot, atleast_one_hit, true);





		if (atleast_one_hit)
		{

			if (atleast_one_hit && position_Based_rot.Pitch < MaximumPitch && position_Based_rot.Pitch > MinimumPitch)
				spine_RotDifference[origin_point_index].Pitch = position_Based_rot.Pitch;
			else if (position_Based_rot.Pitch > MaximumPitch)
			{
				spine_RotDifference[origin_point_index].Pitch = MaximumPitch;
			}
			else if (position_Based_rot.Pitch < MinimumPitch)
			{
				spine_RotDifference[origin_point_index].Pitch = MinimumPitch;
			}




			if (atleast_one_hit && position_Based_rot.Roll < MaximumRoll && position_Based_rot.Roll > MinimumRoll)
				spine_RotDifference[origin_point_index].Roll = position_Based_rot.Roll;
			else if (position_Based_rot.Roll > MaximumRoll)
			{

				spine_RotDifference[origin_point_index].Roll = MaximumRoll;
			}
			else if (position_Based_rot.Roll < MinimumRoll)
			{
				spine_RotDifference[origin_point_index].Roll = MinimumRoll;
			}


		}


	}




	FRotator current_rot = FRotator(OutputTransform.Rotator().Pitch, final_rot.Yaw, OutputTransform.Rotator().Roll).GetNormalized();
	//FRotator target_rot = FRotator(spine_RotDifference[origin_point_index].Pitch, final_rot.Yaw, spine_RotDifference[origin_point_index].Roll).GetNormalized();



	//FRotator target_rot = FRotator(UKismetMathLibrary::Lerp<float>( spine_RotDifference[origin_point_index].Pitch, spine_RotDifference[origin_point_index].Pitch,0,adapt), final_rot.Yaw, spine_RotDifference[origin_point_index].Roll).GetNormalized();

	FRotator target_rot = FRotator(UKismetMathLibrary::Lerp(0, spine_RotDifference[origin_point_index].Pitch, Adaptive_Alpha), final_rot.Yaw, UKismetMathLibrary::Lerp(0, spine_RotDifference[origin_point_index].Roll, Adaptive_Alpha)).GetNormalized();

	{
		if (atleast_one_hit && Enable_Solver == true)
		{

			OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(current_rot, target_rot, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Rotation_Lerp_Speed* 0.05f).Quaternion().GetNormalized());
		}
		else
		{
			target_rot = FRotator(0, target_rot.Yaw, 0);
			//					OutputTransform.SetRotation(target_rot.Quaternion().GetNormalized());

			OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(current_rot, target_rot, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Rotation_Lerp_Speed* 0.05f).Quaternion().GetNormalized());
		}
	}







	if (spine_hit_pairs.Num() > origin_point_index - 1)
	{





		FVector forward_dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);

		FVector right_dir_vector = FVector::CrossProduct(forward_dir_forward, FVector(0, 0, 1));

		FVector spine_position = FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]);



		FVector pitch_based_location = RotateAroundPoint(spine_position, right_dir_vector, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Pitch);




		float Z_Offset = 10000000000;


		/*
		for (int32 i = 0; i < spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num(); i++)
		{

			if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint.Z < Z_Offset)
			{
					Z_Offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint.Z;
			}

		}
		*/
		
		if (Z_Offset == 10000000000)
		{
			if(spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit)
			 Z_Offset = spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z;
			else
				Z_Offset = lerp_data.Z - Total_spine_heights[origin_point_index];
		
		}


//		GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "SNake Hit ROT " + FString::SanitizeFloat(OutputTransform.Rotator().Pitch) );


		//		if (all_feets_hitting)
		{
			//			if (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit && spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.ContainsNaN() == false && spine_hit_pairs.Num() > origin_point_index - 1)
			//				result.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z+Total_spine_heights[origin_point_index] ));


			if (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit && atleast_one_hit && Enable_Solver)
			{
				//OutputTransform.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]));


				if ((  (FVector(lerp_data.X, lerp_data.Y, Z_Offset + Total_spine_heights[origin_point_index])) - lerp_data).Size() > 10000*component_scale)
					OutputTransform.SetLocation(FVector(lerp_data.X, lerp_data.Y, Z_Offset + Total_spine_heights[origin_point_index]));
				else
					OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), FVector(lerp_data.X, lerp_data.Y, Z_Offset + Total_spine_heights[origin_point_index]), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));





				//					GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Impact Hit " + OutputTransform.GetLocation().ToString());

			}
			else
			{

				OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), lerp_data, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));
			}


		}

	}
	else
	{

		OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), lerp_data, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));
	}




//	OutputTransform.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]));




	//OutputTransform.SetRotation(new_rotator.Quaternion());

	//	OutputTransform = result;
}


/*
void FAnimNode_DragonSpineSolver::Snake_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases)
{

	FTransform result = FTransform::Identity;

	FVector new_location = (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint + FVector(0, 0, Total_spine_heights[origin_point_index]));

	FTransform bonetraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[origin_point_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));

	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());



    
	FRotator bone_rotation = FRotator(owning_skel->GetComponentRotation());

	if (owning_skel->GetOwner() != nullptr)
	{
		bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
	}


	FRotator new_rotator = FRotator(0, bone_rotation.Yaw,0);



//	result.SetLocation(lerp_data);



	if (spine_hit_pairs.Num() > origin_point_index - 1)
	{

//		if (all_feets_hitting)
		{
//			if (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit && spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.ContainsNaN() == false && spine_hit_pairs.Num() > origin_point_index - 1)
//				result.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z+Total_spine_heights[origin_point_index] ));


			if (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit && atleast_one_hit && Enable_Solver)
			{
				OutputTransform.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]));

		//		GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Impact Hit "+ );

//				if ((OutputTransform.GetLocation() - lerp_data).Size() > 10000)
//					OutputTransform.SetLocation(FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]));
//				else
//					OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));

//				result.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), FVector(lerp_data.X, lerp_data.Y, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z + Total_spine_heights[origin_point_index]), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));


//					GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Impact Hit " + OutputTransform.GetLocation().ToString());

			}
			else
			{

				OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), lerp_data, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));
			}


		}
		
	}
	else
	{

		OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(lerp_data.X, lerp_data.Y, OutputTransform.GetLocation().Z), lerp_data, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));
	}







//	OutputTransform.SetLocation(lerp_data);
//	result.SetLocation(new_location);

//	OutputTransform.SetRotation(spine_RotDifference[origin_point_index].Quaternion());


	OutputTransform.SetRotation(new_rotator.Quaternion());

//	OutputTransform = result;
}

*/

void FAnimNode_DragonSpineSolver::Dragon_ImpactRotation(int origin_point_index, FTransform &OutputTransform, FCSPose<FCompactPose>& MeshBases, bool is_reverse)
{
	TArray<FBoneTransform> TempBoneTransforms;


	FTransform bonetraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[origin_point_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));

	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());



	FBoneReference root_bone_ref = FBoneReference(owning_skel->GetBoneName(0));
	root_bone_ref.Initialize(*SavedBoneContainer);

	FTransform RoottraceTransform = MeshBases.GetComponentSpaceTransform(root_bone_ref.GetCompactPoseIndex(*SavedBoneContainer));
	RoottraceTransform.SetLocation(FVector(0, 0, 0));

	FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBases, RoottraceTransform, FCompactPoseBoneIndex(root_bone_ref.BoneIndex), EBoneControlSpace::BCS_WorldSpace);


	FTransform world_bone_transform = FTransform::Identity;





	float chest_distance = Total_spine_heights[origin_point_index];



	FRotator final_rot = FRotator();

	bool all_feets_hitting = true;

	for (int i = 0; i<spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num(); i++)
	{
		if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].bBlockingHit == false)
		{
			all_feets_hitting = false;
		}
	}


	if (spine_hit_pairs[origin_point_index].Parent_Spine_Hit.bBlockingHit == false)
	{
		all_feets_hitting = false;
	}

	if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 0)
		all_feets_hitting = false;




	if (spine_hit_pairs.Num() > origin_point_index - 1)
	{

		//if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].bBlockingHit && spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].bBlockingHit)

		//		if (all_feets_hitting)
		if (all_feets_hitting)
		{




			FRotator input_rotation_axis;

			FRotator XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetForwardVector(), (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactNormal + spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactNormal) / 2);
			FRotator YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetRightVector(), (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactNormal + spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactNormal) / 2);
			FRotator bone_rotation = FRotator(owning_skel->GetComponentRotation());

			if (owning_skel->GetOwner() != nullptr)
			{
				XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetOwner()->GetActorForwardVector(), (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactNormal + spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactNormal) / 2);
				YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetOwner()->GetActorRightVector(), (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactNormal + spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactNormal) / 2);
				bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
			}


			input_rotation_axis = FRotator((YZ_rot.Pitch), bone_rotation.Yaw, XZ_rot.Roll);



			FVector impact_difference = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint - spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint).GetSafeNormal();



			int multiplier = 1;

			if (FVector::DotProduct(impact_difference, owning_skel->GetForwardVector()) < 0)
				multiplier = -1;


			impact_difference = FVector(impact_difference.X * multiplier, impact_difference.Y * multiplier, impact_difference.Z * multiplier);


			FVector cross_x = FVector::CrossProduct(impact_difference, owning_skel->GetForwardVector().GetSafeNormal());

			FRotator Feet_XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetForwardVector(), cross_x);
			FRotator Feet_YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetRightVector(), cross_x);
			FRotator final_bone_rotation = FRotator(owning_skel->GetComponentRotation());

			if (owning_skel->GetOwner() != nullptr)
			{

				final_bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());

				cross_x = FVector::CrossProduct(impact_difference, UKismetMathLibrary::RotateAngleAxis(owning_skel->GetOwner()->GetActorRightVector(), owning_skel->GetRelativeTransform().Rotator().Yaw, owning_skel->GetOwner()->GetActorUpVector()));

				Feet_XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetOwner()->GetActorForwardVector(), cross_x);
				Feet_YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetOwner()->GetActorRightVector(), cross_x);

			}

			final_rot = FRotator((YZ_rot.Pitch), final_bone_rotation.Yaw, XZ_rot.Roll);

			FTransform input_bone_transform_cs = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[origin_point_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));
			input_bone_transform_cs.SetRotation((owning_skel->GetRelativeTransform().Rotator() + FRotator(0, 180, 0)).Quaternion());
			FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBases, input_bone_transform_cs, FCompactPoseBoneIndex(spine_Feet_pair[origin_point_index].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);



			FTransform final_transformation = FTransform::Identity;


			FVector Feet_Mid_Point = FVector(0, 0, 0);


			FVector Feet_Opposite_Mid_Point = FVector(0, 0, 0);





			float feet_difference_offset = spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z;

			//if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 2)
			{

				/*
				if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z > spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)
				{
				feet_difference_offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z;
				}
				else
				{
				feet_difference_offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z;
				}
				*/









				float Z_Offset = 10000000000;


				if (Use_FeetTips == false)
				{

					for (int32 i = 0; i < spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num(); i++)
					{

						if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint.Z < Z_Offset)
						{
							Z_Offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint.Z;

							//	lowest_trace_hit = spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint;
						}

					}

				}
				else
				{
					for (int32 i = 0; i < spine_hit_pairs[origin_point_index].RV_FeetBalls_Hits.Num(); i++)
					{

						if (spine_hit_pairs[origin_point_index].RV_FeetBalls_Hits[i].ImpactPoint.Z < Z_Offset)
						{
							Z_Offset = spine_hit_pairs[origin_point_index].RV_FeetBalls_Hits[i].ImpactPoint.Z;

							//	lowest_trace_hit = spine_hit_pairs[origin_point_index].RV_Feet_Hits[i].ImpactPoint;
						}

					}
				}

				Z_Offset = FMath::Clamp(Z_Offset, owning_skel->GetComponentLocation().Z - Maximum_Dip_Height * component_scale, owning_skel->GetComponentLocation().Z);

				/*
				if (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z > spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)
				Z_Offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z;
				else
				Z_Offset = spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z;
				*/


				feet_difference_offset = Z_Offset;

			}



			Feet_Mid_Point = spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint;


			//			accurate_feet_placement = true;

			if (accurate_feet_placement)
				Feet_Mid_Point.Z = FMath::Lerp<float>(feet_difference_offset, Feet_Mid_Point.Z, FMath::Clamp<float>(FMath::Abs(spine_RotDifference[origin_point_index].Pitch*0.05f), 0, 1));


			int32 opposite_index = 0;

			if (origin_point_index == 0)
				opposite_index = spine_hit_pairs.Num() - 1;
			else
				opposite_index = 0;



			float feet_difference_offset_opp = spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint.Z;

			if (spine_hit_pairs[opposite_index].RV_Feet_Hits.Num() == 2)
			{

				if (spine_hit_pairs[opposite_index].RV_Feet_Hits[0].ImpactPoint.Z > spine_hit_pairs[opposite_index].RV_Feet_Hits[1].ImpactPoint.Z)
					feet_difference_offset_opp = spine_hit_pairs[opposite_index].RV_Feet_Hits[1].ImpactPoint.Z;
				else
					feet_difference_offset_opp = spine_hit_pairs[opposite_index].RV_Feet_Hits[0].ImpactPoint.Z;


			}


			Feet_Opposite_Mid_Point = spine_hit_pairs[opposite_index].Parent_Spine_Hit.ImpactPoint;

			if (accurate_feet_placement)
				Feet_Opposite_Mid_Point.Z = FMath::Lerp<float>(feet_difference_offset_opp, Feet_Opposite_Mid_Point.Z, FMath::Clamp<float>(FMath::Abs(spine_RotDifference[origin_point_index].Pitch*0.05f), 0, 1));




			FVector new_location = FVector();


			FVector location_output = FVector();



			bool extreme_difference = false;

			float diff_sum = 0;





			float Slanted_Height_Offset = 0;


			//			if (Root_Pitch_Value > 0)




			if ((Feet_Mid_Point.Z - Feet_Opposite_Mid_Point.Z)>0)
			{
				if (origin_point_index == 0)
				{
					Slanted_Height_Offset = Slanted_Height_Down_Offset;

					//					GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Mid Point Difference for " + FString::SanitizeFloat(origin_point_index) + " = " + FString::SanitizeFloat((Feet_Mid_Point.Z - Feet_Opposite_Mid_Point.Z)));

				}
				else
				{
					Slanted_Height_Offset = Chest_Slanted_Height_Up_Offset;
				}

			}
			else
			{
				if (origin_point_index == 0)
				{
					Slanted_Height_Offset = Slanted_Height_Up_Offset * -1;
				}
				else
				{
					Slanted_Height_Offset = Chest_Slanted_Height_Down_Offset * -1;
				}



			}



			//			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Mid Point Difference for " + FString::SanitizeFloat(origin_point_index)+" = "+ FString::SanitizeFloat((Feet_Mid_Point.Z - Feet_Opposite_Mid_Point.Z)));



			float slop_tolerance = 0;

			if (origin_point_index == 0)
				slop_tolerance = Slope_Detection_Strength;
			else
				slop_tolerance = Chest_Slope_Detection_Strength;



			if (atleast_one_hit)
			{

				//				GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Executing ImpactRotation..");


				if (origin_point_index == 0)
				{
					//					new_location = (Feet_Mid_Point + FVector(0, 0, chest_distance) + FVector(0, 0, FMath::Abs(lerp_data.Z - (Feet_Mid_Point.Z + chest_distance)))*0.25F * 0);


					//new_location = (Feet_Mid_Point + FVector(0, 0, chest_distance - FMath::Abs(down_offset)));
					new_location = (Feet_Mid_Point + FVector(0, 0, Total_spine_heights[origin_point_index]));



					//					new_location += FVector(0, 0, FMath::Abs(Root_Pitch_Value)*Slanted_Height_Offset*component_scale*FMath::Clamp<float>((spine_median_result / 10)*slop_tolerance, 0, 1));


					new_location += FVector(0, 0, (Feet_Mid_Point.Z - Feet_Opposite_Mid_Point.Z)*Slanted_Height_Offset*FMath::Clamp<float>((spine_median_result / 10)*slop_tolerance, 0, 1));



				}
				else
				{
					new_location = (Feet_Mid_Point + FVector(0, 0, chest_distance));
					//						new_location += FVector(0, 0, FMath::Abs(Root_Pitch_Value)*Slanted_Height_Offset*component_scale*FMath::Clamp<float>((spine_median_result / 10)*slop_tolerance, 0, 1));

					new_location += FVector(0, 0, (Feet_Mid_Point.Z - Feet_Opposite_Mid_Point.Z)*Slanted_Height_Offset*FMath::Clamp<float>((spine_median_result / 10)*slop_tolerance, 0, 1));



					FVector opposite_root_transform = owning_skel->GetComponentTransform().TransformPosition(MeshBases.GetComponentSpaceTransform(spine_Feet_pair[opposite_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer)).GetLocation());
					FVector relative_root_offset = opposite_root_transform - RootEffectorTransform.GetLocation();


					new_location = FVector(new_location.X, new_location.Y, FMath::Lerp<float>((lerp_data - relative_root_offset).Z, new_location.Z, Chest_Influence_Alpha));

				}




				new_location.Z = FMath::Lerp<float>(lerp_data.Z, new_location.Z, FMath::Clamp<float>(Adaptive_Alpha, 0, 1));

				if (Enable_Solver == false)
					new_location.Z = lerp_data.Z;




				//new_location.Z = lerp_data.Z;


				FVector saved_pos = new_location;




				FVector virtual_pelvic_position = (Feet_Mid_Point + FVector(0, 0, chest_distance) + FVector(0, 0, FMath::Abs(lerp_data.Z - (Feet_Mid_Point.Z + chest_distance)))*0.25F * 1);



				FTransform mesh_transformy = FTransform::Identity;
				mesh_transformy.SetLocation(saved_pos);




				spine_LocDifference[origin_point_index] = FVector(0, 0, owning_skel->GetComponentTransform().GetLocation().Z - mesh_transformy.GetLocation().Z);


				FTransform mesh_transform = FTransform::Identity;

				mesh_transform.SetLocation(spine_LocDifference[origin_point_index]);

				location_output = mesh_transformy.GetLocation();


			}
			else
			{


				new_location = (Feet_Mid_Point + FVector(0, 0, Total_spine_heights[origin_point_index]));
				new_location.Z = lerp_data.Z;



				//				FTransform mesh_transform = FTransform::Identity;
				//				mesh_transform.SetLocation(FVector(lerp_data.X, lerp_data.Y, owning_skel->GetComponentLocation().Z - spine_LocDifference[origin_point_index].Z));


				location_output = lerp_data;

				/*
				FTransform mesh_transform = FTransform::Identity;
				mesh_transform.SetLocation(FVector(lerp_data.X, lerp_data.Y, owning_skel->GetComponentLocation().Z - spine_LocDifference[origin_point_index].Z));

				bool should_solving_persist = true;

				if (should_solving_persist)
				location_output = mesh_transform.GetLocation();
				else
				location_output = lerp_data;
				*/
			}



			final_transformation.SetLocation(location_output);


			const FVector pointing_to_transformpoint = FVector(new_location.X, new_location.Y, final_transformation.GetLocation().Z);

			new_location.X = lerp_data.X;
			new_location.Y = lerp_data.Y;

			final_transformation.SetLocation(FVector(new_location.X, new_location.Y, final_transformation.GetLocation().Z));


			FRotator position_Based_rot = FRotator();
			position_Based_rot = UKismetMathLibrary::FindLookAtRotation(lerp_data, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint + (spine_hit_pairs[0].Parent_Spine_Hit.ImpactPoint - spine_hit_pairs[spine_hit_pairs.Num() - 1].Parent_Spine_Hit.ImpactPoint).GetSafeNormal() * 50);

			position_Based_rot = FRotator(position_Based_rot.Quaternion() * FRotator(90, 0, 0).Quaternion());
			position_Based_rot.Yaw = final_rot.Yaw;

			FTransform rot_transform = FTransform(position_Based_rot);






			if (origin_point_index == 0)
			{




				FVector forward_direction = FVector();
				FVector right_direction = FVector();
				float Intensifier_forward = 0;

				float Intensifier_side = 0;


				if (owning_skel->GetOwner())
				{


//					Intensifier_forward = FMath::Clamp<float>((Feet_Mid_Point.Z - (Feet_Opposite_Mid_Point.Z)) * Pelvis_ForwardRotation_Intensity, -10000, 10000)  *-1;

//					Intensifier_forward = FMath::Lerp<float>(Intensifier_forward, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));



					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num()>2 || Use_Fake_Pelvis_Rotations || spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 0)
					{

						if (spine_hit_pairs[origin_point_index].Parent_Front_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Back_Hit.bBlockingHit)
							Intensifier_forward = (spine_hit_pairs[origin_point_index].Parent_Front_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Back_Hit.ImpactPoint.Z)* Pelvis_ForwardRotation_Intensity;
						else
							Intensifier_forward = 0;

					}
					else
					{
						Intensifier_forward = FMath::Clamp<float>((Feet_Mid_Point.Z - (Feet_Opposite_Mid_Point.Z)) * Pelvis_ForwardRotation_Intensity, -10000, 10000)  *-1;

						Intensifier_forward = FMath::Lerp<float>(Intensifier_forward, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));


					}


					forward_direction = owning_skel->GetOwner()->GetActorForwardVector() * Intensifier_forward;


					forward_direction.Z = 0;



					int direction = 1;


					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() > 0)
					{

						if (owning_skel->GetComponentToWorld().InverseTransformPosition(spine_Transform_pairs[origin_point_index].Associated_Feet[0].GetLocation()).X > 0)
							direction = 1;
						else
							direction = -1;
					}



//					Intensifier_side = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z - spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)*direction* Body_Rotation_Intensity *-1;


					
					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num()>2 || Use_Fake_Pelvis_Rotations || spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 0)
					{

						if (spine_hit_pairs[origin_point_index].Parent_Left_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Right_Hit.bBlockingHit)
							Intensifier_side = (spine_hit_pairs[origin_point_index].Parent_Left_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Right_Hit.ImpactPoint.Z)* Body_Rotation_Intensity;
						else
							Intensifier_side = 0;

					}
					else
					{
//						Intensifier_side = FMath::Clamp<float>((Feet_Mid_Point.Z - (Feet_Opposite_Mid_Point.Z)) * Pelvis_ForwardRotation_Intensity, -10000, 10000)  *-1;

						Intensifier_side = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z - spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)*direction* Body_Rotation_Intensity *-1;


						Intensifier_side = FMath::Lerp<float>(Intensifier_side, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));


					}


					right_direction = owning_skel->GetOwner()->GetActorRightVector()*Intensifier_side;



				}



				//final_transformation.TransformVector(owning_skel->GetForwardVector() )
				FVector relativeUp = (owning_skel->GetForwardVector()*-1);

				FVector relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + forward_direction)).GetSafeNormal();

				//				FVector relativePos = (lerp_data - DebugEffectorTransform.GetLocation()).GetSafeNormal();



				FRotator look_rot = FRotator(UDragonIK_Library::CustomLookRotation(relativePos, relativeUp));

				position_Based_rot = look_rot;


				position_Based_rot.Yaw = final_rot.Yaw;


				Root_Roll_Value = position_Based_rot.Roll;

				Root_Pitch_Value = position_Based_rot.Pitch;


				FVector dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);


				FVector Roll_relativeUp = (owning_skel->GetForwardVector()*-1);


				FVector Roll_relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + right_direction)).GetSafeNormal();


				//FVector Roll_relativePos = (pointing_to_transformpoint - DebugEffectorTransform.GetLocation()).GetSafeNormal();



				FRotator Roll_look_rot = FRotator(UDragonIK_Library::CustomLookRotation(Roll_relativePos, Roll_relativeUp));


				position_Based_rot.Roll = Roll_look_rot.Roll;


				spine_RotDifference[origin_point_index].Yaw = position_Based_rot.Yaw;




				if (atleast_one_hit)
				{

					if (atleast_one_hit && position_Based_rot.Pitch < MaximumPitch && position_Based_rot.Pitch > MinimumPitch)
						spine_RotDifference[origin_point_index].Pitch = position_Based_rot.Pitch;
					else if (position_Based_rot.Pitch > MaximumPitch)
					{
						spine_RotDifference[origin_point_index].Pitch = MaximumPitch;
					}
					else if (position_Based_rot.Pitch < MinimumPitch)
					{
						spine_RotDifference[origin_point_index].Pitch = MinimumPitch;
					}




					if (atleast_one_hit && position_Based_rot.Roll < MaximumRoll && position_Based_rot.Roll > MinimumRoll)
						spine_RotDifference[origin_point_index].Roll = position_Based_rot.Roll;
					else if (position_Based_rot.Roll > MaximumRoll)
					{

						/*
						if(position_Based_rot.Roll>0)
						spine_RotDifference[origin_point_index].Roll = MaximumRoll;
						else
						spine_RotDifference[origin_point_index].Roll = MaximumRoll*-1;
						*/

						spine_RotDifference[origin_point_index].Roll = MaximumRoll;
					}
					else if (position_Based_rot.Roll < MinimumRoll)
					{
						spine_RotDifference[origin_point_index].Roll = MinimumRoll;
					}



				}



				//				OutputTransform.SetRotation(UKismetMathLibrary::RLerp(FRotator(OutputTransform.Rotator().Pitch, position_Based_rot.Yaw, OutputTransform.Rotator().Roll), spine_RotDifference[origin_point_index], owning_skel->GetWorld()->DeltaTimeSeconds * Body_Rotation_Interpolation_Speed, true).Quaternion());


			}
			else if (origin_point_index == spine_Transform_pairs.Num() - 1)
			{
				FVector forward_direction = FVector();

				FVector right_direction = FVector();


				float forward_intensity = 0;

				float right_intensity = 0;



				if (owning_skel->GetOwner())
				{














//					forward_intensity = FMath::Clamp<float>(((Feet_Opposite_Mid_Point.Z) - Feet_Mid_Point.Z) * Chest_ForwardRotation_Intensity, -10000, 10000)  *-1;

//					forward_intensity = FMath::Lerp<float>(forward_intensity, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));




					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num()>2 || Use_Fake_Chest_Rotations || spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 0)
					{

						if (spine_hit_pairs[origin_point_index].Parent_Front_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Back_Hit.bBlockingHit)
							forward_intensity = (spine_hit_pairs[origin_point_index].Parent_Front_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Back_Hit.ImpactPoint.Z)* Chest_ForwardRotation_Intensity;
						else
							forward_intensity = 0;

					}
					else
					{
						forward_intensity = FMath::Clamp<float>(((Feet_Opposite_Mid_Point.Z) - Feet_Mid_Point.Z) * Chest_ForwardRotation_Intensity, -10000, 10000)  *-1;

						forward_intensity = FMath::Lerp<float>(forward_intensity, 0, FMath::Clamp<float>((spine_median_result / (10 * Slope_Detection_Strength)), 0, 1));


					}




					forward_direction = owning_skel->GetOwner()->GetActorForwardVector() * forward_intensity;



					int direction = 1;

					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() > 0)
					{
						if (owning_skel->GetComponentToWorld().InverseTransformPosition(spine_Transform_pairs[origin_point_index].Associated_Feet[0].GetLocation()).X > 0)
							direction = 1;
						else
							direction = -1;
					}




					//						right_intensity = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z - spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)*direction* Body_Rotation_Intensity *-1;



					if (spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num()>2 || Use_Fake_Chest_Rotations || spine_hit_pairs[origin_point_index].RV_Feet_Hits.Num() == 0)
					{

						if (spine_hit_pairs[origin_point_index].Parent_Left_Hit.bBlockingHit&&spine_hit_pairs[origin_point_index].Parent_Right_Hit.bBlockingHit)
							right_intensity = (spine_hit_pairs[origin_point_index].Parent_Left_Hit.ImpactPoint.Z - spine_hit_pairs[origin_point_index].Parent_Right_Hit.ImpactPoint.Z)* Body_Rotation_Intensity;
						else
							right_intensity = 0;

					}
					else
					{

						right_intensity = (spine_hit_pairs[origin_point_index].RV_Feet_Hits[0].ImpactPoint.Z - spine_hit_pairs[origin_point_index].RV_Feet_Hits[1].ImpactPoint.Z)*direction* Body_Rotation_Intensity *-1;


					}

					//					right_direction = owning_skel->GetComponentToWorld().InverseTransformVector(owning_skel->GetRightVector()) ;

					right_direction = owning_skel->GetOwner()->GetActorRightVector()*right_intensity;



					forward_direction.Z = 0;
				}


				FVector relativeUp = (owning_skel->GetForwardVector()*-1);
				FVector relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + forward_direction)).GetSafeNormal();


				//FVector relativePos = (pointing_to_transformpoint - DebugEffectorTransform.GetLocation()).GetSafeNormal();


				FRotator look_rot = FRotator(UDragonIK_Library::CustomLookRotation(relativePos, relativeUp));

				position_Based_rot = look_rot;

				position_Based_rot.Yaw = final_rot.Yaw;
				//				position_Based_rot.Roll = final_rot.Roll;



				FVector Roll_relativeUp = (owning_skel->GetForwardVector()*-1);

				FVector Roll_relativePos = (pointing_to_transformpoint - (Feet_Mid_Point + right_direction)).GetSafeNormal();



				FRotator Roll_look_rot = FRotator(UDragonIK_Library::CustomLookRotation(Roll_relativePos, Roll_relativeUp));


				position_Based_rot.Roll = Roll_look_rot.Roll;


				spine_RotDifference[origin_point_index].Yaw = position_Based_rot.Yaw;
				//			position_Based_rot = UKismetMathLibrary::RLerp(FRotator(0, final_rot.Yaw, 0), position_Based_rot, atleast_one_hit, true);





				if (atleast_one_hit)
				{

					if (atleast_one_hit && position_Based_rot.Pitch < MaximumPitch && position_Based_rot.Pitch > MinimumPitch)
						spine_RotDifference[origin_point_index].Pitch = position_Based_rot.Pitch;
					else if (position_Based_rot.Pitch > MaximumPitch)
					{
						spine_RotDifference[origin_point_index].Pitch = MaximumPitch;
					}
					else if (position_Based_rot.Pitch < MinimumPitch)
					{
						spine_RotDifference[origin_point_index].Pitch = MinimumPitch;
					}




					if (atleast_one_hit && position_Based_rot.Roll < MaximumRoll && position_Based_rot.Roll > MinimumRoll)
						spine_RotDifference[origin_point_index].Roll = position_Based_rot.Roll;
					else if (position_Based_rot.Roll > MaximumRoll)
					{

						/*
						if(position_Based_rot.Roll>0)
						spine_RotDifference[origin_point_index].Roll = MaximumRoll;
						else
						spine_RotDifference[origin_point_index].Roll = MaximumRoll*-1;
						*/

						spine_RotDifference[origin_point_index].Roll = MaximumRoll;
					}
					else if (position_Based_rot.Roll < MinimumRoll)
					{
						spine_RotDifference[origin_point_index].Roll = MinimumRoll;
					}


				}


			}




			FRotator current_rot = FRotator(OutputTransform.Rotator().Pitch, final_rot.Yaw, OutputTransform.Rotator().Roll).GetNormalized();
			//FRotator target_rot = FRotator(spine_RotDifference[origin_point_index].Pitch, final_rot.Yaw, spine_RotDifference[origin_point_index].Roll).GetNormalized();



//			FRotator target_rot = FRotator(UKismetMathLibrary::Lerp<float>( spine_RotDifference[origin_point_index].Pitch, spine_RotDifference[origin_point_index].Pitch,0,adapt), final_rot.Yaw, spine_RotDifference[origin_point_index].Roll).GetNormalized();

			FRotator target_rot = FRotator(UKismetMathLibrary::Lerp(0, spine_RotDifference[origin_point_index].Pitch, Adaptive_Alpha), final_rot.Yaw, UKismetMathLibrary::Lerp(0, spine_RotDifference[origin_point_index].Roll,Adaptive_Alpha) ).GetNormalized();

		//	target_rot = UKismetMathLibrary::RLerp(target_rot, FRotator(0, target_rot.Yaw, 0).GetNormalized(),Adaptive_Alpha,true);


		
			//			if (spine_RotDifference[origin_point_index].ContainsNaN() == false && current_rot.ContainsNaN() == false && target_rot.ContainsNaN() == false)
			{
				if (atleast_one_hit && Enable_Solver == true)
				{

					OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(current_rot, target_rot, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Rotation_Lerp_Speed* 0.05f).Quaternion().GetNormalized());
				}
				else
				{
					target_rot = FRotator(0, target_rot.Yaw, 0);
					//					OutputTransform.SetRotation(target_rot.Quaternion().GetNormalized());

					OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(current_rot, target_rot, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Rotation_Lerp_Speed* 0.05f).Quaternion().GetNormalized());
				}
			}


		//	OutputTransform.SetRotation(FRotator(0, target_rot.Yaw, 0).Quaternion());

			/*else
			{
			OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(current_rot, target_rot, owning_skel->GetWorld()->GetDeltaSeconds(), Shift_Speed).Quaternion().GetNormalized() );
			}*/

			//			OutputTransform.NormalizeRotation();


			//if (LineTraceInitialized == true)
			{

				//				FVector right_dir = UKismetMathLibrary::GetRightVector((spine_Transform_pairs[0].Spine_Involved.GetLocation() - spine_Transform_pairs[spine_Transform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal().Rotation()) * 1;


				//				FVector right_dir = UKismetMathLibrary::GetRightVector((spine_Transform_pairs[0].Spine_Involved.GetLocation() - spine_Transform_pairs[spine_Transform_pairs.Num() - 1].Spine_Involved.GetLocation()).GetSafeNormal().Rotation()) * 1;


				FVector forward_dir_forward = owning_skel->GetComponentToWorld().TransformVector(Forward_Direction_Vector);

				FVector right_dir_vector = FVector::CrossProduct(forward_dir_forward, FVector(0, 0, 1));


				final_transformation.AddToTranslation(FVector(0, 0, FMath::Abs(spine_RotDifference[origin_point_index].Roll*upward_push_side_rotation)*component_scale));


				FVector spine_position = final_transformation.GetLocation();


				spine_position.Y = lerp_data.Y;
				spine_position.X = lerp_data.X;


				FVector pitch_based_location = RotateAroundPoint(spine_position, right_dir_vector, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Pitch);


//				if(is_snake==false)
				final_transformation.SetLocation(pitch_based_location);
//				else
//				final_transformation.SetLocation(spine_position);


				//				else
				//					final_transformation.SetLocation(roll_based_location);


				//				if(FMath::Abs<float>(OutputTransform.Rotator().Pitch) > FMath::Abs<float>(OutputTransform.Rotator().Roll))
				//				final_transformation.SetLocation(RotateAroundPoint(spine_position, forward_dir_forward, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Pitch));
				//				else
				//				final_transformation.SetLocation(RotateAroundPoint(spine_position, right_dir_vector, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Roll));




				//final_transformation.SetLocation(    ( RotateAroundPoint(spine_position, forward_dir_forward, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Pitch) + RotateAroundPoint(spine_position, right_dir_vector, spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint, OutputTransform.Rotator().Roll))/2      );



				float base_offset = 0;

				if (origin_point_index == 0)
					base_offset = Pelvis_Base_Offset * owning_skel->GetComponentTransform().GetScale3D().Z;
				else if (origin_point_index == spine_Feet_pair.Num() - 1)
					base_offset = Chest_Base_Offset * owning_skel->GetComponentTransform().GetScale3D().Z;



				FVector location_reset = lerp_data + FVector(0, 0, base_offset);



				if (atleast_one_hit)
				{

					if ((OutputTransform.GetLocation() - final_transformation.GetLocation()).Size() > 10000*component_scale)
						OutputTransform.SetLocation(final_transformation.GetLocation() + FVector(0, 0, base_offset));
					else
						//						OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(final_transformation.GetLocation().X, final_transformation.GetLocation().Y, OutputTransform.GetLocation().Z), final_transformation.GetLocation() + FVector(0, 0, base_offset), 1 - FMath::Exp(-2 * owning_skel->GetWorld()->GetDeltaSeconds()), FMath::Clamp<float>(1/FMath::Abs(OutputTransform.GetLocation().Z - final_transformation.GetLocation().Z) , 0.1,1)* Location_Lerp_Speed * 20));
					{
						//							OutputTransform.SetLocation(SmoothApproach(spine_AnimatedTransform_pairs[origin_point_index].last_spine_location, spine_AnimatedTransform_pairs[origin_point_index].last_target_location, final_transformation.GetLocation(), Location_Lerp_Speed));

						//							spine_AnimatedTransform_pairs[origin_point_index].last_spine_location = OutputTransform.GetLocation();

						//							spine_AnimatedTransform_pairs[origin_point_index].last_spine_location = final_transformation.GetLocation();


					//	final_transformation.SetLocation(spine_hit_pairs[origin_point_index].Parent_Spine_Hit.ImpactPoint+Total_spine_heights[origin_point_index] );

						OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(final_transformation.GetLocation().X, final_transformation.GetLocation().Y, OutputTransform.GetLocation().Z), final_transformation.GetLocation() + FVector(0, 0, base_offset), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));

					}


					//	`transform.position = Vector3.Lerp(transform.position, point, 1 - Mathf.Exp(-2 * Time.deltaTime)); `

					//						if (origin_point_index == 0)
					//							GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "Executing ImpactRotation...... "+FString::SanitizeFloat(FMath::Clamp<float>(1 / FMath::Abs(OutputTransform.GetLocation().Z - final_transformation.GetLocation().Z), 0.001, 1)));
				}
				else
				{


					if ((OutputTransform.GetLocation() - location_reset).Size() > 10000*component_scale)
						OutputTransform.SetLocation(location_reset);
					else
						OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(location_reset.X, location_reset.Y, OutputTransform.GetLocation().Z), location_reset, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()),Location_Lerp_Speed* 0.05f));




					//OutputTransform.SetLocation(lerp_data + FVector(0, 0, base_offset));
					//						OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo_Constant(FVector(final_transformation.GetLocation().X, final_transformation.GetLocation().Y, OutputTransform.GetLocation().Z), lerp_data + FVector(0, 0, base_offset), owning_skel->GetWorld()->GetDeltaSeconds(), Location_Lerp_Speed *10 ));
				}
			}

			

			/*Debug

			OutputTransform.SetRotation(FRotator(0, final_rot.Yaw, 0).Quaternion());
			if (origin_point_index == 0)
			OutputTransform.SetLocation(DebugEffectorTransform.GetLocation());
			else
			OutputTransform.SetLocation(DebugEffectorTransformTwo.GetLocation());
			*/
		}
		else
		{


			FTransform mesh_transform = owning_skel->GetComponentTransform();

			mesh_transform.SetLocation(MeshBases.GetComponentSpaceTransform(spine_Feet_pair[origin_point_index].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer)).GetLocation());

			FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBases, mesh_transform, FCompactPoseBoneIndex(spine_Feet_pair[origin_point_index].Spine_Involved.BoneIndex), EBoneControlSpace::BCS_WorldSpace);


			float owner_bone_yaw = 0;

			if (owning_skel->GetOwner() != nullptr)
			{
				owner_bone_yaw = FRotator(owning_skel->GetOwner()->GetActorRotation()).Yaw;
			}

			float base_offset = 0;

			if (origin_point_index == 0)
				base_offset = Pelvis_Base_Offset * owning_skel->GetComponentTransform().GetScale3D().Z;
			else if (origin_point_index == spine_Feet_pair.Num() - 1)
				base_offset = Chest_Base_Offset * owning_skel->GetComponentTransform().GetScale3D().Z;

			FVector location_reset = lerp_data + FVector(0, 0, base_offset);


			if ((OutputTransform.GetLocation() - location_reset).Size() > 10000)
				OutputTransform.SetLocation(location_reset);
			else
				OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(location_reset.X, location_reset.Y, OutputTransform.GetLocation().Z), location_reset, 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));



			OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(OutputTransform.Rotator(), FRotator(0, owner_bone_yaw, 0), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Rotation_Lerp_Speed* 0.05f).Quaternion().GetNormalized());


			//			OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(OutputTransform.GetLocation(), mesh_transform.GetLocation(), owning_skel->GetWorld()->DeltaTimeSeconds, Shift_Speed));
			//			OutputTransform.SetRotation(UKismetMathLibrary::RInterpTo(OutputTransform.Rotator(), FRotator(0, owner_bone_yaw, 0), owning_skel->GetWorld()->DeltaTimeSeconds, Shift_Speed).Quaternion());

		}
	}
}


FVector FAnimNode_DragonSpineSolver::SmoothApproach(FVector pastPosition, FVector pastTargetPosition, FVector targetPosition, float speed)
{
	float t = owning_skel->GetWorld()->DeltaTimeSeconds * speed;
	FVector v = (targetPosition - pastTargetPosition) / t;
	FVector f = pastPosition - pastTargetPosition + v;
	return targetPosition - v + f * FMath::Exp(-t);
}

FVector FAnimNode_DragonSpineSolver::RotateAroundPoint(FVector input_point, FVector forward_vector, FVector origin_point, float angle)
{
	FVector orbit_direction;

	orbit_direction = input_point - origin_point;

	FVector axis_dir = UKismetMathLibrary::RotateAngleAxis(orbit_direction, angle, forward_vector);

	FVector result_vector = input_point + (axis_dir - orbit_direction);

	return result_vector;

}


DragonSpineOutput FAnimNode_DragonSpineSolver::DragonSpineProcessor(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{

	//Declare output
	DragonSpineOutput output = DragonSpineOutput();

	FTransform CSEffectorTransform = EffectorTransform;
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentToWorld(), MeshBases, CSEffectorTransform, spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer), EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();


	// Gather all bone indices between root and tip.
	const TArray<FCompactPoseBoneIndex> BoneIndices = Spine_Indices;


	//Supply bone indices array to output struct
	output.BoneIndices = BoneIndices;

	// Maximum length of skeleton segment at full extension . Default set to 0
	float MaximumReach = 0;
	//	float MaximumSecReach = 0;

	// Gather transforms and define total num of bones present
	int32 const NumTransforms = BoneIndices.Num();
	//	int32 const Num2Transforms = BoneSecIndices.Num();


	//Temp transform is the actual bone data used
	//Initialize numTransform amount of dummy data to temp_transforms
	output.temp_transforms.AddUninitialized(NumTransforms);


	//Declare are initialize dummy data for the chain struct array
	TArray<DragonChainLink> Chain;
	Chain.Reserve(NumTransforms);


	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));



	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());




	float scale_mod = 1;

	if (owning_skel->GetOwner() != nullptr)
	{
		scale_mod = owning_skel->GetOwner()->GetActorScale().Z;
	}

	float pelvis_distance = FMath::Abs(lerp_data.Z - owning_skel->GetBoneLocation(owning_skel->GetBoneName(0)).Z);



	const FCompactPoseBoneIndex& TipBoneIndex = BoneIndices[BoneIndices.Num() - 1];
	const FTransform& BoneCSTransform_Local = MeshBases.GetComponentSpaceTransform(TipBoneIndex);

	FTransform offseted_Transform_Local = BoneCSTransform_Local;
	offseted_Transform_Local.SetLocation(owning_skel->GetComponentTransform().InverseTransformPosition(ChestEffectorTransform.GetLocation()));


	//Set root transform to 0th temp transform
	output.temp_transforms[BoneIndices.Num() - 1] = FBoneTransform(TipBoneIndex, offseted_Transform_Local);


	Chain.Add(DragonChainLink(offseted_Transform_Local.GetLocation(), 0.f, TipBoneIndex, (BoneIndices.Num() - 1) * 1));

	output.PelvicEffectorTransform = RootEffectorTransform;



	const FVector relative_diff = (Chain[0].Position - MeshBases.GetComponentSpaceTransform(TipBoneIndex).GetLocation());


	// starting from spine_01 to effector point , loop around ...
	for (int32 TransformIndex = NumTransforms - 2; TransformIndex > -1; TransformIndex--)
	{




		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];

		const FTransform& BoneCSTransform_T_Index = MeshBases.GetComponentSpaceTransform(BoneIndex);


		FTransform offseted_Transform_T_Index = BoneCSTransform_T_Index;
		offseted_Transform_T_Index.SetLocation(BoneCSTransform_T_Index.GetLocation() + relative_diff);


		FVector const BoneCSPosition = offseted_Transform_T_Index.GetLocation();

		output.temp_transforms[TransformIndex] = FBoneTransform(BoneIndex, offseted_Transform_T_Index);

		// Calculate the combined length of this segment of skeleton



		float const BoneLength = FVector::Dist(BoneCSPosition, output.temp_transforms[TransformIndex + 1].Transform.GetLocation());


		if (!FMath::IsNearlyZero(BoneLength))
		{
			//Then add chain link with the respective bone
			Chain.Add(DragonChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));

			//Update maximum reach
			MaximumReach += BoneLength;

		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.

			//Otherwise if bone length is zero , then add it to child zero indice array
			DragonChainLink & ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}


	float const Pelvic_Chest_distance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);


	//	CSEffectorLocation = MeshBases.GetComponentSpaceTransform(BoneIndices[0]).GetLocation();
	//	CSEffectorLocation = Chain[Chain.Num() - 1].Position;


	FVector const cons_CSEffectorLocation = CSEffectorLocation;



	//	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*MaximumReach*1.2f;

	/*
	if(full_extend==false)
	{

	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) > (Pelvic_Chest_distance)*1.20f)
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*Pelvic_Chest_distance*1.20f;
	}
	else
	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) < (Pelvic_Chest_distance)*0.80f)
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*(Pelvic_Chest_distance)*0.80f;
	}


	}
	else*/
	{
		CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*Pelvic_Chest_distance;
	}


	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, "% decrease in size" + FString::SanitizeFloat( (FVector::Dist(Chain[0].Position, CSEffectorLocation) / Pelvic_Chest_distance)*100) +" %");


	float percentage_decrease = FMath::Clamp<float>((FVector::Dist(Chain[0].Position, CSEffectorLocation) / Pelvic_Chest_distance), 0, 1);



	bool bBoneLocationUpdated = false;
	output.is_moved = false;
	float const RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	int32 const NumChainLinks = Chain.Num();
	output.NumChainLinks = NumChainLinks;



	{
		//TipBoneLink index is total number of bones minus 1 .... i dont know why...
		int32 const TipBoneLinkIndex = NumChainLinks - 1;

		// DISTANCE BW  TIP TO EFFECTOR LOCATION
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);

		// IF SLOP > PRECISION (EG:- 0.5) THEN PROCEED WITH THE CALCULATION
		if (Slop > Precision)
		{
			// Set tip bone at end effector location.
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < MaxIterations))
			{
				// "Forward Reaching" stage - adjust bones from end effector.
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{

					//IN THE FR-STAGE , WE TRAVEL FROM TIP POINT INDEX TILL THE ROOT

					DragonChainLink & CurrentLink = Chain[LinkIndex];
					DragonChainLink const & ChildLink = Chain[LinkIndex + 1];


					//Current link position = child position + (direction from child to current link)*boneLength
					CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length*percentage_decrease);

				}

				// "Backward Reaching" stage - adjust bones from root.
				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{

					//IN THE BR-STAGE , WE TRAVEL FROM SPINE_01 TO TIP BONE INDEX


					DragonChainLink const & ParentLink = Chain[LinkIndex - 1];

					DragonChainLink & CurrentLink = Chain[LinkIndex];

					{
						CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length*percentage_decrease);
					}

				}

				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}

			// Place tip bone based on how close we got to target.
			{
				DragonChainLink const & ParentLink = Chain[TipBoneLinkIndex - 1];
				DragonChainLink & CurrentLink = Chain[TipBoneLinkIndex];

				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length*percentage_decrease);
			}

			bBoneLocationUpdated = true;
			output.is_moved = true;
		}
	}


	output.chain_data = Chain;


	return output;

}

DragonSpineOutput FAnimNode_DragonSpineSolver::DragonSpineProcessor_Direct(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	//Declare output
	DragonSpineOutput output = DragonSpineOutput();

	FTransform CSEffectorTransform = EffectorTransform;


	//GEngine->AddOnScreenDebugMessage(-1, 0.025f, FColor::Red, " Spine[0] : " + spine_Feet_pair[0].Spine_Involved.BoneName.ToString()+" Spine[1] : "+ spine_Feet_pair[spine_Feet_pair.Num()-1].Spine_Involved.BoneName.ToString());

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentTransform(), MeshBases, CSEffectorTransform, spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer), EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();


	// Gather all bone indices between root and tip.
	const TArray<FCompactPoseBoneIndex> BoneIndices = Spine_Indices;


	//Supply bone indices array to output struct
	output.BoneIndices = BoneIndices;

	// Maximum length of skeleton segment at full extension . Default set to 0
	float MaximumReach = 0;
	//	float MaximumSecReach = 0;

	// Gather transforms and define total num of bones present
	int32 const NumTransforms = BoneIndices.Num();
	//	int32 const Num2Transforms = BoneSecIndices.Num();


	//Temp transform is the actual bone data used
	//Initialize numTransform amount of dummy data to temp_transforms
	output.temp_transforms.AddUninitialized(NumTransforms);


	//Declare are initialize dummy data for the chain struct array
	TArray<DragonChainLink> Chain;
	Chain.Reserve(NumTransforms);


	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));



	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());




	float scale_mod = 1;

	if (owning_skel->GetOwner() != nullptr)
	{
		scale_mod = owning_skel->GetOwner()->GetActorScale().Z;
	}

	float pelvis_distance = FMath::Abs(lerp_data.Z - owning_skel->GetBoneLocation(owning_skel->GetBoneName(0)).Z);

	FVector root_difference(0, 0, 0);

	FVector root_cs_position;

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);

		root_cs_position = BoneCSTransform.GetLocation();

		//Set root transform to 0th temp transform
		output.temp_transforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);

		//	if (spine_hit_pairs[0].RV_Feet_Hits[0].bBlockingHit && spine_hit_pairs[0].RV_Feet_Hits[1].bBlockingHit)
		{

			Chain.Add(DragonChainLink(owning_skel->GetComponentTransform().InverseTransformPosition(RootEffectorTransform.GetLocation()), 0.f, RootBoneIndex, 0));

			output.PelvicEffectorTransform = RootEffectorTransform;

		}

	}


	// starting from spine_01 to effector point , loop around ...
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{


		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];

		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FVector const BoneCSPosition = BoneCSTransform.GetLocation();

		FTransform parent_trans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);

		parent_trans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);


		const FTransform& ParentCSTransform = parent_trans;


		output.temp_transforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);


		/*
		* Calculate total distance from current bone to parent bone
		*/
		float const BoneLength = FVector::Dist(BoneCSPosition, ParentCSTransform.GetLocation());

		//if bone length is not zero
		if (!FMath::IsNearlyZero(BoneLength))
		{
			//Then add chain link with the respective bone
			Chain.Add(DragonChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));

			//Update maximum reach
			MaximumReach += BoneLength;

		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.

			//Otherwise if bone length is zero , then add it to child zero indice array
			DragonChainLink & ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	//	float const Pelvic_Chest_distance = FVector::Dist(MeshBases.GetComponentSpaceTransform(BoneIndices[0]).GetLocation() , MeshBases.GetComponentSpaceTransform(BoneIndices[BoneIndices.Num()-1]).GetLocation());

	float const Pelvic_Chest_distance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);


	FVector const cons_CSEffectorLocation = CSEffectorLocation;

	//	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*MaximumReach*1.2f;


	/*
	if (full_extend == false)
	{

	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) > (Pelvic_Chest_distance))
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*(Pelvic_Chest_distance);
	}
	else
	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) < (MaximumReach)*0.80f)
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*(MaximumReach)*0.80f;
	}

	}
	else*/
	{
		CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*MaximumReach*1.05f;
	}


	bool bBoneLocationUpdated = false;
	output.is_moved = false;
	float const RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	int32 const NumChainLinks = Chain.Num();
	output.NumChainLinks = NumChainLinks;



	{
		//TipBoneLink index is total number of bones minus 1 .... i dont know why...
		int32 const TipBoneLinkIndex = NumChainLinks - 1;

		// DISTANCE BW  TIP TO EFFECTOR LOCATION
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);

		// IF SLOP > PRECISION (EG:- 0.5) THEN PROCEED WITH THE CALCULATION
		if (Slop > Precision)
		{
			// Set tip bone at end effector location.
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < MaxIterations))
			{
				// "Forward Reaching" stage - adjust bones from end effector.
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{

					//IN THE FR-STAGE , WE TRAVEL FROM TIP POINT INDEX TILL THE ROOT

					DragonChainLink & CurrentLink = Chain[LinkIndex];
					DragonChainLink const & ChildLink = Chain[LinkIndex + 1];


					//Current link position = child position + (direction from child to current link)*boneLength
					CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);

				}

				// "Backward Reaching" stage - adjust bones from root.
				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{

					//IN THE BR-STAGE , WE TRAVEL FROM SPINE_01 TO TIP BONE INDEX


					DragonChainLink const & ParentLink = Chain[LinkIndex - 1];

					DragonChainLink & CurrentLink = Chain[LinkIndex];

					{
						CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
					}

				}

				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}

			// Place tip bone based on how close we got to target.
			{
				DragonChainLink const & ParentLink = Chain[TipBoneLinkIndex - 1];
				DragonChainLink & CurrentLink = Chain[TipBoneLinkIndex];

				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
			}

			bBoneLocationUpdated = true;
			output.is_moved = true;
		}
	}


	output.chain_data = Chain;


	return output;
}





















DragonSpineOutput FAnimNode_DragonSpineSolver::DragonSpineProcessor_Snake(FTransform& EffectorTransform, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	//Declare output
	DragonSpineOutput output = DragonSpineOutput();

	FTransform CSEffectorTransform = EffectorTransform;


	//GEngine->AddOnScreenDebugMessage(-1, 0.025f, FColor::Red, " Spine[0] : " + spine_Feet_pair[0].Spine_Involved.BoneName.ToString()+" Spine[1] : "+ spine_Feet_pair[spine_Feet_pair.Num()-1].Spine_Involved.BoneName.ToString());

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentTransform(), MeshBases, CSEffectorTransform, spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer), EBoneControlSpace::BCS_WorldSpace);

	FVector CSEffectorLocation = CSEffectorTransform.GetLocation();


	// Gather all bone indices between root and tip.
	const TArray<FCompactPoseBoneIndex> BoneIndices = Spine_Indices;


	//Supply bone indices array to output struct
	output.BoneIndices = BoneIndices;

	// Maximum length of skeleton segment at full extension . Default set to 0
	float MaximumReach = 0;
	//	float MaximumSecReach = 0;

	// Gather transforms and define total num of bones present
	int32 const NumTransforms = BoneIndices.Num();
	//	int32 const Num2Transforms = BoneSecIndices.Num();


	//Temp transform is the actual bone data used
	//Initialize numTransform amount of dummy data to temp_transforms
	output.temp_transforms.AddUninitialized(NumTransforms);


	//Declare are initialize dummy data for the chain struct array
	TArray<DragonChainLink> Chain;
	Chain.Reserve(NumTransforms);


	FTransform RootTraceTransform = MeshBases.GetComponentSpaceTransform(spine_Feet_pair[0].Spine_Involved.GetCompactPoseIndex(*SavedBoneContainer));



	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(RootTraceTransform.GetLocation());




	float scale_mod = 1;

	if (owning_skel->GetOwner() != nullptr)
	{
		scale_mod = owning_skel->GetOwner()->GetActorScale().Z;
	}

	float pelvis_distance = FMath::Abs(lerp_data.Z - owning_skel->GetBoneLocation(owning_skel->GetBoneName(0)).Z);

	FVector root_difference(0, 0, 0);

	FVector root_cs_position;

	// Start with Root Bone
	{
		const FCompactPoseBoneIndex& RootBoneIndex = BoneIndices[0];
		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(RootBoneIndex);

		root_cs_position = BoneCSTransform.GetLocation();

		//Set root transform to 0th temp transform
		output.temp_transforms[0] = FBoneTransform(RootBoneIndex, BoneCSTransform);

		//	if (spine_hit_pairs[0].RV_Feet_Hits[0].bBlockingHit && spine_hit_pairs[0].RV_Feet_Hits[1].bBlockingHit)
		{

			Chain.Add(DragonChainLink(owning_skel->GetComponentTransform().InverseTransformPosition(RootEffectorTransform.GetLocation()), 0.f, RootBoneIndex, 0));

			output.PelvicEffectorTransform = RootEffectorTransform;

		}

	}


	// starting from spine_01 to effector point , loop around ...
	for (int32 TransformIndex = 1; TransformIndex < NumTransforms; TransformIndex++)
	{


		const FCompactPoseBoneIndex& BoneIndex = BoneIndices[TransformIndex];

		const FTransform& BoneCSTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FVector const BoneCSPosition = BoneCSTransform.GetLocation();

		FTransform parent_trans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);

		parent_trans = MeshBases.GetComponentSpaceTransform(BoneIndices[TransformIndex - 1]);


		const FTransform& ParentCSTransform = parent_trans;


		output.temp_transforms[TransformIndex] = FBoneTransform(BoneIndex, BoneCSTransform);


		/*
		* Calculate total distance from current bone to parent bone
		*/
		float const BoneLength = FVector::Dist(BoneCSPosition, ParentCSTransform.GetLocation());

		//if bone length is not zero
		if (!FMath::IsNearlyZero(BoneLength))
		{
			//Then add chain link with the respective bone
			Chain.Add(DragonChainLink(BoneCSPosition, BoneLength, BoneIndex, TransformIndex));

			//Update maximum reach
			MaximumReach += BoneLength;

		}
		else
		{
			// Mark this transform as a zero length child of the last link.
			// It will inherit position and delta rotation from parent link.

			//Otherwise if bone length is zero , then add it to child zero indice array
			DragonChainLink & ParentLink = Chain[Chain.Num() - 1];
			ParentLink.ChildZeroLengthTransformIndices.Add(TransformIndex);
		}
	}

	//	float const Pelvic_Chest_distance = FVector::Dist(MeshBases.GetComponentSpaceTransform(BoneIndices[0]).GetLocation() , MeshBases.GetComponentSpaceTransform(BoneIndices[BoneIndices.Num()-1]).GetLocation());

	float const Pelvic_Chest_distance = FVector::Dist(Chain[Chain.Num() - 1].Position, Chain[0].Position);


	FVector const cons_CSEffectorLocation = CSEffectorLocation;

	//	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*MaximumReach*1.2f;


	/*
	if (full_extend == false)
	{

	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) > (Pelvic_Chest_distance))
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*(Pelvic_Chest_distance);
	}
	else
	if (FVector::Dist(Chain[0].Position, CSEffectorLocation) < (MaximumReach)*0.80f)
	{
	CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*(MaximumReach)*0.80f;
	}

	}
	else*/
	{
//		CSEffectorLocation = Chain[0].Position + (CSEffectorLocation - Chain[0].Position).GetSafeNormal()*MaximumReach*1.05f;
	}


	bool bBoneLocationUpdated = false;
	output.is_moved = false;
	float const RootToTargetDistSq = FVector::DistSquared(Chain[0].Position, CSEffectorLocation);
	int32 const NumChainLinks = Chain.Num();
	output.NumChainLinks = NumChainLinks;



	{
		//TipBoneLink index is total number of bones minus 1 .... i dont know why...
		int32 const TipBoneLinkIndex = NumChainLinks - 1;

		// DISTANCE BW  TIP TO EFFECTOR LOCATION
		float Slop = FVector::Dist(Chain[TipBoneLinkIndex].Position, CSEffectorLocation);

		// IF SLOP > PRECISION (EG:- 0.5) THEN PROCEED WITH THE CALCULATION
		if (Slop > Precision)
		{
			// Set tip bone at end effector location.
			Chain[TipBoneLinkIndex].Position = CSEffectorLocation;

			int32 IterationCount = 0;
			while ((Slop > Precision) && (IterationCount++ < MaxIterations))
			{
				// "Forward Reaching" stage - adjust bones from end effector.
				for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
				{

					//IN THE FR-STAGE , WE TRAVEL FROM TIP POINT INDEX TILL THE ROOT

					DragonChainLink & CurrentLink = Chain[LinkIndex];
					DragonChainLink const & ChildLink = Chain[LinkIndex + 1];


					//Current link position = child position + (direction from child to current link)*boneLength
				//	CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);

				    //CurrentLink.Position = (ChildLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);

					

						if (atleast_one_hit && Enable_Solver)
						{

							if (((ChildLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length) - snake_spine_positions[LinkIndex]).Size() > 1000 * component_scale)
							{

								CurrentLink.Position = (ChildLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
								snake_spine_positions[LinkIndex] = CurrentLink.Position;

							}
							else
							{
								snake_spine_positions[LinkIndex] = UKismetMathLibrary::VInterpTo(snake_spine_positions[LinkIndex], (ChildLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Snake_Joint_Speed* 0.05f);
								CurrentLink.Position = snake_spine_positions[LinkIndex];

							}
						}
						else
						{
							CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length);
							snake_spine_positions[LinkIndex] = CurrentLink.Position;

							//	snake_spine_positions[LinkIndex] = UKismetMathLibrary::VInterpTo(snake_spine_positions[LinkIndex], (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Snake_Joint_Speed* 0.05f);
							//	CurrentLink.Position = snake_spine_positions[LinkIndex];

						}


					//						snake_spine_positions[LinkIndex] = UKismetMathLibrary::VInterpTo(snake_spine_positions[LinkIndex], CurrentLink.Position = (ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Snake_Joint_Speed* 0.05f);
					
				//	CurrentLink.Position = spine_between_offseted_transforms[LinkIndex-1];

				}

				// "Backward Reaching" stage - adjust bones from root.
				for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
				{

					//IN THE BR-STAGE , WE TRAVEL FROM SPINE_01 TO TIP BONE INDEX


					DragonChainLink const & ParentLink = Chain[LinkIndex - 1];

					DragonChainLink & CurrentLink = Chain[LinkIndex];

					{
					//	CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
					}

					
					
					if (atleast_one_hit && Enable_Solver)
					{

					if (( (ParentLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length)- snake_spine_positions[LinkIndex] ).Size() > 1000*component_scale)
					{

						CurrentLink.Position = (ParentLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
						snake_spine_positions[LinkIndex] = CurrentLink.Position;

					}
					else
					{
						snake_spine_positions[LinkIndex] = UKismetMathLibrary::VInterpTo(snake_spine_positions[LinkIndex], (ParentLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Snake_Joint_Speed* 0.05f);
						CurrentLink.Position = snake_spine_positions[LinkIndex];
					}

					}
					else
					{
						//CurrentLink.Position = (ParentLink.Position + (spine_between_offseted_transforms[LinkIndex - 1] - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);

						CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
						snake_spine_positions[LinkIndex] = CurrentLink.Position;


					//	snake_spine_positions[LinkIndex] = UKismetMathLibrary::VInterpTo(snake_spine_positions[LinkIndex], (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Snake_Joint_Speed* 0.05f);
					//	CurrentLink.Position = snake_spine_positions[LinkIndex];

					}
					
					

//					OutputTransform.SetLocation(UKismetMathLibrary::VInterpTo(FVector(final_transformation.GetLocation().X, final_transformation.GetLocation().Y, OutputTransform.GetLocation().Z), final_transformation.GetLocation() + FVector(0, 0, base_offset), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()), Location_Lerp_Speed* 0.05f));


					//CurrentLink.Position = spine_between_offseted_transforms[LinkIndex-1];

				}

				// Re-check distance between tip location and effector location
				// Since we're keeping tip on top of effector location, check with its parent bone.
				Slop = FMath::Abs(Chain[TipBoneLinkIndex].Length - FVector::Dist(Chain[TipBoneLinkIndex - 1].Position, CSEffectorLocation));
			}

			// Place tip bone based on how close we got to target.
			{
				DragonChainLink const & ParentLink = Chain[TipBoneLinkIndex - 1];
				DragonChainLink & CurrentLink = Chain[TipBoneLinkIndex];

				CurrentLink.Position = (ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length);
			}

			bBoneLocationUpdated = true;
			output.is_moved = true;
		}
	}


	output.chain_data = Chain;


	return output;
}









void FAnimNode_DragonSpineSolver::OrthoNormalize(FVector& Normal, FVector& Tangent)
{
	Normal = Normal.GetSafeNormal();
	Tangent = Tangent - (Normal * FVector::DotProduct(Tangent, Normal));
	Tangent = Tangent.GetSafeNormal();
}

FQuat FAnimNode_DragonSpineSolver::LookRotation(FVector lookAt, FVector upDirection) {

	FVector forward = lookAt;
	FVector up = upDirection;

	OrthoNormalize(forward, up);


	FVector right = FVector::CrossProduct(up, forward);

#define m00 right.X

#define m01 up.X

#define m02 forward.X

#define m10 right.Y

#define m11 up.Y

#define m12 forward.Y

#define m20 right.Z

#define m21 up.Z

#define m22 forward.Z



	float num8 = (m00 + m11) + m22;
	FQuat quaternion = FQuat();
	if (num8 > 0.0f)
	{
		float num = (float)FMath::Sqrt(num8 + 1.0f);
		quaternion.W = num * 0.5f;
		num = 0.5f / num;
		quaternion.X = (m12 - m21) * num;
		quaternion.Y = (m20 - m02) * num;
		quaternion.Z = (m01 - m10) * num;
		return quaternion;
	}
	if ((m00 >= m11) && (m00 >= m22))
	{
		float num7 = (float)FMath::Sqrt(((1.0f + m00) - m11) - m22);
		float num4 = 0.5f / num7;
		quaternion.X = 0.5f * num7;
		quaternion.Y = (m01 + m10) * num4;
		quaternion.Z = (m02 + m20) * num4;
		quaternion.W = (m12 - m21) * num4;
		return quaternion;
	}
	if (m11 > m22)
	{
		float num6 = (float)FMath::Sqrt(((1.0f + m11) - m00) - m22);
		float num3 = 0.5f / num6;
		quaternion.X = (m10 + m01) * num3;
		quaternion.Y = 0.5f * num6;
		quaternion.Z = (m21 + m12) * num3;
		quaternion.W = (m20 - m02) * num3;
		return quaternion;
	}
	float num5 = (float)FMath::Sqrt(((1.0f + m22) - m00) - m11);
	float num2 = 0.5f / num5;
	quaternion.X = (m20 + m02) * num2;
	quaternion.Y = (m21 + m12) * num2;
	quaternion.Z = 0.5f * num5;
	quaternion.W = (m01 - m10) * num2;



#undef m00

#undef m01

#undef m02

#undef m10

#undef m11

#undef m12

#undef m20

#undef m21

#undef m22

	return quaternion;

	//	return ret;

}


DragonSpineOutput FAnimNode_DragonSpineSolver::Dragon_Transformation(DragonSpineOutput& input, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{



	//	if (input.is_moved)
	{
		// First step: update bone transform positions from chain links.
		for (int32 LinkIndex = 0; LinkIndex < input.NumChainLinks; LinkIndex++)
		{
			//		if (LinkIndex != 0)
			{

				DragonChainLink const & ChainLink = input.chain_data[LinkIndex];



				const FCompactPoseBoneIndex ModifyBoneIndex = input.BoneIndices[ChainLink.TransformIndex];

				FTransform ComponentBoneTransform;

				ComponentBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

				FTransform chainTransform = FTransform::Identity;

				{
					chainTransform.SetLocation(ChainLink.Position);

				}


				//FVector transposed_position = ComponentBoneTransform.InverseTransformPosition(ChainLink.Position);

				//				OutBoneTransforms[ChainLink.TransformIndex].Transform.SetTranslation(chainTransform.GetLocation());

				input.temp_transforms[ChainLink.TransformIndex].Transform.SetTranslation(chainTransform.GetLocation());


				//input.temp_transforms[ChainLink.TransformIndex].Transform.SetTranslation(UKismetMathLibrary::VInterpTo( input.temp_transforms[ChainLink.TransformIndex].Transform.GetLocation(),chainTransform.GetLocation(), 1 - FMath::Exp(-Smooth_Factor * owning_skel->GetWorld()->GetDeltaSeconds()),Location_Lerp_Speed));


//				input.temp_transforms[ChainLink.TransformIndex].Transform.SetTranslation(chainTransform.GetLocation());

				// If there are any zero length children, update position of those
				int32 const NumChildren = ChainLink.ChildZeroLengthTransformIndices.Num();
				for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
				{
					//					OutBoneTransforms[ChainLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform.SetTranslation(ChainLink.Position);
					input.temp_transforms[ChainLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform.SetTranslation(ChainLink.Position);
				}

			}

		}


		FRotator initial_rot_delta(0, 0, 0);


		bool is_chain_swapped = false;

		do
		{
			is_chain_swapped = false;

			for (int i = 1; i < input.chain_data.Num(); i++)
			{
				if (input.chain_data[i - 1].BoneIndex > input.chain_data[i].BoneIndex)
				{
					DragonChainLink temp = input.chain_data[i - 1];
					input.chain_data[i - 1] = input.chain_data[i];
					input.chain_data[i] = temp;

					is_chain_swapped = true;
				}

			}
		} while (is_chain_swapped);



		// FABRIK algorithm - re-orientation of bone local axes after translation calculation
		for (int32 LinkIndex = 0; LinkIndex < input.NumChainLinks - 1; LinkIndex++)
		{
			DragonChainLink const & CurrentLink = input.chain_data[LinkIndex];
			DragonChainLink const & ChildLink = input.chain_data[LinkIndex + 1];

			// Calculate pre-translation vector between this bone and child
			FVector const OldDir = (GetCurrentLocation(MeshBases, ChildLink.BoneIndex) - GetCurrentLocation(MeshBases, CurrentLink.BoneIndex)).GetUnsafeNormal();

			// Get vector from the post-translation bone to it's child
			FVector const NewDir = (ChildLink.Position - CurrentLink.Position).GetUnsafeNormal();

			// Calculate axis of rotation from pre-translation vector to post-translation vector
			FVector const RotationAxis = FVector::CrossProduct(OldDir, NewDir).GetSafeNormal();
			float const RotationAngle = FMath::Acos(FVector::DotProduct(OldDir, NewDir));
			FQuat const DeltaRotation = FQuat(RotationAxis, RotationAngle);
			// We're going to multiply it, in order to not have to re-normalize the final quaternion, it has to be a unit quaternion.
			checkSlow(DeltaRotation.IsNormalized());


			// Calculate absolute rotation and set it
			//			FTransform& CurrentBoneTransform = OutBoneTransforms[CurrentLink.TransformIndex].Transform;
			FTransform& CurrentBoneTransform = input.temp_transforms[CurrentLink.TransformIndex].Transform;


			FTransform const ConstBoneTransform = input.temp_transforms[CurrentLink.TransformIndex].Transform;

//			if (LinkIndex == 0 || Total_spine_bones[LinkIndex] == spine_Feet_pair[0].Spine_Involved.BoneName)

//			if (LinkIndex == 0 && is_snake == false)
			// && feet_is_empty==false

			if (LinkIndex == 0&& feet_is_empty ==false)
			{

				/*
				if (is_snake)
				{

					FTransform Rotation_Tip = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);

					input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(Rotation_Tip.GetRotation());
				}
				else*/
				{
					FRotator direction_rot = BoneRelativeConversion(CurrentLink.BoneIndex, input.PelvicEffectorTransform.Rotator(), *SavedBoneContainer, MeshBases);

					input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(direction_rot.Quaternion());
				}

			}
			else
			{

//				FTransform Rotation_Tip = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);

//				input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(Rotation_Tip.GetRotation());



				CurrentBoneTransform.SetRotation(FQuat::Slerp(CurrentBoneTransform.GetRotation(), (DeltaRotation*1.0f) * CurrentBoneTransform.GetRotation(), rotation_power_between));


				input.temp_transforms[CurrentLink.TransformIndex].Transform = CurrentBoneTransform;
			}



//			FTransform Rotation_Tip = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);
//			input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(Rotation_Tip.GetRotation());



			// Update zero length children if any
			int32 const NumChildren = CurrentLink.ChildZeroLengthTransformIndices.Num();
			for (int32 ChildIndex = 0; ChildIndex < NumChildren; ChildIndex++)
			{

				FTransform& ChildBoneTransform = input.temp_transforms[CurrentLink.ChildZeroLengthTransformIndices[ChildIndex]].Transform;
				ChildBoneTransform.NormalizeRotation();

			}
		}


		//FINAL LINK TRANSFORMATION

		DragonChainLink const & CurrentLink = input.chain_data[input.NumChainLinks - 1];

		if ((input.NumChainLinks - 2) > 0)
			DragonChainLink const & ChildLink = input.chain_data[input.NumChainLinks - 2];


		FRotator direction_rot = BoneRelativeConversion(CurrentLink.BoneIndex, input.SpineFirstEffectorTransform.Rotator(), *SavedBoneContainer, MeshBases);

/*
		if (feet_is_empty)
		{
			FTransform Rotation_Tip = MeshBases.GetComponentSpaceTransform(CurrentLink.BoneIndex);
			input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(Rotation_Tip.GetRotation());
		}
		else*/
		{
			input.temp_transforms[CurrentLink.TransformIndex].Transform.SetRotation(direction_rot.Quaternion());
		}



	}




	return input;

}







FRotator FAnimNode_DragonSpineSolver::BoneRelativeConversion_LEGACY(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases)
{
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

	FTransform ComponentTransform = owning_skel->GetComponentTransform();

	const FTransform C_ComponentTransform = owning_skel->GetComponentTransform();


	// Convert to Bone Space.
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	float temp_yaw = ComponentTransform.Rotator().Yaw;

	FQuat actor_rotation_world(0, 0, 0, 0);

	if (owning_skel->GetOwner() != nullptr)
	{
		actor_rotation_world = owning_skel->GetOwner()->GetActorRotation().Quaternion().Inverse()*FRotator(NewBoneTM.GetRotation()).Quaternion();

		FRotator make_rot = NewBoneTM.Rotator();

		make_rot.Yaw = FRotator(actor_rotation_world).Yaw;



		NewBoneTM.SetRotation(make_rot.Quaternion());

	}

	const FQuat BoneQuat(target_rotation);

	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());

	// Convert back to Component Space.
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

	return NewBoneTM.Rotator();
}




FRotator FAnimNode_DragonSpineSolver::BoneRelativeConversion(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases)
{
	//FCompactPoseBoneIndex CompactPoseBoneToModify = BoneToModify.GetCompactPoseIndex(BoneContainer);
	FTransform NewBoneTM_Legacy = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	//	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FTransform ComponentTransform = owning_skel->GetComponentTransform();


	FTransform BoneQuat_Legacy(target_rotation);

	//Convert input to CS .   input * [CS_rot*relative_skel_rot]
	//DIFF = Convert input to WS . input*
	NewBoneTM_Legacy.SetRotation(owning_skel->GetRelativeTransform().Rotator().Quaternion() * NewBoneTM_Legacy.GetRotation());

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, MeshBases, BoneQuat_Legacy, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);


	NewBoneTM_Legacy.SetRotation(BoneQuat_Legacy.GetRotation() * NewBoneTM_Legacy.GetRotation());

	FRotator final_rotator = NewBoneTM_Legacy.Rotator();

	return final_rotator;


	//	return FRotator();
}





FVector FAnimNode_DragonSpineSolver::GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex)
{
	return MeshBases.GetComponentSpaceTransform(BoneIndex).GetLocation();
}

