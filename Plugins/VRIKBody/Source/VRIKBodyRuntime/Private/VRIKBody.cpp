// VR IK Body Plugin
// (c) Yuri N Kalinin, 2017, ykasczc@gmail.com. All right reserved.

#include "VRIKBody.h"
// ENGINE
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Features/IModularFeatures.h"
#include "IMotionController.h"
#include "MotionControllerComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine.h"
// PROJECT
#include "VRIKBodyPrivatePCH.h"
#include "VRIKBodyMath.h"

// Math statistics functions
using namespace VRIKBodyMath;

#define DELTA_DEG(a, b) FMath::RadiansToDegrees(FMath::ACos(FVector::DotProduct(a, b)))
#define CHECK_ROT_ROLL(rot) if (FMath::Abs(rot.Roll) > 135.f) { rot = FRotator(-90.f, rot.Yaw, 0.f); }
#define PACK_NT_TRANSFORM(ntt, t) ntt.Location = t.GetTranslation(); ntt.Rotation = t.GetRotation()
#define UNPACK_NT_TRANSFORM(ntt, t) t.SetTranslation(ntt.Location); t.SetRotation(ntt.Rotation);
#define LOCALLY_CONTROLLED (IsValid(OwningPawn->GetController()) && OwningPawn->GetController()->IsLocalController())
#define CHECKORIENTATION(O) SkeletonTransformData.BodyOrientation == O

DEFINE_LOG_CATEGORY(LogVRIKBody);

UVRIKBody::UVRIKBody()
{
	PrimaryComponentTick.bCanEverTick = false;
	
	// setup defaults
	bTorsoYawRotation = false;
	bTorsoPitchRotation = false;
	bTorsoRollRotation = false;
	ResetFootLocationL = false;
	ResetFootLocationR = false;
	ComputeFeetIKTargets = true;
	ComputeLegsIK = false;
	ComputeHandsIK = false;
	UseActorLocationAsFloor = true;
	VRInputOption = EVRInputSetup::DirectVRInput;
	LockShouldersRotation = false;
	FollowPawnTransform = true;
	DetectContinuousHeadYawRotation = false;
	bYawInterpToHead = false;
	bResetTorso = false;
	bIsSitting = bIsStanding = false;
	nIsRotating = 0;
	JumpingOffset = 0.f;
	TimerInterval = 0.01f;
	HeadHeight = 20.f;
	SpineLength = 50.f;
	MaxHeadRotation = 90.f;
	SoftHeadRotationLimit = 0.f;
	BodyWidth = 40.f;
	BodyThickness = 25.f;
	HeadHalfWidth = 10.f;
	HandLength = 60.f;
	nModifyHeightState = 0;
	RibcageToNeckOffset = FVector(0.f, 0.f, 10.f);
	NeckToHeadsetOffset = FVector(15.0f, 0.0f, 15.0f);
	CharacterHeight = CharacterHeightClear = 120.f;
	ArmSpanClear = HandLength * 2.f + BodyWidth;
	nYawControlCounter = 0.f;
	FootOffsetToGround = 10.f;
	FloorCollisionObjectType = ECollisionChannel::ECC_WorldStatic;
	TracedFloorLevel = 0.f;
	FeetCyclePhase = 0.f;
	RecalcCharacteHeightTime = 0.f;
	HeadLocationBeforeSitting = FVector::ZeroVector;
	ReplicateFullBodyState = false;
	ReplicateComponentsMovement = true;
	ReplicateInWorldSpace = false;
	SmoothingInterpolationSpeed = 24.f;
	bDebugOutput = false;
	EnableTorsoTwist = true;
	TraceFloorByObjectType = true;
	TorsoRotationSensitivity = 0.8f;
	bLockTorsoToHead = false;

	HandAttachedRight = HandAttachedLeft = false;
	HandParentRight = HandParentLeft = nullptr;
	HandAttachTransformRight = HandAttachTransformLeft = FTransform::Identity;
	HandAttachSocketRight = HandAttachSocketLeft = TEXT("");

	RightPalmOffset.SetTranslation(FVector(-15.92f, 1.78f, 5.16f));
	RightPalmOffset.SetRotation(FRotator(-50.872f, 4.478f, 3.374f).Quaternion());
	RightPalmOffset.SetScale3D(FVector(1.f, 1.f, 1.f));
	LeftPalmOffset.SetTranslation(FVector(-15.92f, -1.78f, 5.16f));
	LeftPalmOffset.SetRotation(FRotator(-50.872f, -4.478f, -3.374f).Quaternion());
	LeftPalmOffset.SetScale3D(FVector(1.f, 1.f, 1.f));
}

void UVRIKBody::BeginPlay()
{
	Super::BeginPlay();

	// clear memory
	CurrentVRInputIndex = CORR_SAVE_POINTS_NUM;

	// not used anymore
	for (int i = 0; i < CORR_POINTS_NUM; i++) {
		cor_linear[0][i] = (float)i;
		cor_linear[1][i] = (float)i;
		cor_linear[2][i] = (float)i;
	}

	// initialize
	OwningPawn = Cast<APawn>(GetOwner());
	if (OwningPawn)
	{
		FloorBaseComp = OwningPawn->GetRootComponent();
	}
	bCalibratedT = bCalibratedI = false;
	bIsInitialized = false;
	fDeltaTimeR = fDeltaTimeL = 0.f;

	UpperarmLength = HandLength * UpperarmForearmRatio / (UpperarmForearmRatio + 1.f);
	ThighLength = (LegsLength * 0.8f) * ThighCalfRatio / (ThighCalfRatio + 1.f);	
}

/* Networking Properties Replication */
void UVRIKBody::GetLifetimeReplicatedProps(TArray <FLifetimeProperty> &OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	if (ReplicateFullBodyState) {
		DOREPLIFETIME(UVRIKBody, NT_SkeletonTransformData);
	}
	DOREPLIFETIME(UVRIKBody, NT_InputHMD);
	DOREPLIFETIME(UVRIKBody, NT_InputHandRight);
	DOREPLIFETIME(UVRIKBody, NT_InputHandLeft);
}

void UVRIKBody::Initialize(USceneComponent* Camera, UPrimitiveComponent* RightController, UPrimitiveComponent* LeftController)
{
	CameraComponent = Camera;
	RightHandController = RightController;
	LeftHandController = LeftController;

	if (!bIsInitialized && IsValid(OwningPawn) && IsValid(CameraComponent) && IsValid(RightHandController) && IsValid(LeftHandController)) {
		FloorBaseComp = CameraComponent->GetAttachParent();

		if (GetIsReplicated() && IsValid(OwningPawn)) {
			// turn off input from VR controllers on remote machines
			int32 ControllerPlayerIndex = LOCALLY_CONTROLLED ? 0 : -1;

			if (RightController->IsA(UMotionControllerComponent::StaticClass()))
			{
				UMotionControllerComponent* handr = Cast<UMotionControllerComponent>(RightController);
				if (IsValid(handr)) handr->PlayerIndex = ControllerPlayerIndex;
			}
			if (LeftController->IsA(UMotionControllerComponent::StaticClass()))
			{
				UMotionControllerComponent* handl = Cast<UMotionControllerComponent>(LeftController);
				if (IsValid(handl)) handl->PlayerIndex = ControllerPlayerIndex;
			}
			if (CameraComponent->IsA(UCameraComponent::StaticClass()))
			{
				UCameraComponent* cam = Cast<UCameraComponent>(CameraComponent);
				if (IsValid(cam)) cam->bLockToHmd = (ControllerPlayerIndex == 0) ? true : false;
			}
		}

		ActivateInput(FloorBaseComp);
	}
}

void UVRIKBody::ComputeFrame(float DeltaTime, FIKBodyData& ReturnValue)
{
	ReturnValue = FIKBodyData();

	if (!bIsInitialized || !IsValid(OwningPawn))
	{
		return;
	}
	if (!IsValid(FloorBaseComp))
	{
		UE_LOG(LogVRIKBody, Warning, TEXT("Root Component initialization failed."));
		return;
	}

	const bool IsLocalInNetwork = LOCALLY_CONTROLLED;
	const bool IsReplicated = GetIsReplicated();


	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 0. Replication. ////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// if running at remote PC
	if (IsReplicated && !IsLocalInNetwork) {
		// interpolate current head and hands transforms to replicated transforms
		InputHMD = UKismetMathLibrary::TInterpTo(InputHMD, InputHMD_Target, DeltaTime, SmoothingInterpolationSpeed);
		InputHandRight = UKismetMathLibrary::TInterpTo(InputHandRight, InputHandRight_Target, DeltaTime, SmoothingInterpolationSpeed);
		InputHandLeft = UKismetMathLibrary::TInterpTo(InputHandLeft, InputHandLeft_Target, DeltaTime, SmoothingInterpolationSpeed);

		const FTransform w_InputHMD = GetHMDTransform();
		const FTransform w_InputHandRight = GetHandTransform(EControllerHand::Right);
		const FTransform w_InputHandLeft = GetHandTransform(EControllerHand::Left);

		// update local components if necessary
		if (ReplicateComponentsMovement) {
			if (IsValid(CameraComponent)) {
				FTransform t_newcam = UKismetMathLibrary::TInterpTo(CameraComponent->GetComponentTransform(), w_InputHMD, DeltaTime, SmoothingInterpolationSpeed);

				CameraComponent->SetWorldLocationAndRotation(t_newcam.GetTranslation(), t_newcam.GetRotation());
			}
			if (IsValid(RightHandController)) {
				FTransform t_newctrlr = UKismetMathLibrary::TInterpTo(RightHandController->GetComponentTransform(), w_InputHandRight, DeltaTime, SmoothingInterpolationSpeed);

				RightHandController->SetWorldLocationAndRotation(t_newctrlr.GetTranslation(), t_newctrlr.GetRotation());
			}
			if (IsValid(LeftHandController)) {
				FTransform t_newctrll = UKismetMathLibrary::TInterpTo(LeftHandController->GetComponentTransform(), w_InputHandLeft, DeltaTime, SmoothingInterpolationSpeed);

				LeftHandController->SetWorldLocationAndRotation(t_newctrll.GetTranslation(), t_newctrll.GetRotation());
			}
		}

		// interpolate current body state to replicated
		if (ReplicateFullBodyState)
		{
			SkeletonTransformDataRelative.Pelvis = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.Pelvis, SkeletonTransformData_Target.Pelvis, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.Ribcage = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.Ribcage, SkeletonTransformData_Target.Ribcage, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.Neck = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.Neck, SkeletonTransformData_Target.Neck, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.Head = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.Head, SkeletonTransformData_Target.Head, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.HandRight = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.HandRight, SkeletonTransformData_Target.HandRight, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.HandLeft = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.HandLeft, SkeletonTransformData_Target.HandLeft, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.FootRightCurrent = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.FootRightCurrent, SkeletonTransformData_Target.FootRightCurrent, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.FootLeftCurrent = UKismetMathLibrary::TInterpTo(SkeletonTransformDataRelative.FootLeftCurrent, SkeletonTransformData_Target.FootLeftCurrent, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.ElbowJointTargetRight = FMath::VInterpTo(SkeletonTransformDataRelative.ElbowJointTargetRight, SkeletonTransformData_Target.ElbowJointTargetRight, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.ElbowJointTargetLeft = FMath::VInterpTo(SkeletonTransformDataRelative.ElbowJointTargetLeft, SkeletonTransformData_Target.ElbowJointTargetLeft, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.Velocity = FMath::VInterpTo(SkeletonTransformDataRelative.Velocity, SkeletonTransformData_Target.Velocity, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.GroundLevel = FMath::FInterpTo(SkeletonTransformDataRelative.GroundLevel, SkeletonTransformData_Target.GroundLevel, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.GroundLevelRight = FMath::FInterpTo(SkeletonTransformDataRelative.GroundLevelRight, SkeletonTransformData_Target.GroundLevelRight, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.GroundLevelLeft = FMath::FInterpTo(SkeletonTransformDataRelative.GroundLevelLeft, SkeletonTransformData_Target.GroundLevelLeft, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.IsJumping = SkeletonTransformData_Target.IsJumping;
			SkeletonTransformDataRelative.CollarboneRight = FMath::RInterpTo(SkeletonTransformDataRelative.CollarboneRight, SkeletonTransformData_Target.CollarboneRight, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.CollarboneLeft = FMath::RInterpTo(SkeletonTransformDataRelative.CollarboneLeft, SkeletonTransformData_Target.CollarboneLeft, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.LowerarmTwistRight = FMath::FInterpTo(SkeletonTransformDataRelative.LowerarmTwistRight, SkeletonTransformData_Target.LowerarmTwistRight, DeltaTime, SmoothingInterpolationSpeed);
			SkeletonTransformDataRelative.LowerarmTwistLeft = FMath::FInterpTo(SkeletonTransformDataRelative.LowerarmTwistLeft, SkeletonTransformData_Target.LowerarmTwistLeft, DeltaTime, SmoothingInterpolationSpeed);

			SkeletonTransformData.BodyOrientation = ComputeCurrentBodyOrientation(w_InputHMD, w_InputHandRight, w_InputHandLeft);

			// convert back to world space
			if (!ReplicateInWorldSpace) {
				bool bRet = RestoreBodyFromRelative();
			}
			else {
				SkeletonTransformData = SkeletonTransformDataRelative;
			}

			// don't need to calculate if full body is replicated
			ReturnValue = SkeletonTransformData;

			return;
		}
	}

	// head transform variables
	const FTransform	wcam = GetHMDTransform();
	const FVector		wcamloc = wcam.GetTranslation();
	FRotator			wcamrot = wcam.GetRotation().Rotator();

	// update ground position
	if (!UseActorLocationAsFloor) TraceFloor(wcam.GetTranslation());
	const float FloorZ = GetFloorCoord();
	const float BaseZ = FloorBaseComp->GetComponentLocation().Z;

	// init other transform variables
	const FTransform whandr = GetHandTransform(EControllerHand::Right);
	const FTransform whandl = GetHandTransform(EControllerHand::Left);
	FTransform whandl_fixed = LeftPalmOffset * whandl;								// add hand offset to controller transform
	FTransform whandr_fixed = RightPalmOffset * whandr;								// add hand offset to controller transform
	const FVector PrevPelvisLoc = SkeletonTransformData.Pelvis.GetTranslation();
	const float CurrHeight = wcamloc.Z - BaseZ;

	// get current body orientation
	// this variable allow to choose a proper way to calculate body
	SkeletonTransformData.BodyOrientation = ComputeCurrentBodyOrientation(wcam, whandr_fixed, whandl_fixed);

	// adjust head rotation to keep roll in -90..90, pitch in -180..90
	if (FMath::Abs(wcamrot.Roll) >= 90.f) {
		float y = -wcamrot.Yaw - 180.f, p = -wcamrot.Pitch - 180.f;

		if (y < -180.f) y += 360.f; else if (y > 180.f) y -= 360.f;
		if (p < -180.f) p += 360.f; else if (p > 180.f) p -= 360.f;
		wcamrot = FRotator(p, y, 0.f);
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 1. Head. ///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SkeletonTransformData.Head.SetTranslation(wcamloc - wcamrot.RotateVector(FVector(HeadHalfWidth, 0.f, 0.f)));
	SkeletonTransformData.Head.SetRotation(wcam.GetRotation());

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 2. Ribcage and Pelvis. /////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SkeletonTransformData.Neck.SetTranslation(wcamloc - wcamrot.RotateVector(NeckToHeadsetOffset));
	SkeletonTransformData.Neck.SetRotation(UKismetMathLibrary::FindLookAtRotation(SkeletonTransformData.Neck.GetTranslation(), SkeletonTransformData.Head.GetTranslation()).Quaternion());

	FVector RibcageLoc = SkeletonTransformData.Neck.GetTranslation();
	SkeletonTransformData.Ribcage.SetTranslation(RibcageLoc);
	FVector ShoulderRight = SkeletonTransformData.Ribcage.GetTranslation() + SkeletonTransformData.Pelvis.GetRotation().GetRightVector() * (BodyWidth * 0.5f);
	FVector ShoulderLeft = SkeletonTransformData.Ribcage.GetTranslation() - SkeletonTransformData.Pelvis.GetRotation().GetRightVector() * (BodyWidth * 0.5f);
	FRotator PelvisRot = SkeletonTransformData.Pelvis.Rotator();
	SkeletonTransformData.UpperarmRight.SetTranslation(ShoulderRight);
	SkeletonTransformData.UpperarmLeft.SetTranslation(ShoulderLeft);

	// count artificial rotation
	const FRotator NewWorldRot = FloorBaseComp->GetComponentRotation();
	float YawDiffInWorld = 0.f;
	if (NewWorldRot.Yaw != PrevFrameActorRot.Yaw && !bManualBodyOrientation)
	{
		if (NewWorldRot.Yaw * PrevFrameActorRot.Yaw > 0.f) {
			YawDiffInWorld = (NewWorldRot.Yaw - PrevFrameActorRot.Yaw);
		}
		else {
			const FRotator NewWorldDeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(NewWorldRot, PrevFrameActorRot);
			YawDiffInWorld = NewWorldDeltaRot.Yaw;
		}
		PelvisRot.Yaw += YawDiffInWorld;

		PrevFrameActorRot.Yaw = NewWorldRot.Yaw;
	}


	// calculate ribcage for standing/sitting
	if (CHECKORIENTATION(EBodyOrientation::Stand) || CHECKORIENTATION(EBodyOrientation::Sit))
	{
		// pelvis yaw rotation
		const float PelvisRotYaw = GetForwardYaw(PelvisRot.Quaternion().GetForwardVector(), PelvisRot.Quaternion().GetUpVector());
		const float CameraRotYaw = GetForwardYaw(wcam.GetRotation().GetForwardVector(), wcam.GetRotation().GetUpVector());

		// if correlation exists
		if (bTorsoYawRotation || FMath::Abs(PelvisRotYaw - CameraRotYaw) > MaxHeadRotation) {
			if (nIsRotating < 30) {
				StartRotationYaw = PelvisRot.Yaw;
				nIsRotating = 30;
			}
			if (CharacterHeight - CurrHeight < 35.f) {
				if (bLockTorsoToHead)
				{
					FRotator TargetPelvisRot = PelvisRot;
					TargetPelvisRot.Yaw = GetForwardYaw(wcam.GetRotation().GetForwardVector(), wcam.GetRotation().GetUpVector());
					PelvisRot = FMath::RInterpTo(PelvisRot, TargetPelvisRot, DeltaTime, 8.f);
				}
				else
				{
					float YawDiff = wcamrot.Yaw - rPrevCamRotator.Yaw;
					PelvisRot.Yaw += (YawDiff - YawDiffInWorld);
				}
			}
		}
		// if rotation ended
		else if (nIsRotating > 0) {
			// wait for 30 frames
			nIsRotating--;
			// and if some noticable angle between head and spine remains (previous value: 15.f)
			if (nIsRotating == 0 && FMath::Abs(GetAngleToInterp(PelvisRotYaw, StartRotationYaw) - StartRotationYaw) > 20.f) {
				bYawInterpToHead = true;
			}
		}

		// remove long-time head-pelvis yaw mismach
		if (bYawInterpToHead && ((CharacterHeight - CurrHeight < 10.f) || (false == ComputeLegsIK && (CharacterHeight - CurrHeight < 30.f)))) {
			PelvisRot.Yaw = FMath::FInterpTo(GetAngleToInterp(PelvisRotYaw, CameraRotYaw), CameraRotYaw, DeltaTime, 3.f);
			if (FMath::Abs(GetAngleToInterp(PelvisRot.Yaw, CameraRotYaw) - CameraRotYaw) < 5.f) {
				bYawInterpToHead = false;
			}
		}

		// ribcage pitch depeinding of body state
		if (nModifyHeightState == HEIGHT_STABLE) {
			// 07.31 float HeadZ = SkeletonTransformData.Head.GetTranslation().Z - BaseZ;
			const float HeadZ = SkeletonTransformData.Head.GetRelativeTransform(FloorBaseComp->GetComponentTransform()).GetTranslation().Z;

			JumpingOffset = HeadZ - CharacterHeight;
			SkeletonTransformData.IsJumping = (JumpingOffset > 9.f);

			// is in air
			if (SkeletonTransformData.IsJumping || bResetTorso) {
				FRotator target = FRotator(0.f, PelvisRotYaw, 0.f);
				PelvisRot = FMath::RInterpTo(PelvisRot, target, DeltaTime, 10.f);

				if (bResetTorso) {
					FRotator src = SkeletonTransformData.Ribcage.Rotator();
					src = FMath::RInterpTo(src, target, DeltaTime, 10.f);
					SkeletonTransformData.Ribcage.SetRotation(src.Quaternion());
				}

				if (bResetTorso && FMath::Abs(PelvisRot.Pitch) < 0.5f && FMath::Abs(PelvisRot.Roll) < 0.5f) {
					bResetTorso = false;
				}

				// character height correction
				RecalcCharacteHeightTime = 0.f;
				RecalcCharacteHeightTimeJumping += DeltaTime;
				if (RecalcCharacteHeightTimeJumping > 2.f) {
					CharacterHeight = HeadZ;
					RecalcCharacteHeightTimeJumping = 0.f;
				}
			}
			// is staying
			else if (JumpingOffset > -5.f) {
				// ignore roll and pitch spine rotations
				FRotator target = FRotator(0.f, PelvisRotYaw, 0.f);
				PelvisRot = FMath::RInterpTo(PelvisRot, target, DeltaTime, 10.f);

				HeadPitchBeforeSitting = wcamrot.Pitch;
				HeadLocationBeforeSitting = wcamloc;
				RecalcCharacteHeightTimeJumping = 0.f;
			}
			// leaning or crouching
			else {
				// character height correction
				RecalcCharacteHeightTimeJumping = 0.f;
				if (JumpingOffset > -10.f) {
					RecalcCharacteHeightTime += DeltaTime;
					if (RecalcCharacteHeightTime > 3.f) {
						CharacterHeight = HeadZ;
						RecalcCharacteHeightTime = 0.f;
					}
				}
				else {
					RecalcCharacteHeightTime = 0.f;
				}

				// 07.31 bIsSitting = (SkeletonTransformData.Pelvis.GetTranslation().Z - BaseZ < 50.f) && (PelvisRot.Pitch > -20.f);
				const FTransform PelvisRelative = SkeletonTransformData.Pelvis.GetRelativeTransform(FloorBaseComp->GetComponentTransform());
				bIsSitting = (PelvisRelative.GetTranslation().Z < 50.f) && (PelvisRelative.Rotator().Pitch > -20.f);

				JumpingOffset = -JumpingOffset;

				const bool bPelvisisTooLow = (SkeletonTransformData.Pelvis.GetTranslation().Z < BaseZ + 35.f) || (wcamrot.Pitch < HeadPitchBeforeSitting - 20.f && !bIsSitting);
				const bool DeclineAtStandUp = PelvisRot.Pitch < -10.f && (wcamrot.Pitch > rPrevCamRotator.Pitch);

				// if character is walking: move pelvis
				if (bIsWalking) {
					const FVector CamShift = wcamloc - vPrevCamLocation;
					HeadLocationBeforeSitting.X += CamShift.X;
					HeadLocationBeforeSitting.Y += CamShift.Y;
				}

				// if pelvis is too low: increase pelvis Z coordinate
				if (SkeletonTransformData.Pelvis.GetTranslation().Z < BaseZ + 35.f) {
					float dz = RibcageLoc.Z - (BaseZ + 35.f);
					float pt = -FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(dz / SpineLength, -1.f, 1.f)));

					pt = FMath::FInterpTo(PelvisRot.Pitch, pt, DeltaTime, 8.f);
					PelvisRot = FMath::RInterpTo(PelvisRot, FRotator(pt, PelvisRotYaw, 0.f), DeltaTime, 10.f);
				}
				// otherwise: calc spine pitch angle based on horizontal offset
				else {
					float pt;
					const FVector doffset = HeadLocationBeforeSitting - wcamloc;
					FVector doffsethor = doffset * -1.f; doffsethor.Z = 0.f; doffsethor.Normalize();
					FVector camhor = wcam.GetRotation().GetForwardVector(); camhor.Z = 0.f; camhor.Normalize();
					const float dhor = FMath::Sqrt(doffset.X*doffset.X + doffset.Y*doffset.Y) * FVector::DotProduct(doffsethor, camhor) * 1.5f + 15.f;

					pt = -FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(dhor / SpineLength, -1.f, 1.f)));
					pt = FMath::Clamp(pt, -89.f, 0.f);

					pt = FMath::FInterpTo(PelvisRot.Pitch, pt, DeltaTime, 8.f);
					PelvisRot = FMath::RInterpTo(PelvisRot, FRotator(pt, PelvisRotYaw, 0.f), DeltaTime, 10.f);
				}


				// clamp to max possible pitch/roll

				const FVector RibcageVec = PelvisRot.Quaternion().GetUpVector();
				const float SpineZProjection = FMath::Abs(RibcageVec.Z) * SpineLength;
				const float MinRequirdSpineZProjection = RibcageLoc.Z - BaseZ - LegsLength;

				// standing up: make sure feet are on the ground (decrease absolute pitch value if necessary)
				if (SpineZProjection < MinRequirdSpineZProjection) {

					float AngleProjected = FMath::RadiansToDegrees(FMath::Acos(MinRequirdSpineZProjection / SpineLength));
					float HorProjectionSize = FMath::Sqrt(SpineLength*SpineLength - MinRequirdSpineZProjection*MinRequirdSpineZProjection);
					FVector HorProjectionAngle = FVector(RibcageVec.X, RibcageVec.Y, 0.f);

					HorProjectionAngle.Normalize();
					HorProjectionAngle *= HorProjectionSize;
					HorProjectionAngle.Z = MinRequirdSpineZProjection;
					HorProjectionAngle.Normalize();

					PelvisRot = FMath::RInterpTo(PelvisRot, FRotator(-AngleProjected, PelvisRotYaw, 0.f), DeltaTime, 10.f);
				}
			}
		}

		// Adjust Ribcage Location now when we know pelvis-ribcage axis
		RibcageLoc -= PelvisRot.RotateVector(RibcageToNeckOffset);
		SkeletonTransformData.Ribcage.SetTranslation(RibcageLoc);

		SkeletonTransformData.IsSitting = bIsSitting;
	}
	// crawling
	else if (CHECKORIENTATION(EBodyOrientation::Crawl) || CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown))
	{		
		const float LimitZ = CharacterHeight * 0.2f + BaseZ;
		FRotator TargetPelvisRot;

		bool bForceHeadOrientation = bManualBodyOrientation && CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown);

		if (bForceHeadOrientation)
		{
			const FTransform rcam = GetHMDTransform(true);
			FVector HeadForward = GetForwardDirection(rcam.GetRotation().GetForwardVector(), rcam.GetRotation().GetUpVector());
			HeadForward = FloorBaseComp->GetComponentRotation().RotateVector(HeadForward);

			TargetPelvisRot = UKismetMathLibrary::MakeRotFromZX(HeadForward, wcam.GetRotation().GetUpVector() * -1.f);
			PelvisRot = UKismetMathLibrary::RInterpTo(PelvisRot, TargetPelvisRot, DeltaTime, 4.f);
		}
		// if hands are on the ground: pelvis rotation follow hands
		else if (whandr_fixed.GetTranslation().Z < LimitZ && whandl_fixed.GetTranslation().Z < LimitZ)
		{
			FVector HeadForward = GetForwardDirection(wcam.GetRotation().GetUpVector(), -wcam.GetRotation().GetForwardVector());
			FVector RightDir = whandr_fixed.GetTranslation() - whandl_fixed.GetTranslation();

			RightDir.Normalize();
			if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown)) {
				TargetPelvisRot = UKismetMathLibrary::MakeRotFromYX(RightDir, FloorBaseComp->GetComponentQuat().GetUpVector() * -1.f);
			}
			else {
				TargetPelvisRot = UKismetMathLibrary::MakeRotFromYZ(RightDir, HeadForward);
			}

			PelvisRot = UKismetMathLibrary::RInterpTo(PelvisRot, TargetPelvisRot, DeltaTime, 1.5f);
		}
		// if at least on hand is up: pelvis is following head
		else
		{
			FVector HeadForward = GetForwardDirection(wcam.GetRotation().GetUpVector(), -wcam.GetRotation().GetForwardVector());
			FVector HeadDown = FMath::Lerp(HeadForward, FVector(0.f, 0.f, -1.f), 0.7f).GetSafeNormal();

			TargetPelvisRot = UKismetMathLibrary::MakeRotFromXZ(HeadDown, HeadForward);
			PelvisRot = UKismetMathLibrary::RInterpTo(PelvisRot, TargetPelvisRot, DeltaTime, 4.f);
		}

		// update ribcage froom PelvisRot
		RibcageLoc -= PelvisRot.RotateVector(RibcageToNeckOffset);
		SkeletonTransformData.Ribcage.SetTranslation(RibcageLoc);
		SkeletonTransformData.Ribcage.SetRotation(PelvisRot.Quaternion());
	}
	// laying down and looking to the sky* (* - or roof)
	else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceUp))
	{
		// TODO: add pelvis/ribcage calculation for LieDown_FaceUp

		const FRotator TargetPelvisRot = UKismetMathLibrary::MakeRotFromXZ(FloorBaseComp->GetComponentQuat().GetUpVector(), SavedTorsoDirection);
		PelvisRot = UKismetMathLibrary::RInterpTo(PelvisRot, TargetPelvisRot, DeltaTime, 3.f);

		// update ribcage froom PelvisRot
		RibcageLoc -= PelvisRot.RotateVector(RibcageToNeckOffset);
		SkeletonTransformData.Ribcage.SetTranslation(RibcageLoc);
		SkeletonTransformData.Ribcage.SetRotation(PelvisRot.Quaternion());
	}

	// PELVIS ----------------------------------------------------------------------------
	SkeletonTransformData.Pelvis.SetTranslation(RibcageLoc - PelvisRot.Quaternion().GetUpVector() * SpineLength);
	SkeletonTransformData.Pelvis.SetRotation(PelvisRot.Quaternion());
	SkeletonTransformData.Velocity = (SkeletonTransformData.Pelvis.GetTranslation() - PrevPelvisLoc) / DeltaTime;
	SkeletonTransformData.GroundLevel = FloorZ;
	SkeletonTransformData.GroundLevelRight = TracedFloorLevelR;
	SkeletonTransformData.GroundLevelLeft = TracedFloorLevelL;
	SkeletonTransformData.IsSitting = (SkeletonTransformData.Pelvis.GetTranslation().Z - FloorZ < 50.f);

	// save current camera values
	vPrevCamLocation = wcamloc;
	rPrevCamRotator = wcamrot;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 3. Hands. //////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	

	if (HandAttachedRight)
	{
		whandr_fixed = RightPalmOffset * GetHandTransform(EControllerHand::Right, true, false);
	}
	if (HandAttachedLeft)
	{
		whandl_fixed = LeftPalmOffset * GetHandTransform(EControllerHand::Left, true, false);
	}

	SkeletonTransformData.HandLeft = whandl_fixed;
	SkeletonTransformData.HandRight = whandr_fixed;

	if (ComputeHandsIK)
	{
		CalcShouldersWithOffset(EControllerHand::Left);
		CalcShouldersWithOffset(EControllerHand::Right);

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Ribcage twisting. //////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		FRotator RibcageRot = SkeletonTransformData.Ribcage.Rotator();

		if (EnableTorsoTwist && !bResetTorso && SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl) {
			bool bResetTorsoToPelvis = bResetTorso;
			bool bUpdateShoulders = false;

			const FRotator PelvisRotYaw = UKismetMathLibrary::NormalizedDeltaRotator(wcam.Rotator(), PelvisRot);
			const float PelvisHeadOffset = PelvisRotYaw.Yaw;

			// soft limit
			if (FMath::Abs(PelvisHeadOffset) > SoftHeadRotationLimit && SoftHeadRotationLimit > 0.f) {
				const float ShiftYaw = (PelvisHeadOffset > 0.f) ?
										PelvisHeadOffset - SoftHeadRotationLimit :
										SoftHeadRotationLimit + PelvisHeadOffset;

				FRotator target = RibcageRot;
				target.Yaw = PelvisRot.Yaw + ShiftYaw;
				RibcageRot = FMath::RInterpTo(RibcageRot, target, DeltaTime, 15.f);

				bResetTorsoToPelvis = false;
				bUpdateShoulders = true;
			}
			// just stretchign hands
			else
			{
				if (CharacterHeight - CurrHeight < 30.f && !bIsSitting && !SkeletonTransformData.IsJumping) {
					// TODO: refactoring needed
					ShoulderRight = SkeletonTransformData.Ribcage.GetTranslation() + PelvisRot.Quaternion().GetRightVector() * (BodyWidth * 0.5f);
					ShoulderLeft = SkeletonTransformData.Ribcage.GetTranslation() - PelvisRot.Quaternion().GetRightVector() * (BodyWidth * 0.5f);

					const FVector condDirForward = SkeletonTransformData.Head.GetRotation().GetForwardVector();
					const FVector condDirRight = PelvisRot.Quaternion().GetRightVector();
					const FVector sh2handR = whandr_fixed.GetTranslation() - ShoulderRight;
					const FVector sh2handL = whandl_fixed.GetTranslation() - ShoulderLeft;

					const bool condRight = (sh2handR.Size() > HandLength + 11.f && sh2handR.Size2D() > HandLength * 0.7f && FVector::DotProduct(sh2handR, condDirRight) < 0.7f);
					const bool condLeft = (sh2handL.Size() > HandLength + 11.f && sh2handL.Size2D() > HandLength * 0.7f && FVector::DotProduct(sh2handL, condDirRight) > -0.7f);

					if (condRight && condLeft && FVector::DotProduct(sh2handR.GetSafeNormal(), condDirForward) * FVector::DotProduct(sh2handL.GetSafeNormal(), condDirForward) > 0.f) {
						bResetTorsoToPelvis = true;
					}
					else if (condRight) {
						// right hand can't reach required hand palm location
						float a = IKB_AdjustRibcageYawForShoulder(whandr_fixed.GetTranslation(), RibcageLoc, PelvisRot, HandLength, BodyWidth * 0.5f);
						//a = 30.f;

						if (a < 45.f) {
							FRotator target = PelvisRot;
							if (IsHandInFront(SkeletonTransformData.Pelvis.GetRotation().GetForwardVector(), RibcageLoc, whandr_fixed.GetTranslation())) {
								target.Yaw -= (90.f - a - ShoulderYawOffsetRight);
							}
							else {
								target.Yaw += (90.f - a - ShoulderYawOffsetRight);
							}
							RibcageRot = FMath::RInterpTo(RibcageRot, target, DeltaTime, 8.f);
							bUpdateShoulders = true;
						}
						bResetTorsoToPelvis = true;
					}
					else if (condLeft) {
						// left hand can't reach required hand palm location
						float a = IKB_AdjustRibcageYawForShoulder(whandl_fixed.GetTranslation(), RibcageLoc, PelvisRot, HandLength, BodyWidth * 0.5f);
						a = 30.f;

						if (a < 45.f) {
							FRotator target = PelvisRot;
							if (IsHandInFront(SkeletonTransformData.Pelvis.GetRotation().GetForwardVector(), RibcageLoc, whandl_fixed.GetTranslation())) {
								target.Yaw += (90.f - a - ShoulderYawOffsetLeft);
							}
							else {
								target.Yaw -= (90.f - a - ShoulderYawOffsetRight);
							}
							RibcageRot = FMath::RInterpTo(RibcageRot, target, DeltaTime, 8.f);
							bUpdateShoulders = true;
						}
						bResetTorsoToPelvis = true;
					}
					else {
						bResetTorsoToPelvis = true;
					}
				}
				// if sitting/jumping - interpolate ribcage rotation to pelvis
				else {
					bResetTorsoToPelvis = true;
				}
			}

			if (bResetTorsoToPelvis && FMath::Abs(RibcageRot.Yaw - PelvisRot.Yaw) > 0.1f) {
				RibcageRot = FMath::RInterpTo(RibcageRot, PelvisRot, DeltaTime, 5.f);
				bUpdateShoulders = true;
			}

			SkeletonTransformData.Ribcage.SetRotation(RibcageRot.Quaternion());

			if (bUpdateShoulders) {
				CalcShouldersWithOffset(EControllerHand::Left);
				CalcShouldersWithOffset(EControllerHand::Right);
			}
		}
		else {
			SkeletonTransformData.Ribcage.SetRotation(PelvisRot.Quaternion());
		}

		CalcHandIKTransforms(EControllerHand::Right);
		CalcHandIKTransforms(EControllerHand::Left);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 4. Feet. ///////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	if (ComputeFeetIKTargets)
	{
		CalcFeetIKTransforms2(DeltaTime, FloorZ);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 5. Replication. ////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// if locally controlled - send VR input data and body (if necessary) to server
	if (IsReplicated)
	{
		ConvertBodyToRelative(); 
		
		if (IsLocalInNetwork)
		{
			PackDataForReplication(wcam, whandr, whandl);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 6. Finish. /////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ReturnValue = SkeletonTransformData;
}

FIKBodyData UVRIKBody::GetLastFrameData()
{
	if (GetIsReplicated() && IsValid(OwningPawn) && !LOCALLY_CONTROLLED)
	{
		bool bRet = RestoreBodyFromRelative();
		return SkeletonTransformData;
	}
	else
	{
		return SkeletonTransformData;
	}
};

// Not in use!
FIKBodyData UVRIKBody::ConvertDataToSkeletonFriendly(const FIKBodyData& WorldSpaceIKBody)
{
	FIKBodyData ret = WorldSpaceIKBody;
	FRotator r;

	// make rotations match normal bones rotations

	// 1) head
	r = UKismetMathLibrary::MakeRotationFromAxes(ret.Head.GetRotation().GetUpVector(), ret.Head.GetRotation().GetForwardVector(), ret.Head.GetRotation().GetRightVector());
	ret.Head.SetRotation(r.Quaternion());

	// 2) hand palms
	r = ret.HandLeft.Rotator();
	r.Roll += 180.f;
	ret.HandLeft.SetRotation(r.Quaternion());

	r = ret.HandRight.Rotator();
	r.Roll = -1.f * (r.Roll + 180.f); r.Pitch += 180.f;
	ret.HandRight.SetRotation(r.Quaternion());

	// 3) hands (right only actually)
	r = ret.UpperarmRight.Rotator();
	r.Pitch += 180.f;
	ret.UpperarmRight.SetRotation(r.Quaternion());

	r = ret.ForearmRight.Rotator();
	r.Pitch += 180.f;
	ret.ForearmRight.SetRotation(r.Quaternion());

	// 4) ribcage
	r = UKismetMathLibrary::MakeRotationFromAxes(ret.Ribcage.GetRotation().GetUpVector(), ret.Ribcage.GetRotation().GetForwardVector(), ret.Ribcage.GetRotation().GetRightVector());
	ret.Ribcage.SetRotation(r.Quaternion());

	// 5) pelvis

	// 6) legs
	r = ret.ThighRight.Rotator();
	r.Roll += 90.f;
	ret.ThighRight.SetRotation(r.Quaternion());

	r = ret.CalfRight.Rotator();
	r.Roll += 90.f;
	ret.CalfRight.SetRotation(r.Quaternion());

	r = UKismetMathLibrary::MakeRotationFromAxes(-ret.ThighLeft.GetRotation().GetForwardVector(), ret.ThighLeft.GetRotation().GetUpVector(), ret.ThighLeft.GetRotation().GetRightVector());
	ret.ThighLeft.SetRotation(r.Quaternion());

	r = UKismetMathLibrary::MakeRotationFromAxes(-ret.CalfLeft.GetRotation().GetForwardVector(), ret.CalfLeft.GetRotation().GetUpVector(), ret.CalfLeft.GetRotation().GetRightVector());
	ret.CalfLeft.SetRotation(r.Quaternion());

	// 7) feet

	return ret;
}

void UVRIKBody::ResetTorso()
{
	bResetTorso = true;
	bYawInterpToHead = true;
}

bool UVRIKBody::AttachHandToComponent(EControllerHand Hand, UPrimitiveComponent* Component, const FName& SocketName, const FTransform& RelativeTransform)
{
	if (!IsValid(Component)) return false;

	if (Hand == EControllerHand::Right)
	{
		HandAttachedRight = true;
		HandParentRight = Component;
		HandAttachTransformRight = RelativeTransform;
		HandAttachSocketRight = SocketName;
	}
	else
	{
		HandAttachedLeft = true;
		HandParentLeft = Component;
		HandAttachTransformLeft = RelativeTransform;
		HandAttachSocketLeft = SocketName;
	}

	return true;
}

void UVRIKBody::DetachHandFromComponent(EControllerHand Hand)
{
	if (Hand == EControllerHand::Right) {
		HandAttachedRight = false;
		HandParentRight = nullptr;
		HandAttachTransformRight = FTransform::Identity;
		HandAttachSocketRight = TEXT("");
	}
	else {
		HandAttachedLeft = false;
		HandParentLeft = nullptr;
		HandAttachTransformLeft = FTransform::Identity;
		HandAttachSocketLeft = TEXT("");
	}
}

void UVRIKBody::VRInputTimer_Tick()
{
	if (!IsValid(OwningPawn)) return;
	if (CurrentVRInputIndex == CORR_SAVE_POINTS_NUM) CurrentVRInputIndex = 0; else CurrentVRInputIndex++;

	const FTransform CamTr = GetHMDTransform(true);
	const FTransform RHTr = GetHandTransform(EControllerHand::Right, false, true, true);
	const FTransform LHTr = GetHandTransform(EControllerHand::Left, false, true, true);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 1. Add current data to arrays ////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VRInputData[CurrentVRInputIndex].HeadLoc = CamTr.GetTranslation();
	VRInputData[CurrentVRInputIndex].RightHandLoc = RHTr.GetTranslation();
	VRInputData[CurrentVRInputIndex].LeftHandLoc = LHTr.GetTranslation();

	VRInputData[CurrentVRInputIndex].HeadRot = CamTr.GetRotation().Rotator();
	VRInputData[CurrentVRInputIndex].RightHandRot = RHTr.Rotator();
	VRInputData[CurrentVRInputIndex].LeftHandRot = LHTr.Rotator();

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 2. Calculate correlation coeffitients ////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GetCorrelationKoef();

	// a. Rotation velocities
	float CorrelationResultYaw1, CorrelationResultYaw2;
	float r1, r3;

	IKB_CorrelateArraysExt(cor_rot_a, cor_rot_b, CORR_POINTS_NUM, r1, CorrelationResultYaw1, r3);
	IKB_CorrelateArraysExt(cor_rot_a, cor_rot_c, CORR_POINTS_NUM, r1, CorrelationResultYaw2, r3);

	// b. Location values
	FVector MovementCorrelation1, MovementCorrelation2;
	IKB_CorrelateArraysExt(cor_rotv_a, cor_rotv_b, CORR_POINTS_NUM, MovementCorrelation1.X, MovementCorrelation1.Y, MovementCorrelation1.Z);
	IKB_CorrelateArraysExt(cor_rotv_a, cor_rotv_c, CORR_POINTS_NUM, MovementCorrelation2.X, MovementCorrelation2.Y, MovementCorrelation2.Z);

	FVector fHeadSpeed;
	int32 PrevIndex = CurrentVRInputIndex > 0 ? CurrentVRInputIndex - 1 : CORR_SAVE_POINTS_NUM - 1;
	fHeadSpeed.X = VRInputData[CurrentVRInputIndex].HeadLoc.X - VRInputData[PrevIndex].HeadLoc.X;
	fHeadSpeed.Y = VRInputData[CurrentVRInputIndex].HeadLoc.Y - VRInputData[PrevIndex].HeadLoc.Y;
	fHeadSpeed.Z = VRInputData[CurrentVRInputIndex].HeadLoc.Z - VRInputData[PrevIndex].HeadLoc.Z;
	fHeadSpeed /= TimerInterval;

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 3. Results analysis //////////////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Yaw Rotation
	const float YawCoef = FMath::Lerp(0.9f, 0.65f, TorsoRotationSensitivity);
	bTorsoYawRotation = bLockTorsoToHead || (CorrelationResultYaw1 > YawCoef && CorrelationResultYaw2 > YawCoef);

	// Pitch Rotation
	fHeadSpeed.X = FMath::Sqrt(fHeadSpeed.X*fHeadSpeed.X + fHeadSpeed.Y*fHeadSpeed.Y);
	if (fHeadSpeed.X > 48.f && FMath::Abs(fHeadSpeed.Z) > 38.f)
	{
		bTorsoPitchRotation = true;
	}
	else
	{
		bTorsoPitchRotation = false;
	}

	// Walking
	bIsWalking = (
		MovementCorrelation1.X > 0.85f &&
		MovementCorrelation1.Y > 0.85f &&
		MovementCorrelation2.X > 0.85f &&
		MovementCorrelation2.Y > 0.85f &&
		FMath::Abs(fHeadSpeed.X) > 20.f &&
		FMath::Abs(fHeadSpeed.Z) < 50.f
		);
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 3. Check camera Z coordinate /////////////////////////////////////////////////////////////////////////////
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	float val = CamTr.GetTranslation().Z - GetFloorCoord();
	if (CharacterHeight < val) {
		// initial CharacterHeight setup before body calibration

		if (nModifyHeightState == HEIGHT_INIT && val > 2.0f /* ignore fluctuations; start when player takes HMD */)
		{
			nModifyHeightState = HEIGHT_MODIFY;
			ModifyHeightStartTime = GetWorld()->GetRealTimeSeconds();
		}
		else if (nModifyHeightState == HEIGHT_MODIFY)
		{
			if (GetWorld()->GetRealTimeSeconds() - ModifyHeightStartTime > 4.0f /* 4 seconds to set height */)
			{
				nModifyHeightState = HEIGHT_STABLE;
			}
		}

		if (nModifyHeightState == HEIGHT_MODIFY)
		{
			// update player height
			CharacterHeight = val;

			FVector r = SkeletonTransformData.Pelvis.GetTranslation();
			r.Z = val - (FMath::Abs(NeckToHeadsetOffset.Z) + RibcageToNeckOffset.Z + SpineLength + HeadHeight);
			SkeletonTransformData.Pelvis.SetTranslation(r);

			LegsLength = CharacterHeight - SpineLength - RibcageToNeckOffset.Z - NeckToHeadsetOffset.Z;
			ThighLength = (LegsLength * 0.8f) * ThighCalfRatio / (ThighCalfRatio + 1.f);

			bResetFeet = true;
		} 
	}
}

void UVRIKBody::ResetFootsTimerL_Tick()
{
	if (hResetFootLTimer.IsValid())
	{
		ResetFootLocationL = true;
		OwningPawn->GetWorldTimerManager().ClearTimer(hResetFootLTimer);
		hResetFootLTimer.Invalidate();
	}
}

void UVRIKBody::ResetFootsTimerR_Tick()
{
	if (hResetFootRTimer.IsValid())
	{
		ResetFootLocationR = true;
		OwningPawn->GetWorldTimerManager().ClearTimer(hResetFootRTimer);
		hResetFootRTimer.Invalidate();
	}
}

void UVRIKBody::GetCorrelationKoef(int32 StartIndex)
{
	const int num = CORR_POINTS_NUM;
	int i, index, index0, q = 0;
	if (StartIndex < 0) StartIndex = CurrentVRInputIndex;

	FVector th, tl, tr;
	FRotator dH, dR, dL;
	// load current values to the data arrays
	// difference of values is proportionate to velocity (because timer ticks on approximately static interval)
	// So division to delta [t] isn't necessary
	for (i = StartIndex - num; i <= StartIndex; i++, q++) {
		index = (i > 0) ? i : i + CORR_SAVE_POINTS_NUM;
		index0 = (i > 1) ? i - 1 : CORR_SAVE_POINTS_NUM - 1;

		// Rotators of Head, HandR, HandL
		dH = VRInputData[index].HeadRot - VRInputData[index0].HeadRot;
		dR = VRInputData[index].RightHandRot - VRInputData[index0].RightHandRot;
		dL = VRInputData[index].LeftHandRot - VRInputData[index0].LeftHandRot;
		cor_rot_a[0][q] = dH.Pitch;									cor_rot_a[1][q] = dH.Yaw;									cor_rot_a[2][q] = dH.Roll;
		cor_rot_b[0][q] = dR.Pitch;									cor_rot_b[1][q] = dR.Yaw;									cor_rot_b[2][q] = dR.Roll;
		cor_rot_c[0][q] = dL.Pitch;									cor_rot_c[1][q] = dL.Yaw;									cor_rot_c[2][q] = dL.Roll;

		// Locations of Head, HandR, HandL
		th = VRInputData[index].HeadLoc - VRInputData[index0].HeadLoc;
		tr = VRInputData[index].RightHandLoc - VRInputData[index0].RightHandLoc;
		tl = VRInputData[index].LeftHandLoc - VRInputData[index0].LeftHandLoc;
		cor_loc_a[0][q] = th.X;										cor_loc_a[1][q] = th.Y;										cor_loc_a[2][q] = th.Z;
		cor_loc_b[0][q] = tr.X;										cor_loc_b[1][q] = tr.Y;										cor_loc_b[2][q] = tr.Z;
		cor_loc_c[0][q] = tl.X;										cor_loc_c[1][q] = tl.Y;										cor_loc_c[2][q] = tl.Z;

		// RotationVectors of Head, HandR, HandL
		th = VRInputData[index].HeadLoc;
		tr = VRInputData[index].RightHandLoc;
		tl = VRInputData[index].LeftHandLoc;
		cor_rotv_a[0][q] = th.X;									cor_rotv_a[1][q] = th.Y;									cor_rotv_a[2][q] = th.Z;
		cor_rotv_b[0][q] = tr.X;									cor_rotv_b[1][q] = tr.Y;									cor_rotv_b[2][q] = tr.Z;
		cor_rotv_c[0][q] = tl.X;									cor_rotv_c[1][q] = tl.Y;									cor_rotv_c[2][q] = tl.Z;

	}
}

inline float UVRIKBody::GetAngleToInterp(const float Current, const float Target)
{
	float fRet = Current;
	if (FMath::Abs(Current - Target) > 180.0f) {
		if (Current < Target) fRet += 360.0f; else fRet -= 360.0f;
	}
	return fRet;
}

inline float UVRIKBody::GetFloorCoord()
{
	if (UseActorLocationAsFloor) {
		if (IsValid(FloorBaseComp))
		{
			return FloorBaseComp->GetComponentLocation().Z;
		}
		else
		{
			UE_LOG(LogVRIKBody, Log, TEXT("Can't get ground level: root component isn't initalized."));
			return 0.0f;
		}
	}
	else {
		return TracedFloorLevel;
	}
}

// calc shoulder pitch and yaw offsets
void UVRIKBody::CalcShouldersWithOffset(EControllerHand Hand)
{
	const float ShoulderOffsetYawP = 20.f;
	const float ShoulderOffsetYawN = 20.f;
	const float ShoulderOffsetPitchP = 20.f;
	const float ShoulderOffsetPitchN = 20.f;

	FTransform Shoulder = SkeletonTransformData.Ribcage;
	FRotator ShoulderRot = Shoulder.Rotator();
	FVector ShoulderEnd, HandLoc;

	// init side values
	if (Hand == EControllerHand::Right) {
		IKB_AddRelativeRotation(ShoulderRot, FRotator(0.f, 90.f, 0.f));
		ShoulderEnd = SkeletonTransformData.Ribcage.GetTranslation() + SkeletonTransformData.Ribcage.GetRotation().GetRightVector() * (BodyWidth * 0.5f);
		HandLoc = SkeletonTransformData.HandRight.GetTranslation();
	}
	else {
		IKB_AddRelativeRotation(ShoulderRot, FRotator(0.f, -90.f, 0.f));
		ShoulderEnd = SkeletonTransformData.Ribcage.GetTranslation() - SkeletonTransformData.Ribcage.GetRotation().GetRightVector() * (BodyWidth * 0.5f);
		HandLoc = SkeletonTransformData.HandLeft.GetTranslation();
	}
	FTransform relhand = UKismetMathLibrary::ConvertTransformToRelative(FTransform(SkeletonTransformData.Ribcage.GetRotation(), ShoulderEnd), FTransform(HandLoc));

	// get coef to know how
	float ShoulderShiftKoefX = relhand.GetTranslation().X / HandLength;
	float ShoulderShiftKoefY = -relhand.GetTranslation().Y / HandLength;
	float ShoulderShiftKoefZ = relhand.GetTranslation().Z / HandLength;
	ShoulderShiftKoefX = FMath::Clamp(ShoulderShiftKoefX, -1.f, 1.f);
	ShoulderShiftKoefY = FMath::Clamp(ShoulderShiftKoefY, -1.f, 1.f);
	ShoulderShiftKoefZ = FMath::Clamp(ShoulderShiftKoefZ, -1.f, 1.f);

	float a = ShoulderShiftKoefX > 0.f ? ShoulderOffsetYawP : ShoulderOffsetYawN;
	float b = ShoulderShiftKoefZ > 0.f ? ShoulderOffsetPitchP : ShoulderOffsetPitchN;

	if (!LockShouldersRotation) {
		if (Hand == EControllerHand::Right) {
			SkeletonTransformData.CollarboneRight = FRotator(ShoulderShiftKoefZ * b, -ShoulderShiftKoefX * a, 0.f);
			IKB_AddRelativeRotation(ShoulderRot, SkeletonTransformData.CollarboneRight);
		}
		else {
			SkeletonTransformData.CollarboneLeft = FRotator(ShoulderShiftKoefZ * b, ShoulderShiftKoefX * a, 0.f);
			IKB_AddRelativeRotation(ShoulderRot, SkeletonTransformData.CollarboneLeft);
		}
		Shoulder.SetRotation(ShoulderRot.Quaternion());
		Shoulder.SetTranslation(SkeletonTransformData.Ribcage.GetTranslation() + ShoulderRot.Quaternion().GetForwardVector() * BodyWidth * 0.5f);
	}
	else {
		Shoulder.SetRotation(ShoulderRot.Quaternion());
		Shoulder.SetTranslation(ShoulderEnd);
	}

	// 5) update data struct
	if (Hand == EControllerHand::Right) {
		ShoulderYawOffsetRight = ShoulderShiftKoefX;
		ShoulderPitchOffsetRight = ShoulderShiftKoefY;
		SkeletonTransformData.UpperarmRight.SetTranslation(Shoulder.GetTranslation());
	}
	else {
		ShoulderYawOffsetLeft = ShoulderShiftKoefX;
		ShoulderPitchOffsetLeft = ShoulderShiftKoefY;
		SkeletonTransformData.UpperarmLeft.SetTranslation(Shoulder.GetTranslation());
	}
}

/*
void ClampElbowRotation(const FVector InternalSide, const FVector UpSide, const FVector HandUpVector, FVector& CurrentAngle, bool bDebug)
{
	// check limits
	FQuat LimAngleA;
	FVector LimAngle;

	if (FVector::DotProduct(InternalSide, HandUpVector) < -0.4f && FVector::DotProduct(UpSide, HandUpVector) > -.4f) {
		// elbow is inside
		LimAngleA = FQuat::FastLerp(
			FRotationMatrix::MakeFromX(InternalSide).ToQuat(),
			FRotationMatrix::MakeFromX(UpSide * -1.f).ToQuat(),
			0.35f);
		LimAngleA.Normalize();
		LimAngle = LimAngleA.GetForwardVector();

		float cos1 = FVector::DotProduct(UpSide, LimAngle);
		float cos2 = FVector::DotProduct(UpSide, CurrentAngle);
		if (cos1 < cos2) {
			CurrentAngle = FMath::Lerp(CurrentAngle, LimAngle, (1.f - cos2 / cos1));
		}
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, "internal");
	}
	else if (FVector::DotProduct(InternalSide, HandUpVector) > -.4f && FVector::DotProduct(UpSide, HandUpVector) < -0.4f) {
		// elbow is outside
		LimAngleA = FQuat::FastLerp(
			FRotationMatrix::MakeFromX(InternalSide * -1.f).ToQuat(),
			FRotationMatrix::MakeFromX(UpSide).ToQuat(),
			0.5f);
		LimAngleA.Normalize();
		LimAngle = LimAngleA.GetForwardVector();

		float cos1 = FVector::DotProduct(UpSide, LimAngle);
		float cos2 = FVector::DotProduct(UpSide, CurrentAngle);
		if (cos1 < cos2) {
			CurrentAngle = FMath::Lerp(CurrentAngle, LimAngle, ((cos2 / cos1) - 1.f) * 1.8f);
		}
		if (bDebug) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, "external");
	}
}*/


// Calculate elbow joint target, upperbone and lowerbone hand transforms
void UVRIKBody::CalcHandIKTransforms(EControllerHand Hand)
{
	const float side = (Hand == EControllerHand::Right) ? 1.f : -1.f;

	// elbow-wrist twist limits
	const float SotfAngleLimit = 0.f;// 30.f;
	const float HardAngleLimit = 90.f;

	FVector JointTarget, ShoulderEnd, HandLoc;
	FRotator HandRot;
	FQuat HandQ;
	float ShoulderShiftKoefX, ShoulderShiftKoefY;
	float ForeSize = HandLength - UpperarmLength;
	float* PreviousElbowRoll;

	if (Hand == EControllerHand::Right) {
		ShoulderEnd = SkeletonTransformData.UpperarmRight.GetTranslation();
		ShoulderShiftKoefY = ShoulderPitchOffsetRight;
		ShoulderShiftKoefX = ShoulderYawOffsetRight;
		HandLoc = SkeletonTransformData.HandRight.GetTranslation();

		HandQ = SkeletonTransformData.HandRight.GetRotation();
		HandRot = UKismetMathLibrary::MakeRotFromXZ(HandQ.GetForwardVector(), HandQ.GetRightVector());
		PreviousElbowRoll = &PreviousElbowRollRight;
	}
	else {
		ShoulderEnd = SkeletonTransformData.UpperarmLeft.GetTranslation();
		ShoulderShiftKoefY = ShoulderPitchOffsetLeft;
		ShoulderShiftKoefX = ShoulderYawOffsetLeft;
		HandLoc = SkeletonTransformData.HandLeft.GetTranslation();
		HandRot = SkeletonTransformData.HandLeft.Rotator();

		HandQ = SkeletonTransformData.HandLeft.GetRotation();
		HandRot = UKismetMathLibrary::MakeRotFromXZ(HandQ.GetForwardVector(), -HandQ.GetRightVector());
		PreviousElbowRoll = &PreviousElbowRollLeft;
	}

	// correct hand length
	float DirectSize = FVector::Dist(ShoulderEnd, HandLoc);
	if (DirectSize > HandLength) {
		DirectSize = HandLength;
		const FVector HandDir = (HandLoc - ShoulderEnd).GetSafeNormal();
		HandLoc = ShoulderEnd + HandDir * DirectSize;
	}

	// elbow joint target: relative to ribcage
	////const FVector jf_l = FVector(   0.f,  80.f * side, -60.f);
	const FVector jf_l = FVector(-40.f, 80.f * side, -60.f);

	const FVector jf_r = FVector( -90.f, 100.f * side, -20.f);
	const FVector jb   = FVector(-300.f, 120.f * side,  10.f);
	JointTarget = FMath::Lerp(jf_l, jf_r, FMath::Clamp(FMath::Abs(ShoulderShiftKoefY * 1.2f), 0.f, 1.f));
	JointTarget = FMath::Lerp(JointTarget, jb, FMath::Clamp(-ShoulderShiftKoefX, 0.f, 1.f));

	// convert joint target to world space
	FTransform tr = FTransform(JointTarget) * SkeletonTransformData.Ribcage;
	JointTarget = tr.GetTranslation();

	// twist correction
	const FVector XVec = (HandLoc - ShoulderEnd).GetSafeNormal();
	FVector YVec = (JointTarget - ShoulderEnd).GetSafeNormal() * side;
	//FVector UpVec = XVec ^ YVec; UpVec.Normalize();
	const FRotator ArmRot = UKismetMathLibrary::MakeRotFromXY(XVec, YVec);
	// calculated twist
	const FRotator InitialDelta = UKismetMathLibrary::NormalizedDeltaRotator(HandRot, ArmRot);
	FRotator NewDelta = InitialDelta;

	const float AbsCurrRoll = FMath::Abs(InitialDelta.Roll);
	if (AbsCurrRoll > SotfAngleLimit)
	{
		float RollDecrease;

		float DecreaseAlpha = (AbsCurrRoll - SotfAngleLimit) / (HardAngleLimit - SotfAngleLimit);
		DecreaseAlpha = FMath::Clamp(DecreaseAlpha, 0.f, 1.f);
		RollDecrease = (AbsCurrRoll - SotfAngleLimit) * DecreaseAlpha * 0.5f;

		bool bUtmostCorrection = false;
		if (AbsCurrRoll - RollDecrease > HardAngleLimit)
		{
			bUtmostCorrection = true;
			RollDecrease = AbsCurrRoll - HardAngleLimit;
		}

		// combine rotation-based and body-based joint targets
		const FVector InitialDirection = ArmRot.Quaternion().GetRightVector() * side;
		const FVector HandDirection = HandQ.GetUpVector() * -1.f;
		const FVector JointDirection = FMath::Lerp(InitialDirection, HandDirection, (DecreaseAlpha * DecreaseAlpha) * 0.4f).GetSafeNormal();
		
		JointTarget = JointDirection * 50.f + (HandLoc + ShoulderEnd) * 0.5f;

		YVec = (JointTarget - ShoulderEnd).GetSafeNormal() * side;
		const FRotator NewArmRot = UKismetMathLibrary::MakeRotFromXY(XVec, YVec);
		NewDelta = UKismetMathLibrary::NormalizedDeltaRotator(HandRot, NewArmRot);
	}

	// and now usual two-bones hand calculation
	FTransform UpperarmTransform, ForearmTransform;
	CalculateTwoBoneIK(ShoulderEnd, HandLoc, JointTarget, UpperarmLength, ForeSize, UpperarmTransform, ForearmTransform, side);

	// 5) update data struct
	if (Hand == EControllerHand::Right) {
		SkeletonTransformData.UpperarmRight = UpperarmTransform;
		SkeletonTransformData.ForearmRight = ForearmTransform;
		SkeletonTransformData.ElbowJointTargetRight = JointTarget;
		SkeletonTransformData.LowerarmTwistRight = NewDelta.Roll;
	}
	else {
		SkeletonTransformData.UpperarmLeft = UpperarmTransform;
		SkeletonTransformData.ForearmLeft = ForearmTransform;
		SkeletonTransformData.ElbowJointTargetLeft = JointTarget;
		SkeletonTransformData.LowerarmTwistLeft = NewDelta.Roll;
	}
}

bool UVRIKBody::CalibrateBodyAtTPose()
{
	if (!bIsInitialized) return false;

	CalT_Head = GetHMDTransform();
	CalT_ControllerR = GetHandTransform(EControllerHand::Right);
	CalT_ControllerL = GetHandTransform(EControllerHand::Left);

	bCalibratedT = true;

	if (bCalibratedT && bCalibratedI) {
		CalibrateSkeleton();
		return true;
	}
	else return false;
}

bool UVRIKBody::CalibrateBodyAtIPose()
{
	if (!bIsInitialized) return false;

	CalI_Head = GetHMDTransform();
	CalI_ControllerR = GetHandTransform(EControllerHand::Right);
	CalI_ControllerL = GetHandTransform(EControllerHand::Left);

	bCalibratedI = true;

	if (bCalibratedT && bCalibratedI) {
		CalibrateSkeleton();
		return true;
	}
	else return false;
}

bool UVRIKBody::AutoCalibrateBodyAtPose()
{
	FVector head, handr, handl;
	head = GetHMDTransform().GetTranslation();
	handr = GetHandTransform(EControllerHand::Right).GetTranslation();
	handl = GetHandTransform(EControllerHand::Left).GetTranslation();

	if (FVector::Dist(handr, handl) > 110.f && head.Z - handr.Z < 50.f && head.Z - handl.Z < 50.f) {
		if (FMath::Abs(handr.Z - handl.Z) < 30.f) {
			return CalibrateBodyAtTPose();
		}
		else {
			return false;
		}
	}
	else {
		return CalibrateBodyAtIPose();
	}
}

bool UVRIKBody::ActivateInput(USceneComponent* RootComp)
{
	if (VRInputOption == EVRInputSetup::InputFromComponents && !IsValid(CameraComponent)) {
		if (!IsValid(FloorBaseComp))
		{
			FloorBaseComp = IsValid(RootComp) ? RootComp : OwningPawn->GetRootComponent();
		}
		return false;
	}
	else {
		// init input components
		if (!bIsInitialized) {
			SkeletonTransformData.Pelvis = GetHMDTransform();
			vPrevCamLocation = SkeletonTransformData.Pelvis.GetTranslation();
			rPrevCamRotator = SkeletonTransformData.Pelvis.Rotator();
		}

		// timer to correlate collected data
		if (!hVRInputTimer.IsValid()) {
			OwningPawn->GetWorldTimerManager().SetTimer(hVRInputTimer, this, &UVRIKBody::VRInputTimer_Tick, TimerInterval, true);
		}
		// timer to fix YAW ribcage-head mismatch
		if (!hTorsoYawTimer.IsValid()) {
			OwningPawn->GetWorldTimerManager().SetTimer(hTorsoYawTimer, this, &UVRIKBody::RibcageYawTimer_Tick, 1.f, true);
		}

		// init IMotionController interface pointer
		TArray<IMotionController*> controllers = IModularFeatures::Get().GetModularFeatureImplementations<IMotionController>(IMotionController::GetModularFeatureName());
		if (controllers.Num() > 0) MControllers = controllers[0];

		// init floor component
		if (IsValid(RootComp))
		{
			FloorBaseComp = RootComp;
		}
		else
		{
			FloorBaseComp = OwningPawn->GetRootComponent();
		}
		if (!bIsInitialized) PrevFrameActorRot = FloorBaseComp->GetComponentRotation();

		// finish
		bIsInitialized = (VRInputOption == EVRInputSetup::InputFromComponents || controllers.Num() > 0);
		return bIsInitialized;
	}
}

void UVRIKBody::DeactivateInput()
{
	if (bIsInitialized) {
		// timer to correlate collected data
		OwningPawn->GetWorldTimerManager().ClearTimer(hVRInputTimer);
		hVRInputTimer.Invalidate();

		OwningPawn->GetWorldTimerManager().ClearTimer(hTorsoYawTimer);
		hTorsoYawTimer.Invalidate();

		// finish
		bIsInitialized = false;
	}
}

void UVRIKBody::CalibrateSkeleton()
{
	nModifyHeightState = HEIGHT_STABLE;

	const float HeadPitch = FMath::Clamp((CalI_Head.Rotator().Pitch + CalT_Head.Rotator().Pitch) / 2.f, -90.f, 0.f);
	const float GroundZ = GetFloorCoord();

	BodyWidth = FVector::Dist(CalI_ControllerR.GetTranslation(), CalI_ControllerL.GetTranslation()) - 5.f;
	CharacterHeight = FMath::Max(CalI_Head.GetTranslation().Z, CalT_Head.GetTranslation().Z) + 3.f - GroundZ - FMath::Sin(HeadPitch) * 3.f;
	CharacterHeightClear = FMath::Max(CalI_Head.GetTranslation().Z, CalT_Head.GetTranslation().Z) - GroundZ;
	LegsLength = (CharacterHeight + 5.f) / 2.f;

	FVector armspan = CalT_ControllerR.GetTranslation() - CalT_ControllerL.GetTranslation();
	armspan.Z = 0.f;
	ArmSpanClear = armspan.Size();

	FVector RibcageToHeadsetOffset;
	FVector RibcagePoint = (CalT_ControllerR.GetTranslation() + CalT_ControllerL.GetTranslation()) * 0.5f;
	RibcageToHeadsetOffset = FVector::ZeroVector;
	RibcageToHeadsetOffset.X = (CalT_Head.GetTranslation() - RibcagePoint).Size2D();
	RibcageToHeadsetOffset.Z = CalT_Head.GetTranslation().Z - RibcagePoint.Z;

	//******************************
	NeckToHeadsetOffset = RibcageToHeadsetOffset - RibcageToNeckOffset;

	if (NeckToHeadsetOffset.X < 10.f) NeckToHeadsetOffset.X = 10.f;
	//******************************

	SpineLength = CharacterHeight - LegsLength - RibcageToHeadsetOffset.Z;
	HandLength = (FVector::Dist(CalT_ControllerR.GetTranslation(), CalT_ControllerL.GetTranslation()) - BodyWidth) * 0.5f - 10.f;

	if (bDebugOutput) {
		for (int i = 0; i < 33; i++) {
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "BodyWidth: " + FString::SanitizeFloat(BodyWidth));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "CharacterHeight: " + FString::SanitizeFloat(CharacterHeight));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "LegsLength: " + FString::SanitizeFloat(LegsLength));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "RibcageToHeadsetOffset: " + RibcageToHeadsetOffset.ToString());
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "SpineLength: " + FString::SanitizeFloat(SpineLength));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "HandLength: " + FString::SanitizeFloat(HandLength));
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::White, "RibcageZ: " + FString::SanitizeFloat(RibcagePoint.Z - GroundZ));
		}
	}

	FTransform CamTr = GetHMDTransform();
	FRotator r = CamTr.Rotator();
	SkeletonTransformData.Ribcage.SetTranslation(CamTr.GetTranslation() - CamTr.GetRotation().RotateVector(RibcageToHeadsetOffset));
	r.Pitch = r.Roll = 0.f;
	SkeletonTransformData.Ribcage.SetRotation(r.Quaternion());

	UpperarmLength = HandLength * UpperarmForearmRatio / (UpperarmForearmRatio + 1.f);
	ThighLength = LegsLength * ThighCalfRatio / (ThighCalfRatio + 1.f);

	// Update on server and other clients if in networking
	if (GetIsReplicated()) {
		const FCalibratedBody dat = GetCalibratedBody();
		ServerRestoreCalibratedBody(dat);
	}

	// Broadcast event
	OnCalibrationComplete.Broadcast();

	bResetFeet = true;
}

void UVRIKBody::CalcFeetInstantaneousTransforms(float DeltaTime, float FloorZ)
{
	const float InterpSpeed = 5.f;
	const float StepHeight = 6.f;
	FTransform AniData_TargetR, AniData_TargetL;
	bool NeedMoveR, NeedMoveL, IsMovingR, IsMovingL;
	FVector vR2Target, vL2R;

	// init
	AniData_TargetR = SkeletonTransformData.FootRightTarget;
	AniData_TargetL = SkeletonTransformData.FootLeftTarget;

	IsMovingR = (FVector::DistSquaredXY(FootTargetTransformR.GetTranslation(), FootLastTransformR.GetTranslation()) > 0.5f);
	IsMovingL = (FVector::DistSquaredXY(FootTargetTransformL.GetTranslation(), FootLastTransformL.GetTranslation()) > 0.5f);

	// reset on flag
	if (bResetFeet) {
		bResetFeet = false;
		FootLastTransformR = FootTargetTransformR = AniData_TargetR;
		FootLastTransformL = FootTargetTransformL = AniData_TargetL;
		fDeltaTimeR = fDeltaTimeL = 0.f;
	}

	// if not moving - check if we need to move
	if (!IsMovingL && !IsMovingR) {
		NeedMoveR = (FVector::DistSquaredXY(AniData_TargetR.GetTranslation(), FootLastTransformR.GetTranslation()) > 1.f);
		NeedMoveL = (FVector::DistSquaredXY(AniData_TargetL.GetTranslation(), FootLastTransformL.GetTranslation()) > 1.f);

		// if need move both feet - choose one to start
		if (NeedMoveR && NeedMoveL) {
			vR2Target = AniData_TargetR.GetTranslation() - FootLastTransformR.GetTranslation();
			vL2R = FootLastTransformR.GetTranslation() - FootLastTransformL.GetTranslation();

			if (FVector::DotProduct(vR2Target, vL2R) > 0.f) {
				FootTargetTransformR = AniData_TargetR;
			}
			else {
				FootTargetTransformL = AniData_TargetL;
			}
		}
		// if need move right feet
		else if (NeedMoveR) {
			FootTargetTransformR = AniData_TargetR;
		}
		// if need move left feet
		else if (NeedMoveL) {
			FootTargetTransformL = AniData_TargetL;
		}
	}
	
	// move
	FootTickTransformR = UKismetMathLibrary::TInterpTo(FootLastTransformR, FootTargetTransformR, fDeltaTimeR, InterpSpeed);
	FootTickTransformL = UKismetMathLibrary::TInterpTo(FootLastTransformL, FootTargetTransformL, fDeltaTimeL, InterpSpeed);

	// add z offset
	float k = fDeltaTimeR / (1.f - InterpSpeed);
	if (IsMovingR && k < 0.95f) {
		FVector inc = FootTickTransformR.GetTranslation();
		inc.Z = FloorZ + FMath::Sin(k * PI) * StepHeight;
		FootTickTransformR.SetTranslation(inc);
	}
	k = fDeltaTimeL / (1.f - InterpSpeed);
	if (IsMovingL && k < 0.95f) {
		FVector inc = FootTickTransformL.GetTranslation(); 
		inc.Z = FloorZ + FMath::Sin(k * PI) * StepHeight;
		FootTickTransformL.SetTranslation(inc);
	}

	// finish move
	if (FVector::DistSquaredXY(FootTargetTransformR.GetTranslation(), FootTickTransformR.GetTranslation()) < 0.7f) {
		FootLastTransformR = FootTargetTransformR;
		fDeltaTimeR = 0.f;
	}
	if (FVector::DistSquaredXY(FootTargetTransformL.GetTranslation(), FootTickTransformL.GetTranslation()) < 0.7f) {
		FootLastTransformL = FootTargetTransformL;
		fDeltaTimeL = 0.f;
	}

	// increment step phase
	if (IsMovingR) fDeltaTimeR += DeltaTime;
	if (IsMovingL) fDeltaTimeL += DeltaTime;

	// set values
	SkeletonTransformData.FootRightCurrent = FootTickTransformR;
	SkeletonTransformData.FootLeftCurrent = FootTickTransformL;
}

// Parameters are in world space
FORCEINLINE EBodyOrientation UVRIKBody::ComputeCurrentBodyOrientation(const FTransform& Head, const FTransform& HandRight, const FTransform& HandLeft)
{
	if (!IsValid(FloorBaseComp))
	{
		UE_LOG(LogVRIKBody, Warning, TEXT("ComputeCurrentBodyOrientation(): Component initialization failed."));
		return EBodyOrientation::Stand;
	}
	
	if (bManualBodyOrientation)
	{
		return SkeletonTransformData.BodyOrientation;
	}

	const float ZBase = FloorBaseComp->GetComponentLocation().Z;
	const float HeadOffset = Head.GetTranslation().Z - ZBase;
	const float HandROffset = HandRight.GetTranslation().Z - ZBase;
	const float HandLOffset = HandLeft.GetTranslation().Z - ZBase;

	/*
	const FTransform BaseTransform = FloorBaseComp->GetComponentTransform();
	const float HeadOffset = Head.GetRelativeTransform(BaseTransform).GetTranslation().Z;
	const float HandROffset = HandRight.GetRelativeTransform(BaseTransform).GetTranslation().Z;
	const float HandLOffset = HandLeft.GetRelativeTransform(BaseTransform).GetTranslation().Z;
	*/

	const float CrawlHeightLim = CharacterHeight * 0.5f;
	const float LieHeightLim = CharacterHeight * 0.2f;

	// 1 is lie, 2 is crawl, 3 is stand/sit
	uint8 HeadState = 3;
	if (HeadOffset < LieHeightLim) HeadState = 1; else if (HeadOffset < CrawlHeightLim) HeadState = 2;

	// false is lie, true is not
	const bool HandRState = (HandROffset > LieHeightLim);
	const bool HandLState = (HandLOffset > LieHeightLim);

	EBodyOrientation RetVal = SkeletonTransformData.BodyOrientation;

	switch (RetVal)
	{
		// for sitting and standing
		// change state by head and hand distance from floor
		case EBodyOrientation::Stand:
		case EBodyOrientation::Sit:
			if (HeadState == 2 && !(HandRState || HandRState))
			{
				RetVal = EBodyOrientation::Crawl;
			}
			else if (HeadState == 1)
			{
				// is looking up?
				if (FVector::DotProduct(Head.GetRotation().GetForwardVector(), FloorBaseComp->GetComponentQuat().GetUpVector()) > 0.f) {
					const FQuat q = SkeletonTransformData.Pelvis.GetRotation();
					SavedTorsoDirection = GetForwardDirection(q.GetForwardVector(), q.GetUpVector());
					RetVal = EBodyOrientation::LieDown_FaceUp;
				}
				else {
					RetVal = EBodyOrientation::LieDown_FaceDown;
				}
			}
			else
			{
				RetVal = SkeletonTransformData.IsSitting ? EBodyOrientation::Sit : EBodyOrientation::Stand;
			}
			break;

		case EBodyOrientation::Crawl:
			// return to sitting if head and both hands are above limit
			if ((HeadState == 3 || HeadOffset > CharacterHeight * 0.35f) && HandRState && HandLState)
			{
				RetVal = EBodyOrientation::Stand;
				bResetTorso = true;
			}
			// check for laying down
			else if (HeadState == 1)
			{
				// is looking up?
				if (FVector::DotProduct(Head.GetRotation().GetForwardVector(), FloorBaseComp->GetComponentQuat().GetUpVector()) > 0.f) {
					const FQuat q = SkeletonTransformData.Pelvis.GetRotation();
					SavedTorsoDirection = GetForwardDirection(q.GetForwardVector(), q.GetUpVector());
					RetVal = EBodyOrientation::LieDown_FaceUp;
				}
				else {
					RetVal = EBodyOrientation::LieDown_FaceDown;
				}
			}
			break;

		case EBodyOrientation::LieDown_FaceDown:
			if (HeadState == 3)
			{
				RetVal = SkeletonTransformData.IsSitting ? EBodyOrientation::Sit : EBodyOrientation::Stand;
				bResetTorso = true;
			}
			else if (HeadState == 2)
			{
				RetVal = EBodyOrientation::Crawl;
			}
			break;

		case EBodyOrientation::LieDown_FaceUp:
			if (HeadState > 1 && HandRState && HandLState)
			{
				RetVal = SkeletonTransformData.IsSitting ? EBodyOrientation::Sit : EBodyOrientation::Stand;
				bResetTorso = true;
			}
			break;
	}

	return RetVal;
}

void UVRIKBody::RibcageYawTimer_Tick()
{
	if (FMath::Abs(SkeletonTransformData.Head.Rotator().Yaw - SkeletonTransformData.Ribcage.Rotator().Yaw) > 20.f) {
		// calculate head-hands rotation
		
		// hands centre location
		const FVector HandCentre = (GetHandTransform(EControllerHand::Right).GetTranslation() + GetHandTransform(EControllerHand::Left).GetTranslation()) / 2.f;
		const FTransform HeadT = GetHMDTransform();
		FVector H2H = HeadT.GetTranslation() - HandCentre;
		H2H.Normalize();

		
		// only if head-to-hands angle is less then hip-to-hands angle
		if (FMath::Abs(FVector::DotProduct(SkeletonTransformData.Head.GetRotation().GetForwardVector(), H2H)) > FMath::Abs(FVector::DotProduct(SkeletonTransformData.Ribcage.GetRotation().GetForwardVector(), H2H))) {
			if (nYawControlCounter < 4) nYawControlCounter++;

			if (nYawControlCounter > 3)
			{
				if (DetectContinuousHeadYawRotation)
				{
					const FVector hand_r = SkeletonTransformData.HandRight.GetRotation().GetForwardVector();
					const FVector hand_l = SkeletonTransformData.HandLeft.GetRotation().GetForwardVector();
					FVector hands_ = FMath::Lerp(hand_r, hand_l, 0.5f);
					FVector head = SkeletonTransformData.Head.GetRotation().GetForwardVector();

					head.Z = 0.f; head.Normalize();
					hands_.Z = 0.f; hands_.Normalize();

					if (FVector::DotProduct(head, hands_) > 0.8f)
					{
						nYawControlCounter = 0;
						bYawInterpToHead = true;
					}
				}
				else
				{
					nYawControlCounter = 0;
					bYawInterpToHead = true;
				}
			}
		}
		// otherwise - keep watching
	}
	else {
		nYawControlCounter = 0;
	}
}

void UVRIKBody::TraceFloor(const FVector HeadLocation)
{
	FCollisionQueryParams stTraceParams;
	stTraceParams = FCollisionQueryParams(FName(TEXT("IKBody_FloorCollision")), true, OwningPawn);
	stTraceParams.bTraceComplex = true;
	stTraceParams.bTraceAsyncScene = true;
	stTraceParams.bReturnPhysicalMaterial = false;

	FHitResult HitOutSimpleR, HitOutSimpleL;
	const FVector Offset = SkeletonTransformData.Pelvis.GetRotation().GetRightVector() * BodyWidth * 0.5f;
	const FVector FloorDirection = FVector(0.f, 0.f, 1000.f);
	if (TraceFloorByObjectType) {
		GetWorld()->LineTraceSingleByObjectType(HitOutSimpleR, HeadLocation + Offset, HeadLocation + Offset - FloorDirection, FCollisionObjectQueryParams(FloorCollisionObjectType), stTraceParams);
		GetWorld()->LineTraceSingleByObjectType(HitOutSimpleL, HeadLocation - Offset, HeadLocation - Offset - FloorDirection, FCollisionObjectQueryParams(FloorCollisionObjectType), stTraceParams);
	}
	else {
		GetWorld()->LineTraceSingleByChannel(HitOutSimpleR, HeadLocation + Offset, HeadLocation + Offset - FloorDirection, FloorCollisionObjectType, stTraceParams);
		GetWorld()->LineTraceSingleByChannel(HitOutSimpleL, HeadLocation - Offset, HeadLocation - Offset - FloorDirection, FloorCollisionObjectType, stTraceParams);
	}

	if (!IsValid(FloorBaseComp) || (HitOutSimpleR.bBlockingHit && HitOutSimpleL.bBlockingHit)) {
		TracedFloorLevelR = HitOutSimpleR.ImpactPoint.Z;
		TracedFloorLevelL = HitOutSimpleL.ImpactPoint.Z;
		TracedFloorLevel = (TracedFloorLevelR + TracedFloorLevelL) / 2.f;
	}
	else {
		TracedFloorLevelR = TracedFloorLevelL = TracedFloorLevel = FloorBaseComp->GetComponentLocation().Z;
	}
}

inline FTransform UVRIKBody::GetHMDTransform(bool bRelative)
{
	if (!bIsInitialized) return FTransform::Identity;
	if (!IsValid(FloorBaseComp))
	{
		return FTransform::Identity;
	}
	FTransform BaseTransform = FloorBaseComp->GetComponentTransform();
	if (bDoPawnOrientationAdjustment)
	{
		BaseTransform.SetRotation(FQuat::Identity);
	}

	// component is replicated and owning pawn doesn't have controller, so it's a copy of a real pawn and have to use input from remote PC
	const bool IsRemoteInNetwork = GetIsReplicated() && !LOCALLY_CONTROLLED;

	if (IsRemoteInNetwork) {
		return bRelative ? InputHMD : (InputHMD * BaseTransform);
	}
	else if (VRInputOption == EVRInputSetup::InputFromVariables)
	{
		return bRelative ? UKismetMathLibrary::ConvertTransformToRelative(BaseTransform, InputHMD) : InputHMD;
	}
	else if (VRInputOption == EVRInputSetup::InputFromComponents)
	{
		if (!IsValid(CameraComponent)) {
			return FTransform();
		}
		else {
			if (bRelative)
				return CameraComponent->GetRelativeTransform();
			else if (bDoPawnOrientationAdjustment)
				return CameraComponent->GetRelativeTransform() * BaseTransform;
			else
				return CameraComponent->GetComponentTransform();
		}
	}
	else if (VRInputOption == EVRInputSetup::DirectVRInput && IsValid(OwningPawn))
	{
		FVector loc;
		FRotator rot;

		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(rot, loc);

		if (FollowPawnTransform && !bRelative)
			return FTransform(rot, loc, (FVector)1) * BaseTransform;
		else
			return FTransform(rot, loc, (FVector)1);
	}
	else {
		return FTransform();
	}
}

inline FTransform UVRIKBody::GetHandTransform(EControllerHand Hand, bool UseDefaults, bool PureTransform, bool bRelative)
{
	FTransform ret;
	if (!bIsInitialized) return ret;
	if (!IsValid(FloorBaseComp))
	{
		return FTransform::Identity;
	}
	FTransform BaseTransform = FloorBaseComp->GetComponentTransform();
	if (bDoPawnOrientationAdjustment)
	{
		BaseTransform.SetRotation(FQuat::Identity);
	}

	bool HandTracked = true;
	const bool IsRemoteInNetwork = GetIsReplicated() && !LOCALLY_CONTROLLED;

	// attached hand transform
	if (false == PureTransform && ((Hand == EControllerHand::Right && HandAttachedRight) || (Hand == EControllerHand::Left && HandAttachedLeft))) {
		UPrimitiveComponent* comp;
		FTransform rel_tr;
		FName socket;
		if (Hand == EControllerHand::Right) {
			comp = HandParentRight; rel_tr = HandAttachTransformRight; socket = HandAttachSocketRight;
		}
		else {
			comp = HandParentLeft; rel_tr = HandAttachTransformLeft; socket = HandAttachSocketLeft;
		}

		if (socket.ToString().Len() > 0) {
			ret = comp->GetSocketTransform(socket);
		}
		else {
			ret = comp->GetComponentTransform();
		}
		if (!rel_tr.EqualsNoScale(FTransform::Identity)) {
			ret = rel_tr * ret;
		}
	}
	// motion controller hand transform
	else {
		if (IsRemoteInNetwork) {
			if (Hand == EControllerHand::Left) {
				ret = bRelative ? InputHandLeft : (InputHandLeft * BaseTransform);
			}
			else {
				ret = bRelative ? InputHandRight : (InputHandRight * BaseTransform);
			}
		}
		else if (VRInputOption == EVRInputSetup::InputFromVariables)
		{
			if (Hand == EControllerHand::Left) {
				ret = bRelative ? UKismetMathLibrary::ConvertTransformToRelative(BaseTransform, InputHandLeft) : InputHandLeft;
			}
			else {
				ret = bRelative ? UKismetMathLibrary::ConvertTransformToRelative(BaseTransform, InputHandRight) : InputHandRight;
			}
		}
		else if (VRInputOption == EVRInputSetup::InputFromComponents)
		{
			if (!IsValid(LeftHandController) || !IsValid(RightHandController)) {
			}
			else {
				if (!bRelative && bDoPawnOrientationAdjustment)
				{
					if (Hand == EControllerHand::Left) {
						ret = LeftHandController->GetRelativeTransform() * BaseTransform;
					}
					else {
						ret = RightHandController->GetRelativeTransform() * BaseTransform;
					}
				}
				else
				{
					if (Hand == EControllerHand::Left) {
						ret = bRelative ? LeftHandController->GetRelativeTransform() : LeftHandController->GetComponentTransform();
					}
					else {
						ret = bRelative ? RightHandController->GetRelativeTransform() : RightHandController->GetComponentTransform();
					}
				}
			}
		}
		else if (VRInputOption == EVRInputSetup::DirectVRInput && (IsValid(OwningPawn)))
		{
			FVector loc;
			FRotator rot;
			
#if VERSION_ENGINE_MINOR >= 19
			const FName HandSource = (Hand == EControllerHand::Right) ? FName(TEXT("Right")) : FName(TEXT("Left"));
			MControllers->GetControllerOrientationAndPosition(0, HandSource, rot, loc, 100.f);
			HandTracked = (MControllers->GetControllerTrackingStatus(0, HandSource) == ETrackingStatus::Tracked);
#else
			MControllers->GetControllerOrientationAndPosition(0, Hand, rot, loc, 100.f);
			HandTracked = (MControllers->GetControllerTrackingStatus(0, Hand) == ETrackingStatus::Tracked);
#endif

			if (FollowPawnTransform && !bRelative)
				ret = FTransform(rot, loc, (FVector)1) * BaseTransform;
			else
				ret = FTransform(rot, loc, (FVector)1);
		}
		else {
			ret = FTransform();
		}
	}

	// if have no valid transform - use default value
	if (UseDefaults && (!HandTracked || ret.GetTranslation() == FVector::ZeroVector || FVector::DistSquared(ret.GetTranslation(), SkeletonTransformData.Head.GetTranslation()) > (150.f*150.f))) {
		if (Hand == EControllerHand::Right) {
			ret.SetTranslation(FVector(30.f, 25.f, -70.f));
		}
		else {
			ret.SetTranslation(FVector(30.f, -25.f, -70.f));
		}
		ret *= SkeletonTransformData.Head;

		ret.SetRotation(SkeletonTransformData.Head.GetRotation());
	}

	return ret;
}

void UVRIKBody::CalcFeetIKTransforms2(float DeltaTime, float FloorZ)
{
	const float MinSpeed = 50.f;
	const float MaxSpeed = 100.f;
	const float Speed = SkeletonTransformData.Velocity.Size2D();
	const float AnimationSpeed = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f) * 8.f;
	const float StepLength = 35.f;
	const float FeetDist = -(CHECKORIENTATION(EBodyOrientation::Crawl) ? CharacterHeight * 0.25f : CharacterHeight * 0.5f);

	FTransform lfoot, rfoot, lfoot_rel, rfoot_rel;
	FTransform pelvis_base = FTransform::Identity;
	const float PelvisZ = SkeletonTransformData.Pelvis.GetTranslation().Z - FloorZ;
	FVector v = SkeletonTransformData.Pelvis.GetTranslation(); v.Z = FloorZ;
	pelvis_base.SetTranslation(v);

	if (SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl) {
		FRotator r = FRotator::ZeroRotator; r.Yaw = SkeletonTransformData.Pelvis.Rotator().Yaw;
		pelvis_base.SetRotation(r.Quaternion());
	}
	else {
		pelvis_base.SetRotation(UKismetMathLibrary::MakeRotFromX(SkeletonTransformData.Pelvis.GetRotation().GetUpVector().GetSafeNormal2D()).Quaternion());
	}

	const float CurrentMinSpeed = PelvisZ > LegsLength * 0.5f ? MinSpeed * 1.7f : MinSpeed;

	if (SkeletonTransformData.IsJumping && PelvisZ > LegsLength * 0.8f) {
		rfoot_rel = FTransform(FRotator(-20.0f,  15.f, 0.f).Quaternion(), FVector(0.f, 35.f,  FootOffsetToGround + 20.f));
		lfoot_rel = FTransform(FRotator(-20.0f, -15.f, 0.f).Quaternion(), FVector(0.f, -35.f, FootOffsetToGround + 20.f));

		FVector l = pelvis_base.GetTranslation(); l.Z = SkeletonTransformData.Pelvis.GetTranslation().Z - LegsLength;
		pelvis_base.SetTranslation(l);

		SkeletonTransformData.FootRightTarget = rfoot_rel * pelvis_base;
		SkeletonTransformData.FootLeftTarget = lfoot_rel * pelvis_base;
	}
	// if speed is small, keep static location
	else if (Speed < CurrentMinSpeed) {
		if (SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl) {
			rfoot_rel = FTransform(FRotator(0.0f, 5.f, 0.f).Quaternion(), FVector(15.f, 35.f, FootOffsetToGround));			// right back (normal)
			lfoot_rel = FTransform(FRotator(0.0f, -5.f, 0.f).Quaternion(), FVector(-15.f, -35.f, FootOffsetToGround));		// left front (normal)
		}
		else if (CHECKORIENTATION(EBodyOrientation::Crawl) || CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown)) {
			rfoot_rel = FTransform(FRotator(-80.0f, 5.f, 0.f).Quaternion(), FVector(FeetDist, 25.f, FootOffsetToGround));		// right back (normal)
			lfoot_rel = FTransform(FRotator(-80.0f, -5.f, 0.f).Quaternion(), FVector(FeetDist, -25.f, FootOffsetToGround));		// left front (normal)
		}
		else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceUp)) {
			rfoot_rel = FTransform(FRotator(-80.0f, -5.f, 0.f).Quaternion(), FVector(FeetDist, -35.f, FootOffsetToGround));		// right back (normal)
			lfoot_rel = FTransform(FRotator(-80.0f, 5.f, 0.f).Quaternion(), FVector(FeetDist, 35.f, FootOffsetToGround));		// left front (normal)
		}
		rfoot = rfoot_rel * pelvis_base;
		lfoot = lfoot_rel * pelvis_base;

		if (FVector::DistSquared(SkeletonTransformData.FootRightTarget.GetTranslation(), rfoot.GetTranslation()) > 22.f*22.f) {
			SkeletonTransformData.FootRightTarget = rfoot;
		}
		if (FVector::DistSquared(SkeletonTransformData.FootLeftCurrent.GetTranslation(), lfoot.GetTranslation()) > 22.f*22.f) {
			SkeletonTransformData.FootLeftTarget = lfoot;
		}
		FeetMovingStartedTime = 0.f;
	}
	// if moving, play cycle
	else if (Speed > CurrentMinSpeed && FeetMovingStartedTime < 0.3f) {
		FeetMovingStartedTime += DeltaTime;
	}
	else {
		FVector MovingYaw = SkeletonTransformData.Velocity; MovingYaw.Z = 0.f; MovingYaw.Normalize();
		const FVector rfootoffset = pelvis_base.GetRotation().RotateVector(FVector(-18.217f, 14.193f, FootOffsetToGround));
		const FVector lfootoffset = pelvis_base.GetRotation().RotateVector(FVector(14.42f, -19.3f, FootOffsetToGround));
		float roffset, loffset;

		if (CHECKORIENTATION(EBodyOrientation::Crawl)) {
			pelvis_base.AddToTranslation(FeetDist * pelvis_base.GetRotation().GetForwardVector());
		}
		else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown) || CHECKORIENTATION(EBodyOrientation::LieDown_FaceUp)) {
			pelvis_base.AddToTranslation(FeetDist * pelvis_base.GetRotation().GetForwardVector());
		}
		
		roffset = FMath::Sin(FeetCyclePhase) * StepLength;
		loffset = FMath::Sin(FeetCyclePhase + /*PI*/ 3.3f) * StepLength;

		rfoot_rel.SetTranslation(FVector(MovingYaw.X * roffset, MovingYaw.Y * roffset, 0.f));
		lfoot_rel.SetTranslation(FVector(MovingYaw.X * loffset, MovingYaw.Y * loffset, 0.f));
		
		rfoot = pelvis_base;
		rfoot.AddToTranslation(rfootoffset + rfoot_rel.GetTranslation());
		lfoot = pelvis_base;
		lfoot.AddToTranslation(lfootoffset + lfoot_rel.GetTranslation());
		SkeletonTransformData.FootRightTarget = rfoot;
		SkeletonTransformData.FootLeftTarget = lfoot;

		FeetCyclePhase += (DeltaTime * AnimationSpeed);
	}

	SkeletonTransformData.FootRightCurrent = UKismetMathLibrary::TInterpTo(SkeletonTransformData.FootRightCurrent, SkeletonTransformData.FootRightTarget, DeltaTime, 14.f);
	SkeletonTransformData.FootLeftCurrent = UKismetMathLibrary::TInterpTo(SkeletonTransformData.FootLeftCurrent, SkeletonTransformData.FootLeftTarget, DeltaTime, 14.f);

	// now calc thigh and calf if necessary
	if (ComputeLegsIK) {
		const FVector LegOffset = SkeletonTransformData.Pelvis.GetRotation().GetRightVector() * BodyWidth * 0.5f;
		const float CalfLength = LegsLength - ThighLength;
		FTransform pelvis_base2 = SkeletonTransformData.Pelvis;
		FRotator rot;
		FVector JointRight, JointLeft;

		if (SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl) {
			rot = pelvis_base2.Rotator();
			rot.Pitch = rot.Roll = 0.f;
			pelvis_base2.SetRotation(rot.Quaternion());
		}
		else {
			const FVector forw = pelvis_base2.GetRotation().GetUpVector().GetSafeNormal2D();

			rot = UKismetMathLibrary::MakeRotFromX(forw);
			pelvis_base2.SetRotation(rot.Quaternion());
			pelvis_base2.AddToTranslation(FeetDist * forw);
		}
		// calculate joint target for knees
		GetKneeJointTargetForBase(pelvis_base2, FloorZ, JointRight, JointLeft);

		CalculateTwoBoneIK(
			SkeletonTransformData.Pelvis.GetTranslation() + LegOffset,
			SkeletonTransformData.FootRightCurrent.GetTranslation(),
			JointRight,
			ThighLength,
			CalfLength,
			SkeletonTransformData.ThighRight,
			SkeletonTransformData.CalfRight,
			1.f);

		CalculateTwoBoneIK(
			SkeletonTransformData.Pelvis.GetTranslation() - LegOffset,
			SkeletonTransformData.FootLeftCurrent.GetTranslation(),
			JointLeft,
			ThighLength,
			CalfLength,
			SkeletonTransformData.ThighLeft,
			SkeletonTransformData.CalfLeft,
			1.f);

		// feet rotation
		if (CHECKORIENTATION(EBodyOrientation::Crawl)) {
			SkeletonTransformData.FootRightCurrent.SetRotation((FTransform(FRotator(-80.0f, 5.f, 0.f).Quaternion()) * pelvis_base2).GetRotation());
			SkeletonTransformData.FootLeftCurrent.SetRotation((FTransform(FRotator(-80.0f, -5.f, 0.f).Quaternion()) * pelvis_base2).GetRotation());
		}
		else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown)) {
			SkeletonTransformData.FootRightCurrent.SetRotation((FTransform(FRotator(0.0f, 110.f, -30.f).Quaternion()) * pelvis_base2).GetRotation());
			SkeletonTransformData.FootLeftCurrent.SetRotation((FTransform(FRotator(0.0f, -110.f, 30.f).Quaternion()) * pelvis_base2).GetRotation());
		}
		else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceUp)) {
			SkeletonTransformData.FootRightCurrent.SetRotation((FTransform(FRotator(60.0f, 160.f, 16.f).Quaternion()) * pelvis_base2).GetRotation());
			SkeletonTransformData.FootLeftCurrent.SetRotation((FTransform(FRotator(60.0f, 160.f, -16.f).Quaternion()) * pelvis_base2).GetRotation());
		}
	}
}

void UVRIKBody::GetKneeJointTarget(FVector& RightKneeTarget, FVector& LeftKneeTarget)
{
	FTransform BaseTransform = SkeletonTransformData.Pelvis;
	FRotator rot;
	const float FloorZ = GetFloorCoord();

	if (SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl) {
		rot = BaseTransform.Rotator();
		rot.Pitch = rot.Roll = 0.f;
		BaseTransform.SetRotation(rot.Quaternion());
	}
	else {
		const FVector ForwDir = BaseTransform.GetRotation().GetUpVector().GetSafeNormal2D();
		const float FeetDist = -(CHECKORIENTATION(EBodyOrientation::Crawl) ? CharacterHeight * 0.25f : CharacterHeight * 0.5f);

		rot = UKismetMathLibrary::MakeRotFromX(ForwDir);
		BaseTransform.SetRotation(rot.Quaternion());
		BaseTransform.AddToTranslation(FeetDist * ForwDir);
	}

	GetKneeJointTargetForBase(BaseTransform, FloorZ, RightKneeTarget, LeftKneeTarget);
}

void UVRIKBody::GetFloorRotationAdjustment(FTransform& SimulatedBaseTransform, FTransform& RealBaseTransform)
{
	if (FloorBaseComp)
	{
		SimulatedBaseTransform = RealBaseTransform = FloorBaseComp->GetComponentTransform();
		SimulatedBaseTransform.SetRotation(FQuat::Identity);
	}
	else
	{
		SimulatedBaseTransform = RealBaseTransform = FTransform::Identity;
	}
}

FORCEINLINE bool UVRIKBody::GetKneeJointTargetForBase(const FTransform& BaseTransform, const float GroundZ, FVector& RightJointTarget, FVector& LeftJointTarget)
{
	// joint targets depend on body orientation
	if (SkeletonTransformData.BodyOrientation < EBodyOrientation::Crawl)
	{
		RightJointTarget = (FTransform(FVector(90.f, 15.f, 20.f)) * BaseTransform).GetTranslation();
		LeftJointTarget = (FTransform(FVector(90.f, -15.f, 20.f)) * BaseTransform).GetTranslation();
	}
	else if (CHECKORIENTATION(EBodyOrientation::Crawl))
	{
		RightJointTarget = (FTransform(FVector(140.f, 30.f, -15.f)) * BaseTransform).GetTranslation();
		LeftJointTarget = (FTransform(FVector(140.f, -30.f, -15.f)) * BaseTransform).GetTranslation();
	}
	else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown))
	{
		RightJointTarget = (FTransform(FVector(50.f, 150.f, -25.f)) * BaseTransform).GetTranslation();
		LeftJointTarget = (FTransform(FVector(50.f, -150.f, -25.f)) * BaseTransform).GetTranslation();
	}
	else if (CHECKORIENTATION(EBodyOrientation::LieDown_FaceDown))
	{
		RightJointTarget = (FTransform(FVector(50.f, -90.f, 55.f)) * BaseTransform).GetTranslation();
		LeftJointTarget = (FTransform(FVector(50.f, 90.f, 55.f)) * BaseTransform).GetTranslation();
	}

	if (RightJointTarget.Z < GroundZ + 5.f) RightJointTarget.Z = GroundZ + 5.f;
	if (LeftJointTarget.Z < GroundZ + 5.f) LeftJointTarget.Z = GroundZ + 5.f;

	return true;
}

void UVRIKBody::CalculateTwoBoneIK(const FVector& ChainStart, const FVector& ChainEnd, const FVector& JointTarget, const float UpperBoneSize, const float LowerBoneSize, FTransform& UpperBone, FTransform& LowerBone, float BendSide)
{
	const float DirectSize = FVector::Dist(ChainStart, ChainEnd);
	const float DirectSizeSquared = DirectSize * DirectSize;
	const float a = UpperBoneSize * UpperBoneSize;
	const float b = LowerBoneSize * LowerBoneSize;

	// 1) upperbone and lowerbone plane angles
	float Angle1 = FMath::RadiansToDegrees(FMath::Acos((DirectSizeSquared + a - b) / (2.f * UpperBoneSize * DirectSize)));
	float Angle2 = FMath::RadiansToDegrees(FMath::Acos((a + b - DirectSizeSquared) / (2.f * UpperBoneSize * LowerBoneSize)));

	// 2) IK plane
	FTransform ChainPlane = FTransform(ChainStart);
	FVector FrontVec = ChainEnd - ChainStart; FrontVec.Normalize();
	FVector RightVec = (JointTarget - ChainStart) * BendSide; RightVec.Normalize();
	ChainPlane.SetRotation(UKismetMathLibrary::MakeRotFromXY(FrontVec, RightVec).Quaternion());

	// 3) upper bone
	UpperBone = FTransform(FRotator(0.f, Angle1 * BendSide, 0.f), FVector::ZeroVector) * ChainPlane;

	// 4) lower bone
	ChainPlane = FTransform(UpperBone.GetRotation(), UpperBone.GetTranslation() + UpperBone.GetRotation().GetForwardVector() * UpperBoneSize);
	LowerBone = FTransform(FRotator(0.f, (Angle2 - 180.f) * BendSide, 0.f), FVector::ZeroVector) * ChainPlane;
}

void UVRIKBody::PackDataForReplication(const FTransform& Head, const FTransform& HandRight, const FTransform& HandLeft)
{
	if (!IsValid(FloorBaseComp)) return;

	FTransform t_base = FloorBaseComp->GetComponentTransform();

	// use relative transforms
	const FTransform rHead = UKismetMathLibrary::ConvertTransformToRelative(t_base, Head);
	const FTransform rHandRight = UKismetMathLibrary::ConvertTransformToRelative(t_base, HandRight);
	const FTransform rHandLeft = UKismetMathLibrary::ConvertTransformToRelative(t_base, HandLeft);
	
	FNetworkTransform t_head, t_handr, t_handl;

	PACK_NT_TRANSFORM(t_head,  rHead);
	PACK_NT_TRANSFORM(t_handr, rHandRight);
	PACK_NT_TRANSFORM(t_handl, rHandLeft);

	ServerUpdateInputs(t_head, t_handr, t_handl);

	if (ReplicateFullBodyState)
	{
		FNetworkIKBodyData s_body;
		FIKBodyData s_BodyToUse;

		if (bIsInitialized && !ReplicateInWorldSpace) {
			// convert body to relative
			// required to have smooth animation when player is moving (unnatural sliding locomotion)
			s_BodyToUse = SkeletonTransformDataRelative;
		}
		else {
			// use world space body
			s_BodyToUse = SkeletonTransformData;
		}

		PACK_NT_TRANSFORM(s_body.Pelvis,			s_BodyToUse.Pelvis);
		PACK_NT_TRANSFORM(s_body.Ribcage,			s_BodyToUse.Ribcage);
		PACK_NT_TRANSFORM(s_body.Neck,				s_BodyToUse.Neck);
		PACK_NT_TRANSFORM(s_body.Head,				s_BodyToUse.Head);
		PACK_NT_TRANSFORM(s_body.HandRight,			s_BodyToUse.HandRight);
		PACK_NT_TRANSFORM(s_body.HandLeft,			s_BodyToUse.HandLeft);
		PACK_NT_TRANSFORM(s_body.FootRightCurrent,	s_BodyToUse.FootRightCurrent);
		PACK_NT_TRANSFORM(s_body.FootLeftCurrent,	s_BodyToUse.FootLeftCurrent);

		s_body.ElbowJointTargetRight				= s_BodyToUse.ElbowJointTargetRight;
		s_body.ElbowJointTargetLeft					= s_BodyToUse.ElbowJointTargetLeft;
		s_body.Velocity								= s_BodyToUse.Velocity;
		s_body.IsJumping							= s_BodyToUse.IsJumping;
		s_body.GroundLevel							= s_BodyToUse.GroundLevel;
		s_body.GroundLevelRight						= s_BodyToUse.GroundLevelRight;
		s_body.GroundLevelLeft						= s_BodyToUse.GroundLevelLeft;
		s_body.LowerarmTwistRight					= s_BodyToUse.LowerarmTwistRight;
		s_body.LowerarmTwistLeft					= s_BodyToUse.LowerarmTwistLeft;

		s_body.CollarboneRight						= s_BodyToUse.CollarboneRight.Quaternion();
		s_body.CollarboneLeft						= s_BodyToUse.CollarboneLeft.Quaternion();

		ServerUpdateBodyState(s_body);
	}
}

bool UVRIKBody::ConvertBodyToRelative()
{
	if (IsValid(OwningPawn) && IsValid(FloorBaseComp)) {
		FTransform t_base = FloorBaseComp->GetComponentTransform();

		SkeletonTransformDataRelative.Pelvis = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.Pelvis);
		SkeletonTransformDataRelative.Ribcage = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.Ribcage);
		SkeletonTransformDataRelative.Neck = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.Neck);
		SkeletonTransformDataRelative.Head = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.Head);
		SkeletonTransformDataRelative.HandRight = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.HandRight);
		SkeletonTransformDataRelative.HandLeft = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.HandLeft);
		SkeletonTransformDataRelative.FootRightCurrent = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.FootRightCurrent);
		SkeletonTransformDataRelative.FootLeftCurrent = UKismetMathLibrary::ConvertTransformToRelative(t_base, SkeletonTransformData.FootLeftCurrent);

		SkeletonTransformDataRelative.ElbowJointTargetRight = UKismetMathLibrary::ConvertTransformToRelative(t_base, FTransform(SkeletonTransformData.ElbowJointTargetRight)).GetTranslation();
		SkeletonTransformDataRelative.ElbowJointTargetLeft = UKismetMathLibrary::ConvertTransformToRelative(t_base, FTransform(SkeletonTransformData.ElbowJointTargetLeft)).GetTranslation();
		SkeletonTransformDataRelative.Velocity = t_base.Rotator().UnrotateVector(SkeletonTransformData.Velocity);
		SkeletonTransformDataRelative.IsJumping = SkeletonTransformData.IsJumping;
		SkeletonTransformDataRelative.GroundLevel = SkeletonTransformData.GroundLevel - t_base.GetTranslation().Z;
		SkeletonTransformDataRelative.GroundLevelRight = SkeletonTransformData.GroundLevelRight - t_base.GetTranslation().Z;
		SkeletonTransformDataRelative.GroundLevelLeft = SkeletonTransformData.GroundLevelLeft - t_base.GetTranslation().Z;

		SkeletonTransformDataRelative.CollarboneRight = SkeletonTransformData.CollarboneRight;
		SkeletonTransformDataRelative.CollarboneLeft = SkeletonTransformData.CollarboneLeft;

		SkeletonTransformDataRelative.LowerarmTwistRight = SkeletonTransformData.LowerarmTwistRight;
		SkeletonTransformDataRelative.LowerarmTwistLeft = SkeletonTransformData.LowerarmTwistLeft;

		return true;
	}
	else return false;
}

inline bool UVRIKBody::RestoreBodyFromRelative()
{
	if (IsValid(OwningPawn) && IsValid(FloorBaseComp)) {
		const FTransform t_base = FloorBaseComp->GetComponentTransform();

		SkeletonTransformData.Pelvis = SkeletonTransformDataRelative.Pelvis * t_base;
		SkeletonTransformData.Ribcage = SkeletonTransformDataRelative.Ribcage * t_base;
		SkeletonTransformData.Neck = SkeletonTransformDataRelative.Neck * t_base;
		SkeletonTransformData.Head = SkeletonTransformDataRelative.Head * t_base;
		SkeletonTransformData.HandRight = SkeletonTransformDataRelative.HandRight * t_base;
		SkeletonTransformData.HandLeft = SkeletonTransformDataRelative.HandLeft * t_base;
		SkeletonTransformData.FootRightCurrent = SkeletonTransformDataRelative.FootRightCurrent * t_base;
		SkeletonTransformData.FootLeftCurrent = SkeletonTransformDataRelative.FootLeftCurrent * t_base;

		SkeletonTransformData.ElbowJointTargetRight = (FTransform(SkeletonTransformDataRelative.ElbowJointTargetRight) * t_base).GetTranslation();
		SkeletonTransformData.ElbowJointTargetLeft = (FTransform(SkeletonTransformDataRelative.ElbowJointTargetLeft) * t_base).GetTranslation();
		SkeletonTransformData.Velocity = t_base.Rotator().RotateVector(SkeletonTransformDataRelative.Velocity);
		SkeletonTransformData.GroundLevel = SkeletonTransformDataRelative.GroundLevel + t_base.GetTranslation().Z;
		SkeletonTransformData.GroundLevelRight = SkeletonTransformDataRelative.GroundLevelRight + t_base.GetTranslation().Z;
		SkeletonTransformData.GroundLevelLeft = SkeletonTransformDataRelative.GroundLevelLeft + t_base.GetTranslation().Z;
		SkeletonTransformData.CollarboneRight = SkeletonTransformDataRelative.CollarboneRight;
		SkeletonTransformData.CollarboneLeft = SkeletonTransformDataRelative.CollarboneLeft;
		SkeletonTransformData.LowerarmTwistRight = SkeletonTransformDataRelative.LowerarmTwistRight;
		SkeletonTransformData.LowerarmTwistLeft = SkeletonTransformDataRelative.LowerarmTwistLeft;

		return true;
	}
	else return false;
}

void UVRIKBody::OnRep_InputBodyState()
{
	if (ReplicateFullBodyState) {
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.Pelvis,			SkeletonTransformData_Target.Pelvis);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.Ribcage,			SkeletonTransformData_Target.Ribcage);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.Neck,				SkeletonTransformData_Target.Neck);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.Head,				SkeletonTransformData_Target.Head);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.HandRight,			SkeletonTransformData_Target.HandRight);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.HandLeft,			SkeletonTransformData_Target.HandLeft);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.FootRightCurrent,	SkeletonTransformData_Target.FootRightCurrent);
		UNPACK_NT_TRANSFORM(NT_SkeletonTransformData.FootLeftCurrent,	SkeletonTransformData_Target.FootLeftCurrent);

		SkeletonTransformData_Target.CollarboneRight					= NT_SkeletonTransformData.CollarboneRight.Rotator();
		SkeletonTransformData_Target.CollarboneLeft						= NT_SkeletonTransformData.CollarboneLeft.Rotator();
		SkeletonTransformData_Target.ElbowJointTargetRight				= NT_SkeletonTransformData.ElbowJointTargetRight;
		SkeletonTransformData_Target.ElbowJointTargetLeft				= NT_SkeletonTransformData.ElbowJointTargetLeft;
		SkeletonTransformData_Target.Velocity							= NT_SkeletonTransformData.Velocity;
		SkeletonTransformData_Target.IsJumping							= NT_SkeletonTransformData.IsJumping;
		SkeletonTransformData_Target.GroundLevel						= NT_SkeletonTransformData.GroundLevel;
		SkeletonTransformData_Target.GroundLevelRight					= NT_SkeletonTransformData.GroundLevelRight;
		SkeletonTransformData_Target.GroundLevelLeft					= NT_SkeletonTransformData.GroundLevelLeft;
		SkeletonTransformData_Target.LowerarmTwistRight					= NT_SkeletonTransformData.LowerarmTwistRight;
		SkeletonTransformData_Target.LowerarmTwistLeft					= NT_SkeletonTransformData.LowerarmTwistLeft;
	}
}

void UVRIKBody::OnRep_InputHMD()
{
	// load FNetworkTransform to FTransform
	UNPACK_NT_TRANSFORM(NT_InputHMD, InputHMD_Target);
}

void UVRIKBody::OnRep_InputHandRight()
{
	// load FNetworkTransform to FTransform
	UNPACK_NT_TRANSFORM(NT_InputHandRight, InputHandRight_Target);
}

void UVRIKBody::OnRep_InputHandLeft()
{
	// load FNetworkTransform to FTransform
	UNPACK_NT_TRANSFORM(NT_InputHandLeft, InputHandLeft_Target);
}

FCalibratedBody UVRIKBody::GetCalibratedBody() const
{
	FCalibratedBody BodyParams;
	BodyParams.fBodyWidth = BodyWidth;
	BodyParams.CharacterHeight = CharacterHeight;
	BodyParams.LegsLength = LegsLength;
	BodyParams.vNeckToHeadsetOffset = NeckToHeadsetOffset;
	BodyParams.fSpineLength = SpineLength;
	BodyParams.fHandLength = HandLength;
	BodyParams.CharacterHeightClear = CharacterHeightClear;
	BodyParams.ArmSpanClear = ArmSpanClear;

	return BodyParams;
}

void UVRIKBody::DoRestoreCalibratedBody(const FCalibratedBody& BodyParams)
{
	UE_LOG(LogVRIKBody, Log, TEXT("DoRestoreCalibratedBody on local PC"));

	if (!IsValidCalibrationData(BodyParams))
	{
		UE_LOG(LogVRIKBody, Warning, TEXT("CalibratedBody variable is invalid."));

		return;
	}

	bCalibratedT = bCalibratedI = true;

	BodyWidth = BodyParams.fBodyWidth;
	CharacterHeight = BodyParams.CharacterHeight;
	LegsLength = BodyParams.LegsLength;
	NeckToHeadsetOffset = BodyParams.vNeckToHeadsetOffset;
	SpineLength = BodyParams.fSpineLength;
	HandLength = BodyParams.fHandLength;
	CharacterHeightClear = BodyParams.CharacterHeightClear;
	ArmSpanClear = BodyParams.ArmSpanClear;

	nModifyHeightState = HEIGHT_STABLE;
	UpperarmLength = HandLength * UpperarmForearmRatio / (UpperarmForearmRatio + 1.f);
	ThighLength = LegsLength * ThighCalfRatio / (ThighCalfRatio + 1.f);
	bResetFeet = true;

	OnCalibrationComplete.Broadcast();
}

void UVRIKBody::RestoreCalibratedBody(const FCalibratedBody& BodyParams)
{
	if (GetIsReplicated()) {
		UE_LOG(LogVRIKBody, Log, TEXT("RestoreCalibratedBody: server call"));

		ServerRestoreCalibratedBody(BodyParams);
	}
	else {
		DoRestoreCalibratedBody(BodyParams);
	}
}

void UVRIKBody::ResetCalibrationStatus()
{
	if (GetIsReplicated()) {
		ServerResetCalibrationStatus();
	}
	else {
		DoResetCalibrationStatus();
	}
}

void UVRIKBody::DoResetCalibrationStatus()
{
	bCalibratedT = bCalibratedI = false;
}

// Clamp elbow offset vector. InternalSide is a direction from upperarm to spine, UpSide is ribcage up vector, HandUpVector is palm centre->thumb vector
void UVRIKBody::ClampElbowRotation(const FVector InternalSide, const FVector UpSide, const FVector HandUpVector, FVector& CurrentAngle)
{
	// check limits
	FQuat LimAngleA;
	FVector LimAngle;
	const float SideLimit = -0.4f;

	if (FVector::DotProduct(InternalSide, HandUpVector) < SideLimit) {
		// elbow is inside
		LimAngleA = FQuat::FastLerp(
			FRotationMatrix::MakeFromX(InternalSide).ToQuat(),
			FRotationMatrix::MakeFromX(UpSide * -1.f).ToQuat(),
			0.35f);
		LimAngleA.Normalize();
		LimAngle = LimAngleA.GetForwardVector();

		float cos1 = FVector::DotProduct(UpSide, LimAngle);
		float cos2 = FVector::DotProduct(UpSide, CurrentAngle);
		if (cos1 < cos2) {
			CurrentAngle = FMath::Lerp(CurrentAngle, LimAngle, (1.f - cos2 / cos1));
		}
		if (bDebugOutput) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, "internal");
	}
	else if (FVector::DotProduct(InternalSide, HandUpVector) > SideLimit && FVector::DotProduct(UpSide, HandUpVector) < 0.f) {
		// elbow is outside
		LimAngleA = FQuat::FastLerp(
			FRotationMatrix::MakeFromX(InternalSide * -1.f).ToQuat(),
			FRotationMatrix::MakeFromX(UpSide).ToQuat(),
			0.5f);
		LimAngleA.Normalize();
		LimAngle = LimAngleA.GetForwardVector();

		float cos1 = FVector::DotProduct(UpSide, LimAngle);
		float cos2 = FVector::DotProduct(UpSide, CurrentAngle);
		if (cos1 < cos2) {
			CurrentAngle = FMath::Lerp(CurrentAngle, LimAngle, ((cos2 / cos1) - 1.f) * 1.8f);
		}
		if (bDebugOutput) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::White, "external");
	}
}

// calcuate distance from point to line
inline float UVRIKBody::DistanceToLine(FVector LineA, FVector LineB, FVector Point)
{
	FVector a = LineB - LineA;
	FVector c = Point - LineA;
	float size = c.Size();
	a.Normalize(); c.Normalize();

	const float cosa = FVector::DotProduct(a, c); // cos
	return size * FMath::Sqrt(1.f - cosa*cosa); // c*sin
}

inline bool UVRIKBody::IsHandInFront(const FVector& Forward, const FVector& Ribcage, const FVector& Hand)
{
	const FVector Dir = (Hand - Ribcage).GetSafeNormal();
	return (FVector::DotProduct(Forward, Dir) > 0.f);
}

// Calculate Forward Rotation (Yaw) for pelvis
FORCEINLINE FVector UVRIKBody::GetForwardDirection(const FVector ForwardVector, const FVector UpVector)
{
	float s, c;
	FMath::SinCos(&s, &c, FMath::DegreesToRadians(GetForwardYaw(ForwardVector, UpVector)));
	return FVector(c, s, 0.f);
	//return FRotator(0.f, GetForwardYaw(ForwardVector, UpVector), 0.f).Quaternion().GetForwardVector();
}

FORCEINLINE float UVRIKBody::GetForwardYaw(const FVector ForwardVector, const FVector UpVector)
{
	const FVector Fxy = FVector(ForwardVector.X, ForwardVector.Y, 0.f);
	FVector FxyN = Fxy.GetSafeNormal();
	const FVector Uxy = FVector(UpVector.X, UpVector.Y, 0.f);
	const FVector UxyN = Uxy.GetSafeNormal();

	// squared limit value
	const float TurningTreshold = 0.3f * 0.3f;

	// projection of the forward vector on horizontal plane is too small
	const float ProjectionSize = Fxy.SizeSquared();
	if (ProjectionSize < TurningTreshold) {
		// interpolation alpha
		const float alpha = FMath::Clamp((TurningTreshold - ProjectionSize) / TurningTreshold, 0.f, 1.f);

		// new forward vector
		FVector NewF;
		// choose down or up side and use interpolated vector between forward and up to find current horizontal forward direction
		if (ForwardVector.Z < 0.f && UpVector.Z < 0.f) {
			NewF = FMath::Lerp(-ForwardVector, UpVector, alpha);
		}
		else if (ForwardVector.Z < 0.f && UpVector.Z >= 0.f) {
			NewF = FMath::Lerp(ForwardVector, UpVector, alpha);
		}
		else if (ForwardVector.Z >= 0.f && UpVector.Z >= 0.f) {
			FVector NewUpVector = FVector(UpVector.X * -1.f, UpVector.Y * -1.f, UpVector.Z);
			NewF = FMath::Lerp(ForwardVector, NewUpVector, alpha);
		}
		else if (ForwardVector.Z >= 0.f && UpVector.Z < 0.f) {
			FVector NewFVector = FVector(ForwardVector.X * -1.f, ForwardVector.Y * -1.f, ForwardVector.Z);
			FVector NewUpVector = UpVector * -1.f;
			NewF = FMath::Lerp(NewFVector, NewUpVector, alpha);
		}

		FxyN = FVector(NewF.X, NewF.Y, 0.f).GetSafeNormal();

		return FMath::RadiansToDegrees(FMath::Atan2(FxyN.Y, FxyN.X));
	}
	// projection of the forward vector on horizontal plane is large enough to use it
	else {
		const float YawRet = FMath::RadiansToDegrees(FMath::Atan2(FxyN.Y, FxyN.X));
		return (UpVector.Z > 0.f) ? YawRet : -YawRet;
	}
}