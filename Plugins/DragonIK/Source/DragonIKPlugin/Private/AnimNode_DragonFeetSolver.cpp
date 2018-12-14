/* Copyright (C) Gamasome Interactive LLP, Inc - All Rights Reserved
* Unauthorized copying of this file, via any medium is strictly prohibited
* Proprietary and confidential
* Written by Mansoor Pathiyanthra <codehawk64@gmail.com , mansoor@gamasome.com>, July 2018
*/


#include "AnimNode_DragonFeetSolver.h"
#include "DragonIKPlugin.h"
#include "Animation/AnimInstanceProxy.h"

#include "AnimationRuntime.h"
#include "AnimationCoreLibrary.h"
#include "Algo/Reverse.h"






// Initialize the component pose as well as defining the owning skeleton
void FAnimNode_DragonFeetSolver::Initialize_AnyThread(const FAnimationInitializeContext & Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);
	ComponentPose.Initialize(Context);
	owning_skel = Context.AnimInstanceProxy->GetSkelMeshComponent();
	//	dragon_bone_data.Start_Spine = FBoneReference(dragon_input_data.Start_Spine);
}



// Cache the bones . Thats it !!
void FAnimNode_DragonFeetSolver::CacheBones_AnyThread(const FAnimationCacheBonesContext & Context)
{
	FAnimNode_Base::CacheBones_AnyThread(Context);
	ComponentPose.CacheBones(Context);
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
}


// Main update function . Do not perform any changed !!
void FAnimNode_DragonFeetSolver::Update_AnyThread(const FAnimationUpdateContext & Context)
{
	ComponentPose.Update(Context);
	ActualAlpha = 0.f;


	if (IsLODEnabled(Context.AnimInstanceProxy,LODThreshold))
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







void FAnimNode_DragonFeetSolver::Make_All_Bones(FCSPose<FCompactPose>& MeshBases)
{




}




// Store the zero pose transform data
void FAnimNode_DragonFeetSolver::GetResetedPoseInfo(FCSPose<FCompactPose>& MeshBases)
{

}


// Store the animated and calculated pose transform data
void FAnimNode_DragonFeetSolver::GetAnimatedPoseInfo(FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	int32 const NumTransforms = combined_indices.Num() - 1;
	OutBoneTransforms.AddUninitialized(NumTransforms);
	for (int i = 0; i < NumTransforms; i++)
	{

		FTransform updated_transform = MeshBases.GetComponentSpaceTransform(combined_indices[i]);
		FRotator delta_diff = (AnimatedBoneTransforms[i].Transform.Rotator() - RestBoneTransforms[i].Transform.Rotator()).GetNormalized();
		updated_transform.SetRotation((updated_transform.Rotator() + delta_diff).GetNormalized().Quaternion());

		FVector delta_pos_diff = AnimatedBoneTransforms[i].Transform.GetLocation() - RestBoneTransforms[i].Transform.GetLocation();
		updated_transform.SetLocation(updated_transform.GetLocation() + delta_pos_diff);
		OutBoneTransforms[i] = (FBoneTransform(combined_indices[i], updated_transform));
	}

}



void FAnimNode_DragonFeetSolver::GetFeetHeights(FComponentSpacePoseContext & Output)
{
	FTransform RootTraceTransform = Output.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));

	FeetRootHeights.Empty();

	float scale_mode = 1;

	if (owning_skel->GetOwner())
	{
		scale_mode = owning_skel->GetOwner()->GetActorScale().Z;
	}
	//	FeetRootHeights.AddUninitialized(spine_Feet_pair.Num());
	FeetRootHeights.AddDefaulted(spine_Feet_pair.Num());

	for (int i = 0; i < spine_Feet_pair.Num(); i++)
	{
		for (int j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
		{
			FTransform bonetraceTransform = Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[i].Associated_Feet[j].GetCompactPoseIndex(*SavedBoneContainer));
			//			float feet_root_height = (bonetraceTransform.GetLocation(), RootTraceTransform.GetLocation()).Size();

			FVector bone_location_ws = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());

			FVector zero_ws = owning_skel->GetComponentTransform().TransformPosition(FVector(0,0,0));


			if(Automatic_Foot_Height_Detection)
			FeetRootHeights[i].Add(FMath::Abs(bone_location_ws.Z -zero_ws.Z));
			else
			FeetRootHeights[i].Add(spine_Feet_pair[i].Feet_Heights[j]* owning_skel->GetComponentScale().Z);


			//FeetRootHeights[i].Add(FMath::Abs(10*owning_skel->GetComponentScale().Z));



		}

	}


}

void FAnimNode_DragonFeetSolver::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext & Output)
{


	// Apply the skeletal control if it's valid
	if (  (FVector(0,0,0)-Output.AnimInstanceProxy->GetActorTransform().GetScale3D()).Size()>0 &&FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()) && spine_Feet_pair.Num()>0 && Output.ContainsNaN() == false)
	{

		ComponentPose.EvaluateComponentSpace(Output);

		LineTraceControl_AnyThread(Output, BoneTransforms);






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


				/*
				const FCompactPoseBoneIndex ModifyBoneIndex_Knee_j = spine_Feet_pair[i].Associated_Knees[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Knee_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Knee_j);
				//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

				spine_AnimatedTransform_pairs[i].Associated_Knee[j] = (ComponentBoneTransform_Knee_j)*owning_skel->GetComponentTransform();
				*/


				if (every_foot_dont_have_child==false)
					if (spine_Feet_pair[i].Associated_Feet_Tips[j].IsValidToEvaluate())
					{

						


						const FCompactPoseBoneIndex ModifyBoneIndex_FeetBalls_k = spine_Feet_pair[i].Associated_Feet_Tips[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
						FTransform ComponentBoneTransform_FeetBalls_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_FeetBalls_k);
						//				FVector lerp_data_local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Knee_j.GetLocation());

						spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j] = (ComponentBoneTransform_FeetBalls_j)*owning_skel->GetComponentTransform();
					}


			}
		}




		ComponentPose.EvaluateComponentSpace(Output);




		TArray<TArray<FTransform>> feet_rotation_array = TArray<TArray<FTransform>>();


		for (int spine_index = 0; spine_index < spine_hit_pairs.Num(); spine_index++)
		{
			feet_rotation_array.Add(TArray<FTransform>());



			for (int feet_index = 0; feet_index < spine_hit_pairs[spine_index].RV_Feet_Hits.Num(); feet_index++)
			{

				FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex);

				//FTransform EndBoneCSTransform = FTransform::Identity;


				feet_rotation_array[spine_index].Add(EndBoneCSTransform);
			}
		}



		Output.ResetToRefPose();



		GetFeetHeights(Output);




		for (int spine_index = 0; spine_index < spine_hit_pairs.Num(); spine_index++)
		{

			for (int feet_index = 0; feet_index < spine_hit_pairs[spine_index].RV_Feet_Hits.Num(); feet_index++)
			{

				FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex);


				if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit)
				{



					FTransform LocalBoneTransform = Output.Pose.GetLocalSpaceTransform(spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex);



					if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit&& atleast_one_hit)
					{



						


							FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentToWorld(), Output.Pose, EndBoneCSTransform, spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex,EBoneControlSpace::BCS_WorldSpace);


							FTransform const original_rot = feet_rotation_array[spine_index][feet_index];


							FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentToWorld(), Output.Pose, feet_rotation_array[spine_index][feet_index], spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex, EBoneControlSpace::BCS_WorldSpace);


							FVector normal_impact = spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal;
							FRotator normal_rot = FRotator(UKismetMathLibrary::DegAtan2(normal_impact.X, normal_impact.Z)*-1 , 0, UKismetMathLibrary::DegAtan2(normal_impact.Y, normal_impact.Z)*1);

							
							
						//	FRotator diff = feet_rotation_array[spine_index][feet_index].;
							FQuat rotDiff = (feet_rotation_array[spine_index][feet_index].Rotator().Quaternion()*EndBoneCSTransform.Rotator().Quaternion().Inverse()).GetNormalized();

							FRotator rot_f = FRotator(rotDiff);
							rot_f.Pitch = 0;
							rot_f.Roll = 0;

							rotDiff = rot_f.Quaternion();


//							FQuat rotDiff = (EndBoneCSTransform.Rotator().Quaternion()*feet_rotation_array[spine_index][feet_index].Rotator().Quaternion().Inverse()).GetNormalized();


							const FQuat BoneQuat(normal_rot.Quaternion());
							EndBoneCSTransform.SetRotation((BoneQuat * EndBoneCSTransform.GetRotation()).GetNormalized());


							if(use_ref_rotation==false)
							EndBoneCSTransform.SetRotation( (rotDiff * EndBoneCSTransform.GetRotation()).GetNormalized());

							// Convert back to Component Space.
							FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentToWorld(), Output.Pose, EndBoneCSTransform, spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex, EBoneControlSpace::BCS_WorldSpace);

							FRotator direction_rot = EndBoneCSTransform.Rotator();





							//FTransform EndBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

							//direction_rot = FRotator(owning_skel->GetComponentRotation().Quaternion().Inverse()*spine_Transform_pairs[spine_index].Associated_Feet[feet_index].GetRotation());

							//direction_rot = FRotator(FRotator( FMath::Clamp<float>( XZ_rot.Roll* Enable_Roll,-45,45),0, FMath::Clamp<float>(YZ_rot.Pitch*-1* Enable_Pitch,-45,45)).Quaternion() * direction_rot.Quaternion());

							//GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Foot Pitch : " + FString::SanitizeFloat(YZ_rot.Pitch));


						//	direction_rot.Yaw = EndBoneCSTransform.Rotator().Yaw;
//ORIGINAL							feet_mod_transform_array[spine_index][feet_index].SetRotation(UKismetMathLibrary::RInterpTo(feet_mod_transform_array[spine_index][feet_index].Rotator(), direction_rot, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()), shift_speed).Quaternion());


							feet_mod_transform_array[spine_index][feet_index].SetRotation(FQuat::Slerp(feet_mod_transform_array[spine_index][feet_index].GetRotation(), direction_rot.Quaternion(), (1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()))*shift_speed));



							//feet_mod_transform_array[spine_index][feet_index].SetRotation(direction_rot.Quaternion());

							//						EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());

							


						
					}
					else
					{
//						feet_mod_transform_array[spine_index][feet_index].SetRotation(UKismetMathLibrary::RInterpTo(feet_mod_transform_array[spine_index][feet_index].Rotator(), EndBoneCSTransform.Rotator(), 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()), shift_speed).Quaternion());
					}

				}
			}

		}



		EvaluateComponentSpaceInternal(Output);
		USkeletalMeshComponent* Component = Output.AnimInstanceProxy->GetSkelMeshComponent();
		AnimatedBoneTransforms.Reset(AnimatedBoneTransforms.Num());
		FinalBoneTransforms.Reset(FinalBoneTransforms.Num());
		//Get Initial Pose data
		GetResetedPoseInfo(Output.Pose);
		//Reset Bone Transform array
		BoneTransforms.Reset(BoneTransforms.Num());
		saved_pose = &Output;


		ComponentPose.EvaluateComponentSpace(Output);

		EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);





		bool is_swapped = false;

		do
		{
			is_swapped = false;


			for (int32 i = 1; i < BoneTransforms.Num(); i++)
			{
				if (BoneTransforms[i - 1].BoneIndex > BoneTransforms[i].BoneIndex)
				{
					FBoneTransform temp = BoneTransforms[i - 1];
					BoneTransforms[i - 1] = BoneTransforms[i];
					BoneTransforms[i] = temp;

					is_swapped = true;
				}
			}

		} while (is_swapped == true);




		//		GetAnimatedPoseInfo(Output.Pose, FinalBoneTransforms);
		//		checkSlow(!ContainsNaN(FinalBoneTransforms));

		if (BoneTransforms.Num() > 0)
		{
		//	for(int i=0;i<BoneTransforms.Num();i++)
		//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet : " + owning_skel->GetBoneName(BoneTransforms[i].BoneIndex.GetInt()).ToString());


			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
			Output.Pose.LocalBlendCSBoneTransforms(BoneTransforms, BlendWeight);
		}
	}
	else
	{
	//	Output.ResetToRefPose();
		ComponentPose.EvaluateComponentSpace(Output);
	}

}



//Perform update operations inside this
void FAnimNode_DragonFeetSolver::UpdateInternal(const FAnimationUpdateContext & Context)
{

	float scale_mode = 1;

	if (owning_skel->GetOwner())
	{
		scale_mode = owning_skel->GetComponentScale().Z;
	}



	TArray<FVector> feet_mid_points;
	feet_mid_points.AddUninitialized(spine_hit_pairs.Num());

	for (int32 i = 0; i < spine_hit_pairs.Num(); i++)
	{

		float chest_distance = FMath::Abs(spine_Transform_pairs[i].Spine_Involved.GetLocation().Z - owning_skel->GetComponentToWorld().GetLocation().Z);

		//if (spine_Transform_pairs[i].Associated_Feet.Num() > 0 && spine_hit_pairs[i].RV_Feet_Hits.Num() > 0)
		{
			for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
			{

				FVector offseted_linetrace_location = spine_Transform_pairs[i].Associated_Feet[j].GetLocation();


				//				line_trace_func(owning_skel, offseted_linetrace_location + FVector(0, 0, Total_spine_heights[i] * 1), offseted_linetrace_location - FVector(0, 0, FeetRootHeights[i][j] * 1), spine_hit_pairs[i].RV_Feet_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName, spine_Feet_pair[i].Associated_Feet[j].BoneName, spine_hit_pairs[i].RV_Feet_Hits[j]);
				FVector a = FVector(0, 0, FeetRootHeights[i][j] * 1);

				FHitResult d = spine_hit_pairs[i].RV_Feet_Hits[j];

				FName c = spine_Feet_pair[i].Associated_Feet[j].BoneName;




				//FVector feet_direction = (spine_AnimatedTransform_pairs[i].Associated_Knee[j].GetLocation() - spine_AnimatedTransform_pairs[i].Associated_Feet[j].GetLocation()).GetSafeNormal();

				/*
				line_trace_func(owning_skel
					, offseted_linetrace_location + (feet_direction*line_trace_upper_height * scale_mode),
					offseted_linetrace_location - (feet_direction* (FeetRootHeights[i][j] - line_trace_down_height)  ),
					spine_hit_pairs[i].RV_Feet_Hits[j],
					spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_hit_pairs[i].RV_Feet_Hits[j]);
				*/


				

				line_trace_func(owning_skel, offseted_linetrace_location + FVector(0, 0, line_trace_upper_height * scale_mode), 
					offseted_linetrace_location -FVector(0, 0, FeetRootHeights[i][j] + line_trace_down_height),
//					offseted_linetrace_location,
					spine_hit_pairs[i].RV_Feet_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_hit_pairs[i].RV_Feet_Hits[j], FLinearColor::Blue);


				/*

				FVector forward_dir = owning_skel->GetComponentToWorld().TransformVector(FVector(0,1,0)).GetSafeNormal();
				FVector right_dir = UKismetMathLibrary::GetRightVector(forward_dir.Rotation());




				line_trace_func(owning_skel, offseted_linetrace_location + forward_dir*10 + FVector(0, 0, line_trace_upper_height * scale_mode),
					offseted_linetrace_location + forward_dir * 10 - FVector(0, 0, FeetRootHeights[i][j] - line_trace_down_height),
					//					offseted_linetrace_location,
					spine_hit_pairs[i].RV_FeetFront_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_hit_pairs[i].RV_FeetFront_Hits[j], FLinearColor::Red);



				line_trace_func(owning_skel, offseted_linetrace_location + forward_dir * -10 + FVector(0, 0, line_trace_upper_height * scale_mode),
					offseted_linetrace_location + forward_dir * -10 - FVector(0, 0, FeetRootHeights[i][j] - line_trace_down_height),
					//					offseted_linetrace_location,
					spine_hit_pairs[i].RV_FeetBack_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_hit_pairs[i].RV_FeetBack_Hits[j], FLinearColor::Red);




				line_trace_func(owning_skel, offseted_linetrace_location+ right_dir*-10 + FVector(0, 0, line_trace_upper_height * scale_mode),
					offseted_linetrace_location + right_dir * -10 - FVector(0, 0, FeetRootHeights[i][j] - line_trace_down_height),
					//					offseted_linetrace_location,
					spine_hit_pairs[i].RV_FeetLeft_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_hit_pairs[i].RV_FeetLeft_Hits[j], FLinearColor::Yellow);



				line_trace_func(owning_skel, offseted_linetrace_location + right_dir * 10 + FVector(0, 0, line_trace_upper_height * scale_mode),
					offseted_linetrace_location + right_dir * 10 - FVector(0, 0, FeetRootHeights[i][j] - line_trace_down_height),
					//					offseted_linetrace_location,
					spine_hit_pairs[i].RV_FeetRight_Hits[j], spine_Feet_pair[i].Associated_Feet[j].BoneName
					, spine_Feet_pair[i].Associated_Feet[j].BoneName,
					spine_hit_pairs[i].RV_FeetRight_Hits[j], FLinearColor::Yellow);



					*/




				if (every_foot_dont_have_child == false)
				{
					line_trace_func(owning_skel
						, spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j].GetLocation() + FVector(0, 0, line_trace_upper_height * scale_mode),
						spine_AnimatedTransform_pairs[i].Associated_FeetBalls[j].GetLocation(),
						spine_hit_pairs[i].RV_FeetBalls_Hits[j],
						spine_Feet_pair[i].Associated_Feet_Tips[j].BoneName,
						spine_Feet_pair[i].Associated_Feet_Tips[j].BoneName
						, spine_hit_pairs[i].RV_FeetBalls_Hits[j]);
				}
					
				if (spine_hit_pairs[i].RV_Feet_Hits[j].bBlockingHit)
				{
					feet_mid_points[i] = spine_hit_pairs[i].RV_Feet_Hits[j].ImpactPoint;

				}
			}
		}


		/*
		if (i == 0)
		{
			FVector a = spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation();
			float b = Total_spine_heights[i];
			FVector c = spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation();


			line_trace_func(owning_skel, spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() + FVector(0, 0, Total_spine_heights[i] * line_trace_upper_height * scale_mode), spine_AnimatedTransform_pairs[i].Spine_Involved.GetLocation() - FVector(0, 0, line_trace_downward_height * scale_mode), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit);
		}
		else
		{
			FVector offseted_vector = spine_Transform_pairs[i].Spine_Involved.GetLocation() - spine_Transform_pairs[0].Spine_Involved.GetLocation();

			line_trace_func(owning_skel, spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() + offseted_vector + FVector(0, 0, line_trace_upper_height * scale_mode), spine_AnimatedTransform_pairs[0].Spine_Involved.GetLocation() + offseted_vector - FVector(0, 0, line_trace_downward_height * scale_mode), spine_hit_pairs[i].Parent_Spine_Hit, spine_Feet_pair[i].Spine_Involved.BoneName, spine_Feet_pair[i].Spine_Involved.BoneName, spine_hit_pairs[i].Parent_Spine_Hit);
		}
		*/

	}



}

//Nothing would be needed here
void FAnimNode_DragonFeetSolver::EvaluateComponentSpaceInternal(FComponentSpacePoseContext & Context)
{
}

/*
void FAnimNode_DragonFeetSolver::EvaluateBoneTransforms(USkeletalMeshComponent * SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
}
*/

void FAnimNode_DragonFeetSolver::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{



	if (spine_hit_pairs.Num() > 0)
	{

		atleast_one_hit = false;

		/*
		for (int k = 0; k < spine_hit_pairs.Num(); k++)
		{


			for (int i = 0; i < spine_hit_pairs[k].RV_Feet_Hits.Num(); i++)
			{
				if ((spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > -2.5f*owning_skel->GetComponentScale().Z)
				{
					atleast_one_hit = true;


				}

				//	FeetRootHeights			GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " spine size : " + FString::SanitizeFloat((spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z))));

			}
		}
		*/


		atleast_one_hit = true;

		for (int k = 0; k < spine_hit_pairs.Num(); k++)
		{
			for (int i = 0; i < spine_hit_pairs[k].RV_Feet_Hits.Num(); i++)
			{

/*				if (FMath::Abs(spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > Total_spine_heights[k] * 1.0f)
				{
					atleast_one_hit = false;
				}
*/

				if (FMath::Abs(spine_hit_pairs[k].RV_Feet_Hits[i].ImpactPoint.Z - (owning_skel->GetComponentLocation().Z)) > Vertical_Solver_downward_Scale)
				{
					atleast_one_hit = false;
				}



			}
		}
		//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " F EffectorLocation  :- " + feet_bone_array[i].BoneName.ToString()+"  = "+ FString::SanitizeFloat( feet_bone_array[i].BoneIndex));

		atleast_one_hit = true;


		//	for (int i = 0; i<feet_bone_array.Num(); i++)
		for (int i = 0; i < spine_hit_pairs.Num(); i++)
		{
			for (int j = 0; j < spine_hit_pairs[i].RV_Feet_Hits.Num(); j++)
			{
				
				Leg_Full_Function(spine_Feet_pair[i].Associated_Feet[j].BoneName, i, j, Output, OutBoneTransforms);
			}

		}


	}




}



void FAnimNode_DragonFeetSolver::Leg_Full_Function(FName foot_name, int spine_index, int feet_index, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms)
{


	FBoneReference ik_feet_bone = FBoneReference(foot_name);
	ik_feet_bone.Initialize(*SavedBoneContainer);

	FTransform bonetraceTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(ik_feet_bone.GetCompactPoseIndex(*SavedBoneContainer));



	FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(bonetraceTransform.GetLocation());


	if (ik_type == EIK_Type_Plugin::ENUM_Two_Bone_Ik)
		Leg_ik_Function(ik_feet_bone, spine_index, feet_index, EBoneControlSpace::BCS_WorldSpace, EBoneControlSpace::BCS_ComponentSpace, MeshBasesSaved, OutBoneTransforms);
	else
		Leg_Singleik_Function(ik_feet_bone, spine_index, feet_index, EBoneControlSpace::BCS_WorldSpace, EBoneControlSpace::BCS_ComponentSpace, MeshBasesSaved, OutBoneTransforms);

}






void FAnimNode_DragonFeetSolver::Leg_ik_Function(FBoneReference ik_footbone, int spine_index, int feet_index, TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, FComponentSpacePoseContext& MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms)
{

	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = ik_footbone.GetCompactPoseIndex(*SavedBoneContainer);


	if (automatic_leg_make == false)
	{
	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet : " + owning_skel->GetBoneName(spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex.GetInt()).ToString() + " = "+ owning_skel->GetBoneName(IKBoneCompactPoseIndex.GetInt()).ToString());

		IKBoneCompactPoseIndex = spine_Feet_pair[spine_index].Associated_Feet[feet_index].CachedCompactPoseIndex;
	}



	FCompactPoseBoneIndex LowerLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(IKBoneCompactPoseIndex);

	if (automatic_leg_make == false)
	{
		LowerLimbIndex = spine_Feet_pair[spine_index].Associated_Knees[feet_index].CachedCompactPoseIndex;

	//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Knee : " + owning_skel->GetBoneName(spine_Feet_pair[spine_index].Associated_Knees[feet_index].CachedCompactPoseIndex.GetInt()).ToString() + " = " + owning_skel->GetBoneName(LowerLimbIndex.GetInt()).ToString());

	}



	if (LowerLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}


	 FCompactPoseBoneIndex UpperLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(LowerLimbIndex);

	 if (automatic_leg_make == false)
	 {
		 UpperLimbIndex = spine_Feet_pair[spine_index].Associated_Thighs[feet_index].CachedCompactPoseIndex;

	//	 GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Thigh : " + owning_skel->GetBoneName(spine_Feet_pair[spine_index].Associated_Thighs[feet_index].CachedCompactPoseIndex.GetInt()).ToString() + " = " + owning_skel->GetBoneName(UpperLimbIndex.GetInt()).ToString());

	 }


	if (UpperLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}




	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (*SavedBoneContainer).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (*SavedBoneContainer).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));


	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if (bInvalidLimb)
	{
		return;
	}



	const FTransform EndBoneLocalTransform = MeshBasesSaved.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);

	// Now get those in component space...
	FTransform LowerLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


//	FTransform RootBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));

	FTransform RootBoneCSTransform = FTransform::Identity;


	float feet_root_height = 0;

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialJointPos = LowerLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();



	FVector EffectorLocation_Point = spine_AnimatedTransform_pairs[spine_index].Associated_Feet[feet_index].GetLocation();




	//if 


	//GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet Z : " + FString::SanitizeFloat(FMath::Abs(spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_Feet[feet_index].GetLocation().Z)))+" FeetBall Z : " + FString::SanitizeFloat(FMath::Abs(spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_FeetBalls[feet_index].GetLocation().Z))));


	//bool passing_maximum_height = (FMath::Abs(spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_Feet[feet_index].GetLocation().Z)) < Vertical_Solver_downward_Scale)  || (FMath::Abs(spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_FeetBalls[feet_index].GetLocation().Z)) < Vertical_Solver_downward_Scale);


	bool passing_maximum_height = (FMath::Abs(spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_Feet[feet_index].GetLocation().Z)) < Vertical_Solver_downward_Scale) || (FMath::Abs(spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].ImpactPoint.Z - (spine_AnimatedTransform_pairs[spine_index].Associated_Feet[feet_index].GetLocation().Z)) < Vertical_Solver_downward_Scale);


	if ( (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit || spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].bBlockingHit) && atleast_one_hit && enable_solver == true && passing_maximum_height)
	{


		FTransform eb_wtransform = EndBoneCSTransform;

		FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, eb_wtransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

		if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit)
		{
			EffectorLocation_Point.Z = (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint + FVector(0, 0, FeetRootHeights[spine_index][feet_index] * 1)).Z;


			if (every_foot_dont_have_child == false && Use_Feet_Tips)
			{
				if (spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].bBlockingHit && (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint.Z < spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].ImpactPoint.Z))
				{
					EffectorLocation_Point.Z = (spine_hit_pairs[spine_index].RV_FeetBalls_Hits[feet_index].ImpactPoint + FVector(0, 0, FeetRootHeights[spine_index][feet_index] * 1)).Z;
				}
			}

		}


//		feet_mod_transform_array[spine_index][feet_index].SetLocation(EffectorLocation_Point);


		//EffectorLocation_Point = spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint;


//		if ((eb_wtransform.GetLocation() - EffectorLocation_Point).Size() > 10000)
		feet_mod_transform_array[spine_index][feet_index].SetLocation(EffectorLocation_Point);
//		else
//			feet_mod_transform_array[spine_index][feet_index].SetLocation(UKismetMathLibrary::VInterpTo(feet_mod_transform_array[spine_index][feet_index].GetLocation(), EffectorLocation_Point, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()),Location_Lerp_Speed));
			

//		feet_mod_transform_array[spine_index][feet_index].SetLocation(EffectorLocation_Point);

		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo(feet_Alpha_array[spine_index][feet_index], 1, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()),shift_speed);


//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo_Constant(feet_Alpha_array[spine_index][feet_index], 1, owning_skel->GetWorld()->GetDeltaSeconds(), shift_speed* 25);

	}
	else
	{

		FTransform eb_wtransform = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, eb_wtransform, EffectorSpaceBoneIndex, EffectorLocationSpace);
		EffectorLocation_Point = eb_wtransform.GetLocation();


//		if ((eb_wtransform.GetLocation() - EffectorLocation_Point).Size() > 10000)
			feet_mod_transform_array[spine_index][feet_index].SetLocation(eb_wtransform.GetLocation());
//		else
//		    feet_mod_transform_array[spine_index][feet_index].SetLocation(UKismetMathLibrary::VInterpTo(feet_mod_transform_array[spine_index][feet_index].GetLocation(), eb_wtransform.GetLocation(), 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()),Location_Lerp_Speed));


//		feet_mod_transform_array[spine_index][feet_index].SetLocation(EffectorLocation_Point);

		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo(feet_Alpha_array[spine_index][feet_index], 0, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()), shift_speed);

//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo_Constant(feet_Alpha_array[spine_index][feet_index], 0, owning_skel->GetWorld()->GetDeltaSeconds(), shift_speed * 50);

	}



//	GEngine->AddOnScreenDebugMessage(-1, 0.1f, FColor::Red, " Feet Root Alpha : " + FString::SanitizeFloat(feet_Alpha_array[spine_index][feet_index]));


	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform(feet_mod_transform_array[spine_index][feet_index]);

	//	FTransform EffectorTransform(DebugEffectorTransform);

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, EffectorTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

	// This is our reach goal.
	FVector DesiredPos = EffectorTransform.GetTranslation();
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < (float)KINDA_SMALL_NUMBER)
	{
		DesiredLength = (float)KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta / DesiredLength;
	}



	// Get joint target (used for defining plane that joint should be in).
	//	FTransform JointTargetTransform(ik_footbone.joint_point);

	//The transform for the bending direction joint point
	FTransform BendingDirectionTransform = LowerLimbCSTransform;


	FVector cs_forward = owning_skel->GetComponentToWorld().InverseTransformVector(owning_skel->GetRightVector());

	cs_forward = (((UpperLimbCSTransform.GetLocation() + EndBoneCSTransform.GetLocation() + LowerLimbCSTransform.GetLocation()) / 3) - LowerLimbCSTransform.GetLocation()).GetSafeNormal();

	BendingDirectionTransform.SetLocation(BendingDirectionTransform.GetLocation() + cs_forward *-1000);


	FTransform JointTargetTransform(BendingDirectionTransform);


	FCompactPoseBoneIndex JointTargetSpaceBoneIndex(INDEX_NONE);


	FVector	JointTargetPos = JointTargetTransform.GetTranslation() + FVector(spine_Feet_pair[spine_index].feet_yaw_offset[feet_index],0,0);
	FVector JointTargetDelta = JointTargetPos - RootPos;
	const float JointTargetLengthSqr = JointTargetDelta.SizeSquared();

	// Same check as above, to cover case when JointTarget position is the same as RootPos.
	FVector JointPlaneNormal, JointBendDir;
	if (JointTargetLengthSqr < FMath::Square((float)KINDA_SMALL_NUMBER))
	{
		JointBendDir = FVector(0, 1, 0);
		JointPlaneNormal = FVector(0, 0, 1);
	}
	else
	{
		JointPlaneNormal = DesiredDir ^ JointTargetDelta;

		// If we are trying to point the limb in the same direction that we are supposed to displace the joint in, 
		// we have to just pick 2 random vector perp to DesiredDir and each other.
		if (JointPlaneNormal.SizeSquared() < FMath::Square((float)KINDA_SMALL_NUMBER))
		{
			DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
		}
		else
		{
			JointPlaneNormal.Normalize();

			// Find the final member of the reference frame by removing any component of JointTargetDelta along DesiredDir.
			// This should never leave a zero vector, because we've checked DesiredDir and JointTargetDelta are not parallel.
			JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
			JointBendDir.Normalize();
		}
	}


	float LowerLimbLength = (InitialEndPos - InitialJointPos).Size();
	float UpperLimbLength = (InitialJointPos - RootPos).Size();
	float MaxLimbLength = LowerLimbLength + UpperLimbLength;


	FVector OutEndPos = DesiredPos;
	FVector OutJointPos = InitialJointPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength > MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
		OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
	}
	else
	{
		// So we have a triangle we know the side lengths of. We can work out the angle between DesiredDir and the direction of the upper limb
		// using the sin rule:
		const float TwoAB = 2.f * UpperLimbLength * DesiredLength;

		const float CosAngle = (TwoAB != 0.f) ? ((UpperLimbLength*UpperLimbLength) + (DesiredLength*DesiredLength) - (LowerLimbLength*LowerLimbLength)) / TwoAB : 0.f;

		// If CosAngle is less than 0, the upper arm actually points the opposite way to DesiredDir, so we handle that.
		const bool bReverseUpperBone = (CosAngle < 0.f);

		// If CosAngle is greater than 1.f, the triangle could not be made - we cannot reach the target.
		// We just have the two limbs double back on themselves, and EndPos will not equal the desired EffectorLocation.
		if ((CosAngle > 1.f) || (CosAngle < -1.f))
		{
			// Because we want the effector to be a positive distance down DesiredDir, we go back by the smaller section.
			if (UpperLimbLength > LowerLimbLength)
			{
				OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
				OutEndPos = OutJointPos - (LowerLimbLength * DesiredDir);
			}
			else
			{
				OutJointPos = RootPos - (UpperLimbLength * DesiredDir);
				OutEndPos = OutJointPos + (LowerLimbLength * DesiredDir);
			}
		}
		else
		{
			// Angle between upper limb and DesiredDir
			const float Angle = FMath::Acos(CosAngle);

			// Now we calculate the distance of the joint from the root -> effector line.
			// This forms a right-angle triangle, with the upper limb as the hypotenuse.
			const float JointLineDist = UpperLimbLength * FMath::Sin(Angle);

			// And the final side of that triangle - distance along DesiredDir of perpendicular.
			// ProjJointDistSqr can't be neg, because JointLineDist must be <= UpperLimbLength because appSin(Angle) is <= 1.
			const float ProjJointDistSqr = (UpperLimbLength*UpperLimbLength) - (JointLineDist*JointLineDist);
			// although this shouldn't be ever negative, sometimes Xbox release produces -0.f, causing ProjJointDist to be NaN
			// so now I branch it. 						
			float ProjJointDist = (ProjJointDistSqr > 0.f) ? FMath::Sqrt(ProjJointDistSqr) : 0.f;
			if (bReverseUpperBone)
			{
				ProjJointDist *= -1.f;
			}

			// So now we can work out where to put the joint!
			OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
		}
	}


	// Update transform for upper bone.
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (InitialJointPos - RootPos).GetSafeNormal();
		FVector const NewDir = (OutJointPos - RootPos).GetSafeNormal();
		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation




		UpperLimbCSTransform.SetRotation(DeltaRotation * UpperLimbCSTransform.GetRotation());
		// And put joint where it should be.
		UpperLimbCSTransform.SetTranslation(RootPos);

		// Order important. First bone is upper limb.
	}

	// Update transform for lower bone.
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (InitialEndPos - InitialJointPos).GetSafeNormal();
		FVector const NewDir = (OutEndPos - OutJointPos).GetSafeNormal();

		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);





		LowerLimbCSTransform.SetRotation(DeltaRotation * LowerLimbCSTransform.GetRotation());
		// And put joint where it should be.
		LowerLimbCSTransform.SetTranslation(OutJointPos);



		// Order important. Second bone is lower limb.
	}

	// Update transform for end bone.




	{

		// Set correct location for end bone.
		EndBoneCSTransform.SetTranslation(OutEndPos);
		//+ impact_result.ImpactNormal.GetSafeNormal() 


		//		MeshBasesSaved.ResetToRefPose();

		if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit&& atleast_one_hit)
		{
			if (owning_skel->GetOwner() != nullptr)
			{
				FRotator input_rotation_axis;

				FRotator XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetForwardVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
				FRotator YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetRightVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
				FRotator bone_rotation = FRotator(owning_skel->GetComponentToWorld().GetRotation()*EndBoneCSTransform.GetRotation());

				if (owning_skel->GetOwner() != nullptr)
				{
					XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetOwner()->GetActorForwardVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
					YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetOwner()->GetActorRightVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
					bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
				}

				if (Should_Rotate_Feet == false)
				{
					XZ_rot.Roll = 0;
					YZ_rot.Pitch = 0;
				}


				input_rotation_axis = FRotator(YZ_rot.Pitch, bone_rotation.Yaw, XZ_rot.Roll);

				FRotator direction_rot = BoneRelativeConversion(spine_Feet_pair[spine_index].feet_yaw_offset[feet_index], spine_Feet_pair[spine_index].Associated_Feet[feet_index].GetCompactPoseIndex(*SavedBoneContainer), input_rotation_axis, *SavedBoneContainer, MeshBasesSaved.Pose);


//				feet_mod_transform_array[spine_index][feet_index].SetRotation(direction_rot.Quaternion());


				FRotator rot_result = feet_mod_transform_array[spine_index][feet_index].Rotator();



				FTransform FeetCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

				FTransform Feet_Saved_Transform = FTransform(feet_mod_transform_array[spine_index][feet_index].Rotator());



				if (owning_skel->GetWorld()->IsGameWorld()==false)
				{
					feet_mod_transform_array[spine_index][feet_index].SetRotation(FeetCSTransform.GetRotation());
				}
				else
				{

					if(Should_Rotate_Feet)
					EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());

				}



				//						EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());




			
		}


//					EndBoneCSTransform.SetRotation(UKismetMathLibrary::RInterpTo_Constant(EndBoneCSTransform.Rotator(), feet_mod_transform_array[spine_index][feet_index].Rotator(), feet_Alpha_array[spine_index][feet_index]));

				
			


		}
		else
		{


			FTransform LowerLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(LowerLimbIndex);
			FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
			FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


			//feet_mod_transform_array[spine_index][feet_index].SetRotation(UKismetMathLibrary::RInterpTo(feet_mod_transform_array[spine_index][feet_index].Rotator(), EndBoneCSTransformX.Rotator(),(1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds())),shift_speed).Quaternion());


//			feet_mod_transform_array[spine_index][feet_index].SetRotation(UKismetMathLibrary::RInterpTo(feet_mod_transform_array[spine_index][feet_index].Rotator(), EndBoneCSTransformX.Rotator(), (1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds())), shift_speed).Quaternion());




			if (owning_skel->GetWorld()->IsGameWorld() == false)
			{
				feet_mod_transform_array[spine_index][feet_index].SetRotation(EndBoneCSTransformX.GetRotation());
			}
			else
			feet_mod_transform_array[spine_index][feet_index].SetRotation(FQuat::Slerp(feet_mod_transform_array[spine_index][feet_index].GetRotation(), EndBoneCSTransformX.GetRotation(), (1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()))*shift_speed));


			




//			feet_mod_transform_array[spine_index][feet_index].SetRotation(EndBoneCSTransformX.GetRotation());

			EndBoneCSTransform = EndBoneCSTransformX;
			LowerLimbCSTransform = LowerLimbCSTransformX;

			UpperLimbCSTransform = UpperLimbCSTransformX;

			if(Should_Rotate_Feet)
			EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());








		}




	}



	FTransform LowerLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(LowerLimbIndex);
	FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

	EndBoneCSTransformX.SetRotation(EndBoneCSTransform.GetRotation());



//	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, feet_Alpha_array[spine_index][feet_index]);
//	FTransform Lerped_LowerLimbCSTransform = UKismetMathLibrary::TLerp(LowerLimbCSTransformX, LowerLimbCSTransform, feet_Alpha_array[spine_index][feet_index]);
//	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, feet_Alpha_array[spine_index][feet_index]);




	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, feet_Alpha_array[spine_index][feet_index]);

//	if( FMath::Abs<float>( (EndBoneCSTransformX.GetLocation() - EndBoneCSTransform.GetLocation()).Size())>1000  )
	{
//		Lerped_EndBoneCSTransform = EndBoneCSTransform;
	}


	FTransform Lerped_LowerLimbCSTransform = UKismetMathLibrary::TLerp(LowerLimbCSTransformX, LowerLimbCSTransform, feet_Alpha_array[spine_index][feet_index]);

//	if (FMath::Abs<float>((LowerLimbCSTransformX.GetLocation() - LowerLimbCSTransform.GetLocation()).Size())>1000)
	{
//		Lerped_LowerLimbCSTransform = LowerLimbCSTransform;
	}


	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, feet_Alpha_array[spine_index][feet_index]);

//	if (FMath::Abs<float>((UpperLimbCSTransformX.GetLocation() - UpperLimbCSTransform.GetLocation()).Size())>1000)
	{
//		Lerped_UpperLimbCSTransform = UpperLimbCSTransform;
	}

//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet 1 : " + owning_skel->GetBoneName(UpperLimbIndex.GetInt() ).ToString());
//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet 2 : " + owning_skel->GetBoneName(LowerLimbIndex.GetInt()).ToString());
//	GEngine->AddOnScreenDebugMessage(-1, 0.01f, FColor::Red, " Feet 3 : " + owning_skel->GetBoneName(IKBoneCompactPoseIndex.GetInt()).ToString());



	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));

	OutBoneTransforms.Add(FBoneTransform(LowerLimbIndex, Lerped_LowerLimbCSTransform));

	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));

/*
	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));

	OutBoneTransforms.Add(FBoneTransform(LowerLimbIndex, Lerped_LowerLimbCSTransform));

	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));
	*/

}










void FAnimNode_DragonFeetSolver::Leg_Singleik_Function(FBoneReference ik_footbone, int spine_index, int feet_index, TEnumAsByte<enum EBoneControlSpace> EffectorLocationSpace, TEnumAsByte<enum EBoneControlSpace> JointTargetLocationSpace, FComponentSpacePoseContext & MeshBasesSaved, TArray<FBoneTransform>& OutBoneTransforms)
{





	// Get indices of the lower and upper limb bones and check validity.
	bool bInvalidLimb = false;

	FCompactPoseBoneIndex IKBoneCompactPoseIndex = ik_footbone.GetCompactPoseIndex(*SavedBoneContainer);


	const FCompactPoseBoneIndex UpperLimbIndex = (*SavedBoneContainer).GetParentBoneIndex(IKBoneCompactPoseIndex);
	if (UpperLimbIndex == INDEX_NONE)
	{
		bInvalidLimb = true;
	}


	const bool bInBoneSpace = (EffectorLocationSpace == BCS_ParentBoneSpace) || (EffectorLocationSpace == BCS_BoneSpace);
	const int32 EffectorBoneIndex = bInBoneSpace ? (*SavedBoneContainer).GetPoseBoneIndexForBoneName("") : INDEX_NONE;
	const FCompactPoseBoneIndex EffectorSpaceBoneIndex = (*SavedBoneContainer).MakeCompactPoseIndex(FMeshPoseBoneIndex(EffectorBoneIndex));


	// If we walked past the root, this controlled is invalid, so return no affected bones.
	if (bInvalidLimb)
	{
		return;
	}



	const FTransform EndBoneLocalTransform = MeshBasesSaved.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);

	// Now get those in component space...
	FTransform UpperLimbCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


	FTransform RootBoneCSTransform = MeshBasesSaved.Pose.GetComponentSpaceTransform(FCompactPoseBoneIndex(0));



	float feet_root_height = 0;

	// Get current position of root of limb.
	// All position are in Component space.
	const FVector RootPos = UpperLimbCSTransform.GetTranslation();
	const FVector InitialEndPos = EndBoneCSTransform.GetTranslation();



	FVector EffectorLocation_Point;


	if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit && atleast_one_hit)
	{


		FTransform eb_wtransform = EndBoneCSTransform;

		FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, eb_wtransform, EffectorSpaceBoneIndex, EffectorLocationSpace);
		EffectorLocation_Point = spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactPoint + FVector(0, 0, FeetRootHeights[spine_index][feet_index] * 1);

		feet_mod_transform_array[spine_index][feet_index].SetLocation(EffectorLocation_Point);

//		feet_Alpha_array[spine_index][feet_index] = 1;

//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo(feet_Alpha_array[spine_index][feet_index], 1, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()), shift_speed);

		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo_Constant(feet_Alpha_array[spine_index][feet_index], 1, owning_skel->GetWorld()->GetDeltaSeconds(), shift_speed * 25);


//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::Lerp(feet_Alpha_array[spine_index][feet_index], 1, owning_skel->GetWorld()->DeltaTimeSeconds * 5);

	}
	else
	{

		FTransform eb_wtransform = EndBoneCSTransform;
		FAnimationRuntime::ConvertCSTransformToBoneSpace(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, eb_wtransform, EffectorSpaceBoneIndex, EffectorLocationSpace);
		EffectorLocation_Point = eb_wtransform.GetLocation();

		feet_mod_transform_array[spine_index][feet_index].SetLocation(eb_wtransform.GetLocation());


//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::Lerp(feet_Alpha_array[spine_index][feet_index], 0, owning_skel->GetWorld()->DeltaTimeSeconds * 5);


//		feet_Alpha_array[spine_index][feet_index] = 0;

//		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo(feet_Alpha_array[spine_index][feet_index], 0, 1 - FMath::Exp(-10 * owning_skel->GetWorld()->GetDeltaSeconds()), shift_speed);

		feet_Alpha_array[spine_index][feet_index] = UKismetMathLibrary::FInterpTo_Constant(feet_Alpha_array[spine_index][feet_index], 0, owning_skel->GetWorld()->GetDeltaSeconds(), shift_speed * 25);

	}



	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform EffectorTransform(feet_mod_transform_array[spine_index][feet_index]);

	//	FTransform EffectorTransform(DebugEffectorTransform);

	FAnimationRuntime::ConvertBoneSpaceTransformToCS(owning_skel->GetComponentTransform(), MeshBasesSaved.Pose, EffectorTransform, EffectorSpaceBoneIndex, EffectorLocationSpace);

	// This is our reach goal.
	FVector DesiredPos = EffectorTransform.GetTranslation();
	FVector DesiredDelta = DesiredPos - RootPos;
	float DesiredLength = DesiredDelta.Size();

	// Check to handle case where DesiredPos is the same as RootPos.
	FVector	DesiredDir;
	if (DesiredLength < (float)KINDA_SMALL_NUMBER)
	{
		DesiredLength = (float)KINDA_SMALL_NUMBER;
		DesiredDir = FVector(1, 0, 0);
	}
	else
	{
		DesiredDir = DesiredDelta / DesiredLength;
	}













	float UpperLimbLength = (InitialEndPos - RootPos).Size();
	float MaxLimbLength = UpperLimbLength;


	FVector OutEndPos = DesiredPos;

	// If we are trying to reach a goal beyond the length of the limb, clamp it to something solvable and extend limb fully.
	if (DesiredLength > MaxLimbLength)
	{
		OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
	}
	else
	{

		OutEndPos = EffectorTransform.GetLocation();

	}


	// Update transform for upper bone.
	{
		// Get difference in direction for old and new joint orientations
		FVector const OldDir = (InitialEndPos - RootPos).GetSafeNormal();
		FVector const NewDir = (OutEndPos - RootPos).GetSafeNormal();
		// Find Delta Rotation take takes us from Old to New dir
		FQuat const DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
		// Rotate our Joint quaternion by this delta rotation




		UpperLimbCSTransform.SetRotation(DeltaRotation * UpperLimbCSTransform.GetRotation());
		// And put joint where it should be.
		UpperLimbCSTransform.SetTranslation(RootPos);

		// Order important. First bone is upper limb.
	}




	{

		EndBoneCSTransform.SetTranslation(OutEndPos);

		if (spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].bBlockingHit&& atleast_one_hit)
		{


			if (owning_skel->GetOwner() != nullptr)
			{
				FRotator input_rotation_axis;

				FRotator XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetForwardVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
				FRotator YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetRightVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
				FRotator bone_rotation = FRotator(owning_skel->GetComponentTransform().GetRotation()*EndBoneCSTransform.GetRotation());


				if (owning_skel->GetOwner() != nullptr)
				{
					XZ_rot = UKismetMathLibrary::MakeRotFromXZ(owning_skel->GetOwner()->GetActorForwardVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
					YZ_rot = UKismetMathLibrary::MakeRotFromYZ(owning_skel->GetOwner()->GetActorRightVector(), spine_hit_pairs[spine_index].RV_Feet_Hits[feet_index].ImpactNormal);
					bone_rotation = FRotator(owning_skel->GetOwner()->GetActorRotation());
				}


				input_rotation_axis = FRotator(YZ_rot.Pitch, bone_rotation.Yaw, XZ_rot.Roll);

				EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());
			}


		}
		else
		{


			FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
			FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);


//			feet_mod_transform_array[spine_index][feet_index].SetRotation(UKismetMathLibrary::RLerp(feet_mod_transform_array[spine_index][feet_index].Rotator(), EndBoneCSTransform.Rotator(), owning_skel->GetWorld()->GetDeltaSeconds() * 15, true).Quaternion());


			feet_mod_transform_array[spine_index][feet_index].SetRotation(EndBoneCSTransform.Rotator().Quaternion());


			EndBoneCSTransform = EndBoneCSTransformX;

			UpperLimbCSTransform = UpperLimbCSTransformX;

			EndBoneCSTransform.SetRotation(feet_mod_transform_array[spine_index][feet_index].GetRotation());








		}




	}



	FTransform UpperLimbCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(UpperLimbIndex);
	FTransform EndBoneCSTransformX = MeshBasesSaved.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);



	FTransform Lerped_EndBoneCSTransform = UKismetMathLibrary::TLerp(EndBoneCSTransformX, EndBoneCSTransform, feet_Alpha_array[spine_index][feet_index]);
	FTransform Lerped_UpperLimbCSTransform = UKismetMathLibrary::TLerp(UpperLimbCSTransformX, UpperLimbCSTransform, feet_Alpha_array[spine_index][feet_index]);


	OutBoneTransforms.Add(FBoneTransform(UpperLimbIndex, Lerped_UpperLimbCSTransform));

	OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, Lerped_EndBoneCSTransform));








}






void FAnimNode_DragonFeetSolver::LineTraceControl_AnyThread(FComponentSpacePoseContext & Output, TArray<FBoneTransform>& OutBoneTransforms)
{

	if (spine_hit_pairs.Num() > 0 && spine_Feet_pair.Num() > 0 && spine_Transform_pairs.Num()>0 )
	{

		for (int32 i = 0; i < spine_Feet_pair.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex_Local_i = spine_Feet_pair[i].Spine_Involved.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform_Local_i;
			ComponentBoneTransform_Local_i = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_i);
			FVector lerp_data_Local_i = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_i.GetLocation());

			spine_Transform_pairs[i].Spine_Involved = ComponentBoneTransform_Local_i* owning_skel->GetComponentTransform();

			//spine_Transform_pairs[i].Spine_Involved = FTransform(lerp_data_Local_i);

			

			for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
			{
				const FCompactPoseBoneIndex ModifyBoneIndex_Local_j = spine_Feet_pair[i].Associated_Feet[j].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
				FTransform ComponentBoneTransform_Local_j;
				ComponentBoneTransform_Local_j = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex_Local_j);
				FVector lerp_data_Local_j = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform_Local_j.GetLocation());
//				spine_Transform_pairs[i].Associated_Feet[j] = FTransform(lerp_data_Local_j);

				spine_Transform_pairs[i].Associated_Feet[j] = ComponentBoneTransform_Local_j * owning_skel->GetComponentTransform();


			}
		}
	}



	if (feet_transform_array.Num() > 0)
	{

		for (int32 i = 0; i < feet_bone_array.Num(); i++)
		{
			const FCompactPoseBoneIndex ModifyBoneIndex = feet_bone_array[i].GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
			FTransform ComponentBoneTransform;
			ComponentBoneTransform = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex);
			FVector lerp_data = owning_skel->GetComponentTransform().TransformPosition(ComponentBoneTransform.GetLocation());
			feet_transform_array[i] = FTransform(lerp_data);

		}


	}
}



bool FAnimNode_DragonFeetSolver::IsValidToEvaluate(const USkeleton * Skeleton, const FBoneContainer & RequiredBones)
{

	bool feet_is_true = true;

	for (int i = 0; i < dragon_bone_data.FeetBones.Num(); i++)
	{
//		if (dragon_bone_data.FeetBones.Num() == dragon_input_data.FeetBones.Num())
			if (FBoneReference(dragon_bone_data.FeetBones[i]).IsValidToEvaluate(RequiredBones) == false)
			{
				feet_is_true = false;
			}


			if (automatic_leg_make == false)
			{


				if (FBoneReference(dragon_bone_data.KneeBones[i]).IsValidToEvaluate(RequiredBones) == false)
				{
					feet_is_true = false;
				}

				if (FBoneReference(dragon_bone_data.ThighBones[i]).IsValidToEvaluate(RequiredBones) == false)
				{
					feet_is_true = false;
				}


			}

//			if (dragon_bone_data.FeetBones.Num() == dragon_input_data.FeetBones.Num())

	}

//	return false;

	//return (feet_is_true &&dragon_bone_data.Start_Spine.IsValidToEvaluate(RequiredBones) &&dragon_bone_data.Pelvis.IsValidToEvaluate(RequiredBones));
	
	//return false;

	return (solve_should_fail == false && feet_is_true &&
		dragon_bone_data.Start_Spine.IsValidToEvaluate(RequiredBones) &&
		dragon_bone_data.Pelvis.IsValidToEvaluate(RequiredBones) &&
		RequiredBones.BoneIsChildOf(FBoneReference(dragon_bone_data.Start_Spine).BoneIndex, FBoneReference(dragon_bone_data.Pelvis).BoneIndex));


}


void FAnimNode_DragonFeetSolver::Evaluate_AnyThread(FPoseContext & Output)
{
}


void FAnimNode_DragonFeetSolver::InitializeBoneReferences(FBoneContainer & RequiredBones)
{

	SavedBoneContainer = &RequiredBones;

	feet_bone_array.Empty();

	feet_transform_array.Empty();

	feet_hit_array.Empty();

	EffectorLocationList.Empty();

	feet_mod_transform_array.Empty();

	feet_Alpha_array.Empty();

	FeetRootHeights.Empty();



	/*SPINE CODE START*/

	Total_spine_bones.Empty();

	spine_Feet_pair.Empty();

	spine_hit_pairs.Empty();

	spine_Transform_pairs.Empty();

	spine_AnimatedTransform_pairs.Empty();



	solve_should_fail = false;



	Total_spine_bones = BoneArrayMachine(0, dragon_input_data.Start_Spine, dragon_input_data.Pelvis, false);

	Algo::Reverse(Total_spine_bones);


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


		BoneArrayMachine_Feet(i, dragon_input_data.FeetBones[i].Feet_Bone_Name, dragon_input_data.FeetBones[i].Knee_Bone_Name, dragon_input_data.FeetBones[i].Thigh_Bone_Name, dragon_input_data.Pelvis, true);
	}


	Spine_Indices.Empty();


	for (int32 i = 0; i < Total_spine_bones.Num(); i++)
	{
		FBoneReference bone_ref = FBoneReference(Total_spine_bones[i]);
		bone_ref.Initialize(RequiredBones);
		Spine_Indices.Add(bone_ref.GetCompactPoseIndex(RequiredBones));

	}



	for (int32 i = 0; i < spine_Feet_pair.Num(); i++)
	{
		if (spine_Feet_pair[i].Associated_Feet.Num() == 0)
			spine_Feet_pair.RemoveAt(i, 1, true);

	}
	spine_Feet_pair = Swap_Spine_Pairs(spine_Feet_pair);


	spine_Feet_pair = Swap_Spine_Pairs(spine_Feet_pair);










	spine_hit_pairs.AddDefaulted(spine_Feet_pair.Num());
	spine_Transform_pairs.AddDefaulted(spine_Feet_pair.Num());

	spine_AnimatedTransform_pairs.AddDefaulted(spine_Feet_pair.Num());

	feet_mod_transform_array.AddDefaulted(spine_Feet_pair.Num());

	FeetRootHeights.AddDefaulted(spine_Feet_pair.Num());

	feet_Alpha_array.AddDefaulted(spine_Feet_pair.Num());

	for (int32 i = 0; i < spine_Feet_pair.Num(); i++)
	{

		


		spine_hit_pairs[i].RV_Feet_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
		spine_hit_pairs[i].RV_FeetFront_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
		spine_hit_pairs[i].RV_FeetBack_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
		spine_hit_pairs[i].RV_FeetLeft_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
		spine_hit_pairs[i].RV_FeetRight_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());




		spine_hit_pairs[i].RV_Knee_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

		spine_hit_pairs[i].RV_FeetBalls_Hits.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());




		spine_Transform_pairs[i].Associated_Feet.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());
		spine_AnimatedTransform_pairs[i].Associated_Feet.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

		spine_AnimatedTransform_pairs[i].Associated_Knee.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

		spine_AnimatedTransform_pairs[i].Associated_FeetBalls.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

		spine_AnimatedTransform_pairs[i].Associated_FeetBalls.AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());



		feet_mod_transform_array[i].AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());

		feet_Alpha_array[i].AddDefaulted(spine_Feet_pair[i].Associated_Feet.Num());



		spine_Feet_pair[i].Associated_Feet_Tips.AddUninitialized(spine_Feet_pair[i].Associated_Feet.Num());

//		spine_Feet_pair[i].Associated_Knees.AddUninitialized(spine_Feet_pair[i].Associated_Feet.Num());

//		spine_Feet_pair[i].Associated_Thighs.AddUninitialized(spine_Feet_pair[i].Associated_Feet.Num());



		every_foot_dont_have_child = false;


		for (int32 j = 0; j < spine_Feet_pair[i].Associated_Feet.Num(); j++)
		{
			FeetRootHeights[i].Add(0);
	//		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, " Feet Root Heights : " + FString::SanitizeFloat(FeetRootHeights[i][j]));







			FName child_name = GetChildBone(spine_Feet_pair[i].Associated_Feet[j].BoneName, owning_skel);

			if (owning_skel->BoneIsChildOf(child_name, spine_Feet_pair[i].Associated_Feet[j].BoneName))
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

			/*
			if (automatic_leg_make == true)
			{
				FBoneReference Knee_Involved = FBoneReference(owning_skel->GetParentBone(spine_Feet_pair[i].Associated_Feet[j].BoneName));
				Knee_Involved.Initialize(*SavedBoneContainer);

				spine_Feet_pair[i].Associated_Knees[j] = Knee_Involved;
			}
			else
			{

				FBoneReference Knee_Involved = FBoneReference(owning_skel->GetParentBone(spine_Feet_pair[i].Associated_Feet[j].BoneName));
				Knee_Involved.Initialize(*SavedBoneContainer);
				spine_Feet_pair[i].Associated_Knees[j] = Knee_Involved;

			}
			*/

		}

	}


	/*SPINE CODE END*/



	for (int i = 0; i<dragon_input_data.FeetBones.Num(); i++)
	{
		feet_bone_array.Add(dragon_input_data.FeetBones[i].Feet_Bone_Name);
		feet_bone_array[i].Initialize(RequiredBones);

	}





	bool is_swapped = false;

	do
	{
		is_swapped = false;


		for (int32 i = 1; i < feet_bone_array.Num(); i++)
		{
			if (feet_bone_array[i - 1].BoneIndex < feet_bone_array[i].BoneIndex)
			{
				FBoneReference temp = feet_bone_array[i - 1];
				feet_bone_array[i - 1] = feet_bone_array[i];
				feet_bone_array[i] = temp;

				is_swapped = true;
			}
		}

	} while (is_swapped == true);





	feet_transform_array.AddDefaulted(feet_bone_array.Num());

	feet_Alpha_array.AddDefaulted(feet_bone_array.Num());



	feet_hit_array.AddDefaulted(feet_bone_array.Num());

	EffectorLocationList.AddDefaulted(feet_bone_array.Num());

	//	feet_mod_transform_array.AddDefaulted(feet_bone_array.Num());


	//	GEngine->AddOnScreenDebugMessage(-1, 10, FColor::Red, " F Transform size :- " + FString::SanitizeFloat(feet_transform_array.Num()));





	if (dragon_input_data.Start_Spine == dragon_input_data.Pelvis)
		solve_should_fail = true;

	if (spine_Feet_pair.Num() > 0)
	{
		if (spine_Feet_pair[0].Spine_Involved.BoneIndex > spine_Feet_pair[spine_Feet_pair.Num() - 1].Spine_Involved.BoneIndex)
			solve_should_fail = true;
	}



	dragon_bone_data.Start_Spine = FBoneReference(dragon_input_data.Start_Spine);
	dragon_bone_data.Start_Spine.Initialize(RequiredBones);

	dragon_bone_data.Pelvis = FBoneReference(dragon_input_data.Pelvis);
	dragon_bone_data.Pelvis.Initialize(RequiredBones);


	dragon_bone_data.FeetBones.Empty();
	dragon_bone_data.KneeBones.Empty();
	dragon_bone_data.ThighBones.Empty();


	for (int i = 0; i < dragon_input_data.FeetBones.Num(); i++)
	{

		dragon_bone_data.FeetBones.Add(FBoneReference(dragon_input_data.FeetBones[i].Feet_Bone_Name));
		dragon_bone_data.FeetBones[i].Initialize(RequiredBones);

		if (automatic_leg_make == false)
		{
			dragon_bone_data.KneeBones.Add(FBoneReference(dragon_input_data.FeetBones[i].Knee_Bone_Name));
			dragon_bone_data.KneeBones[i].Initialize(RequiredBones);


			dragon_bone_data.ThighBones.Add(FBoneReference(dragon_input_data.FeetBones[i].Thigh_Bone_Name));
			dragon_bone_data.ThighBones[i].Initialize(RequiredBones);
		}


	}






}






FName FAnimNode_DragonFeetSolver::GetChildBone(FName BoneName, USkeletalMeshComponent* skel_mesh)
{

	FName child_bone = skel_mesh->GetBoneName(skel_mesh->GetBoneIndex(BoneName) + 1);

	return child_bone;

}



/*SPINE FUNCS*/





TArray<FDragonData_SpineFeetPair> FAnimNode_DragonFeetSolver::Swap_Spine_Pairs(TArray<FDragonData_SpineFeetPair> test_list)
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

					if (automatic_leg_make == false)
					{

						FBoneReference tempKnees = test_list[j].Associated_Knees[i - 1];
						test_list[j].Associated_Knees[i - 1] = test_list[j].Associated_Knees[i];
						test_list[j].Associated_Knees[i] = tempKnees;


						FBoneReference tempThighs = test_list[j].Associated_Thighs[i - 1];
						test_list[j].Associated_Thighs[i - 1] = test_list[j].Associated_Thighs[i];
						test_list[j].Associated_Thighs[i] = tempThighs;
					}


					is_swapped = true;
				}
			}
		}

	} while (is_swapped == true);


	return test_list;

}



/*
TArray<FName> FAnimNode_DragonFeetSolver::BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot)
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
			if (Check_Loop_Exist(dragon_input_data.FeetBones[index].Feet_Yaw_Offset, dragon_input_data.FeetBones[index].Feet_Heights, starting,FName(""),FName(""), spine_bones[spine_bones.Num() - 1], Total_spine_bones))
			{
				return spine_bones;
			}
		}

		if (is_foot == false)
		{
			//			GEngine->AddOnScreenDebugMessage(-1, 5.01f, FColor::Red, "Spine INIT == " + spine_bones[iteration_count].ToString());
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


		if (spine_bones[spine_bones.Num() - 1] == ending  && is_foot == false)
		{
			//			GEngine->AddOnScreenDebugMessage(-1, 5.01f, FColor::Red, "Spine INIT == " + spine_bones[spine_bones.Num() - 1].ToString()+" and Ending = "+ending.ToString());


			return spine_bones;
		}

	} while (iteration_count < 50 && finish == false);


	return spine_bones;


}

*/







TArray<FName> FAnimNode_DragonFeetSolver::BoneArrayMachine_Feet(int32 index, FName starting,FName knee,FName thigh, FName ending, bool is_foot)
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
			if (Check_Loop_Exist(dragon_input_data.FeetBones[index].Feet_Yaw_Offset, dragon_input_data.FeetBones[index].Feet_Heights, starting,knee,thigh, spine_bones[spine_bones.Num() - 1], Total_spine_bones))
			{
				return spine_bones;
			}
		}

		if (is_foot == false)
		{
			//			GEngine->AddOnScreenDebugMessage(-1, 5.01f, FColor::Red, "Spine INIT == " + spine_bones[iteration_count].ToString());
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







TArray<FName> FAnimNode_DragonFeetSolver::BoneArrayMachine(int32 index, FName starting, FName ending, bool is_foot)
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
			if (Check_Loop_Exist(dragon_input_data.FeetBones[index].Feet_Yaw_Offset, dragon_input_data.FeetBones[index].Feet_Heights, starting,FName(""),FName(""), spine_bones[spine_bones.Num() - 1], Total_spine_bones))
			{
				return spine_bones;
			}
		}

		if (is_foot == false)
		{
			//			GEngine->AddOnScreenDebugMessage(-1, 5.01f, FColor::Red, "Spine INIT == " + spine_bones[iteration_count].ToString());
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







bool FAnimNode_DragonFeetSolver::Check_Loop_Exist(float yaw_offset,float feet_height, FName start_bone,FName knee_bone,FName thigh_bone, FName input_bone, TArray<FName> total_spine_bones)
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
				spine_Feet_pair[i].feet_yaw_offset.Add(yaw_offset);
				spine_Feet_pair[i].Feet_Heights.Add(feet_height);


				if (automatic_leg_make == false)
				{

					FBoneReference knee_bone_ref = FBoneReference(knee_bone);
					knee_bone_ref.Initialize(*SavedBoneContainer);
					spine_Feet_pair[i].Associated_Knees.Add(knee_bone_ref);


					FBoneReference thigh_bone_ref = FBoneReference(thigh_bone);
					thigh_bone_ref.Initialize(*SavedBoneContainer);
					spine_Feet_pair[i].Associated_Thighs.Add(thigh_bone_ref);





				}



				if (i < 0)
				{
					//		FBoneReference bone_edit = FBoneReference(owning_skel->GetParentBone(spine_Feet_pair[i - 1].Spine_Involved.BoneName));
					//		bone_edit.Initialize(*SavedBoneContainer);
					//	spine_Feet_pair[i - 1].Spine_Involved = bone_edit;
				}

				return true;
			}
		}
	}

	return false;
}




/*SPINE FUNCS END*/






FCollisionQueryParams FAnimNode_DragonFeetSolver::getDefaultColliParams(FName name, AActor *me)
{

	const FName TraceTag(name);

	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), true, me);
	RV_TraceParams.bTraceComplex = true;
	RV_TraceParams.bTraceAsyncScene = true;
	RV_TraceParams.bReturnPhysicalMaterial = false;
	RV_TraceParams.TraceTag = TraceTag;

	//	me->GetWorld()->DebugDrawTraceTag = TraceTag;


	return RV_TraceParams;
}


void FAnimNode_DragonFeetSolver::line_trace_func(USkeletalMeshComponent *skelmesh, FVector startpoint, FVector endpoint, FHitResult RV_Ragdoll_Hit, FName bone_text, FName trace_tag, FHitResult& Output, const FLinearColor& Fdebug_color)
{

	/*
	skelmesh->GetWorld()->LineTraceSingleByChannel(RV_Ragdoll_Hit,        //result
	startpoint,    //start
	endpoint, //end
	ECC_WorldStatic, //collision channel
	getDefaultParams(trace_tag, skelmesh->GetOwner()));

	TArray<AActor*> selfActor;
	//	selfActor.Add(skelmesh->GetOwner());
	*/


	/*BOX
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


	if(show_trace_in_game)
	UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::ForOneFrame, RV_Ragdoll_Hit, true,Fdebug_color);
	else
		UKismetSystemLibrary::LineTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, Trace_Channel, true, ignoreActors, EDrawDebugTrace::None, RV_Ragdoll_Hit, true, Fdebug_color);


	//BoxTraceSingle(UObject* WorldContextObject, const FVector Start, const FVector End, const FVector HalfSize, const FRotator Orientation, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)
	//SphereTraceSingle(UObject* WorldContextObject, const FVector Start, const FVector End, float Radius, ETraceTypeQuery TraceChannel, bool bTraceComplex, const TArray<AActor*>& ActorsToIgnore, EDrawDebugTrace::Type DrawDebugType, FHitResult& OutHit, bool bIgnoreSelf, FLinearColor TraceColor, FLinearColor TraceHitColor, float DrawTime)



	//UKismetSystemLibrary::BoxTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint, FVector(15,15,15), FRotator(0,0,0), Trace_Channel, true, ignoreActors, EDrawDebugTrace::ForOneFrame, RV_Ragdoll_Hit, true, FLinearColor::White, FLinearColor::Black, 0.01f);


	//UKismetSystemLibrary::SphereTraceSingle(owning_skel->GetOwner()->GetWorld(), startpoint, endpoint,25, Trace_Channel, true, ignoreActors, EDrawDebugTrace::ForOneFrame, RV_Ragdoll_Hit, true, FLinearColor::White, FLinearColor::Black, 0.01f);

	/*
	skelmesh->GetWorld()->LineTraceSingleByChannel(RV_Ragdoll_Hit,        //result
	startpoint,    //start
	endpoint, //end
	ECC_WorldStatic, //collision channel
	getDefaultParams(trace_tag, skelmesh->GetOwner()));
	*/

	Output = RV_Ragdoll_Hit;

}




FRotator FAnimNode_DragonFeetSolver::BoneRelativeConversion(float feet_data, FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases)
{


	//FCompactPoseBoneIndex CompactPoseBoneToModify = BoneToModify.GetCompactPoseIndex(BoneContainer);
	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	//	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
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


		//		FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, MeshBases, EndBoneCSTransform, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);

		//		make_rot.Yaw = EndBoneCSTransform.Rotator().Yaw;
		make_rot.Yaw = FRotator(actor_rotation_world).Yaw + feet_data;

		//		make_rot.Yaw = 0;

		//New Mod
		//make_rot.Yaw = NewBoneTM.Rotator().Yaw;

		NewBoneTM.SetRotation(make_rot.Quaternion());

	}

	const FQuat BoneQuat(target_rotation);

	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());

	// Convert back to Component Space.
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);


	FTransform EndBoneCSTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

	//	NewBoneTM.SetRotation(FRotator(NewBoneTM.Rotator().Pitch, EndBoneCSTransform.Rotator().Yaw, NewBoneTM.Rotator().Roll).Quaternion());

	return NewBoneTM.Rotator();


}

FRotator FAnimNode_DragonFeetSolver::BoneInverseConversion(FCompactPoseBoneIndex ModifyBoneIndex, FRotator target_rotation, const FBoneContainer & BoneContainer, FCSPose<FCompactPose>& MeshBases)
{

	FTransform NewBoneTM = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);
	//	FTransform ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
	FTransform ComponentTransform = owning_skel->GetComponentTransform();
	const FTransform C_ComponentTransform = owning_skel->GetComponentTransform();


	// Convert to Bone Space.ConvertBoneSpaceTransformToCS
	FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);
	float temp_yaw = ComponentTransform.Rotator().Yaw;
	FQuat actor_rotation_world(0, 0, 0, 0);

	if (owning_skel->GetOwner() != nullptr)
	{
		actor_rotation_world = owning_skel->GetOwner()->GetActorRotation().Quaternion()*FRotator(NewBoneTM.GetRotation()).Quaternion();
		FRotator make_rot = NewBoneTM.Rotator();
		make_rot.Yaw = FRotator(actor_rotation_world).Yaw;
		NewBoneTM.SetRotation(make_rot.Quaternion());
	}
	const FQuat BoneQuat(target_rotation);
	NewBoneTM.SetRotation(BoneQuat * NewBoneTM.GetRotation());
	// Convert back to Component Space.
	FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, MeshBases, NewBoneTM, ModifyBoneIndex, EBoneControlSpace::BCS_WorldSpace);
	return NewBoneTM.Rotator();

}





FVector FAnimNode_DragonFeetSolver::GetCurrentLocation(FCSPose<FCompactPose>& MeshBases, const FCompactPoseBoneIndex& BoneIndex)
{
	return MeshBases.GetComponentSpaceTransform(BoneIndex).GetLocation();
}


