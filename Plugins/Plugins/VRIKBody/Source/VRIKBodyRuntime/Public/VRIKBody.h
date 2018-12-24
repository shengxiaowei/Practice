// VR IK Body Plugin
// (c) Yuri N Kalinin, 2017, ykasczc@gmail.com. All right reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "IMotionController.h"
#include "VRIKBody.generated.h"

#define VERSION_ENGINE_MINOR 20

#define HEIGHT_INIT					0
#define HEIGHT_MODIFY				1
#define HEIGHT_STABLE				2
#define CORR_POINTS_NUM				10
#define CORR_SAVE_POINTS_NUM		100
#define UpperarmForearmRatio		1.08f
#define ThighCalfRatio				1.f

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIKBodyRepeatable, int32, Iteration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIKBodySimple);

DECLARE_LOG_CATEGORY_EXTERN(LogVRIKBody, Log, All);

/* VR headset and controllers input mode:
DirectVRInput - input from UHeadMountedDisplayFunctionLibrary and I Motioncontroller interface
InputFromComponents - input from scene components (recommended)
InputFromVariables - input from variables manually updated in blueprint */
UENUM(BlueprintType, Blueprintable)
enum class EVRInputSetup : uint8
{
	DirectVRInput				UMETA(DisplayName = "Direct VR Input"),
	InputFromComponents			UMETA(DisplayName = "Input From Components"),
	InputFromVariables			UMETA(DisplayName = "Input From Variables")
};

/* Current body orientation state. Body calculation depends on this state. */
UENUM(BlueprintType, Blueprintable)
enum class EBodyOrientation : uint8
{
	Stand					UMETA(DisplayName = "Stand"),
	Sit						UMETA(DisplayName = "Sit"),
	Crawl					UMETA(DisplayName = "Crawl"),
	LieDown_FaceUp			UMETA(DisplayName = "Lie Down with Face Up"),
	LieDown_FaceDown		UMETA(DisplayName = "Lie Down with Face Down")
};

/* Data set describing calibrated body state. Required to save and load body params on player respawn */
USTRUCT(Blueprintable)
struct FCalibratedBody
{
	GENERATED_USTRUCT_BODY()

	/* Player body width (only for calubrated body) */
	UPROPERTY()
	float fBodyWidth;
	/* Player height from ground to eyes */
	UPROPERTY()
	float CharacterHeight;
	/* Clear vertical distance from ground to camera without any adjustments */
	UPROPERTY()
	float CharacterHeightClear;
	/* Clear horizontal distance between two motion controllers at T pose */
	UPROPERTY()
	float ArmSpanClear;
	/* Feet-pelvis approximate vertical distance */
	UPROPERTY()
	float LegsLength;
	/* Location of a neck relative to VR headset */
	UPROPERTY()
	FVector vNeckToHeadsetOffset;
	/* Approximate pelvis-to-ricage length */
	UPROPERTY()
	float fSpineLength;
	/* Player's hand length (only for calubrated body) */
	UPROPERTY()
	float fHandLength;
};

/* Simplified Transform Data for networking */
USTRUCT(BlueprintType)
struct FNetworkTransform
{
	GENERATED_USTRUCT_BODY()

	/* Location in actor space */
	UPROPERTY()
	FVector_NetQuantize100 Location;
	/* Rotation in actor space */
	UPROPERTY()
	FQuat Rotation;
};

/* Simplified Body State struct for networking */
USTRUCT(BlueprintType)
struct FNetworkIKBodyData
{
	GENERATED_USTRUCT_BODY()

	/* Pelvis transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform Pelvis;
	/* Ribcage transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform Ribcage;
	/* Neck Transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform Neck;
	/* Head transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform Head;
	/* Right collarbone rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FQuat CollarboneRight;
	/* Right hand palm transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform HandRight;
	/* Left collarbone rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FQuat CollarboneLeft;
	/* Left hand palm transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform HandLeft;
	/* Right elbow IK target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FVector ElbowJointTargetRight;
	/* Left elbow IK target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FVector ElbowJointTargetLeft;
	/* Right foot transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform FootRightCurrent;
	/* Left foot transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FNetworkTransform FootLeftCurrent;
	/* Is player jumbing or not */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	bool IsJumping;
	/* is player crawling or laying down? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	EBodyOrientation BodyOrientation;
	/* Current player velocity vector */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	FVector Velocity;
	/* Current ground Z coordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevel;
	/* Current ground Z coordinate for the right foot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevelRight;
	/* Current ground Z coordinate for the left foot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevelLeft;
	/* Twist value between elbow and wrist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float LowerarmTwistRight;
	/* Twist value between elbow and wrist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float LowerarmTwistLeft;
};

/* Internal struct to store VR Input history */
USTRUCT()
struct FVRInputData
{
	GENERATED_USTRUCT_BODY()

	/* VR Headset Location */
	UPROPERTY()
	FVector HeadLoc;
	/* Left Motion Controller Location */
	UPROPERTY()
	FVector LeftHandLoc;
	/* Right Motion Controller Location */
	UPROPERTY()
	FVector RightHandLoc;
	/* VR Headset Rotation */
	UPROPERTY()
	FRotator HeadRot;
	/* Left Motion Controller Rotation */
	UPROPERTY()
	FRotator LeftHandRot;
	/* Right Motion Controller Rotation */
	UPROPERTY()
	FRotator RightHandRot;
};

/* Calculated instantaneous body state */
USTRUCT(BlueprintType)
struct FIKBodyData
{
	GENERATED_USTRUCT_BODY()

	/* Pelvis Transform (always in world space) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform Pelvis;
	/* Ribcage/Spine Transform in world space */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform Ribcage;
	/* Neck Transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform Neck;
	/* Head Transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform Head;
	/* Right collarbone relative (yaw, pitch) rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FRotator CollarboneRight;
	/* Right Upperarm Transform (only if ComputeHandsIK is on) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform UpperarmRight;
	/* Right Forearm Transform (only if ComputeHandsIK is on) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform ForearmRight;
	/* Right Palm Transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform HandRight;
	/* Left collarbone relative (yaw, pitch) rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FRotator CollarboneLeft;
	/* Left Forearm Transform (only if ComputeHandsIK is on) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform UpperarmLeft;
	/* Left Forearm Transform (only if ComputeHandsIK is on) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform ForearmLeft;
	/* Left Palm Transform */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform HandLeft;
	/* IK Joint Target for right hand in world space (if ComputeHandsIK is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FVector ElbowJointTargetRight;
	/* IK Joint Target for left hand in world space (if ComputeHandsIK is true) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FVector ElbowJointTargetLeft;
	/* Right Feet IK Transform (don't use this value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform FootRightTarget;
	/* Left Feet IK Transform (don't use this value) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform FootLeftTarget;
	/* Right Thigh Transform (only if ComputeLegsIK is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform ThighRight;
	/* Right Calf Transform (only if ComputeLegsIK is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform CalfRight;
	/* Left Thigh Transform (only if ComputeLegsIK is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform ThighLeft;
	/* Left Calf Transform (only if ComputeLegsIK is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform CalfLeft;
	/* Right Feet IK instantaneous Transform (only if ComputeFeetIKTargets is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform FootRightCurrent;
	/* Left Feet IK instantaneous Transform (only if ComputeFeetIKTargets is set) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody Bone Transform")
	FTransform FootLeftCurrent;
	/* Flag indicates if character is jumping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	bool IsJumping;
	/* is player crawling or laying down? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	EBodyOrientation BodyOrientation;
	/* Flag indicates if character is sitting */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	bool IsSitting;
	/* Current Player Vector Velocity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	FVector Velocity;
	/* Current Ground Z Coordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevel;
	/* Current ground Z coordinate for right foot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevelRight;
	/* Current ground Z coordinate for left foot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float GroundLevelLeft;
	/* Twist value between elbow and wrist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float LowerarmTwistRight;
	/* Twist value between elbow and wrist */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody State")
	float LowerarmTwistLeft;
};

/*******************************************************************************************************/

/* This component uses input received from VR head set and controllers to calculate approximate player's body state */
UCLASS(
	Blueprintable,
	ClassGroup=(IKBody),
	meta=(BlueprintSpawnableComponent)
)
class VRIKBODYRUNTIME_API UVRIKBody : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVRIKBody();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray <FLifetimeProperty> &OutLifetimeProps) const override;

	/* Set this flag to True if you need Feet Transform Predictions */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool ComputeFeetIKTargets;
	/* Set this flag to True if you need thigh and calf transforms. Only works if ComputeFeetIKTargets is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool ComputeLegsIK;
	/* Set this flag to True if you need upperarm and forearm transforms */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool ComputeHandsIK;

	/* If true, Owning Pawn location Z will be used as floor coordinate; doesn't work properly on slopes and staircases */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool UseActorLocationAsFloor;
	/* If true, component is tracing groun by Object Type, if false - by channel */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool TraceFloorByObjectType;
	/* Collision channel (Object Type) of floor if UseActorLocationAsFloor is false */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	TEnumAsByte<ECollisionChannel> FloorCollisionObjectType;
	/* If VR Input Option is equal to 'Input from Components', call Initialize(...) function to setup Camera and Hand components.
	Don't use 'Input from Variables' if component is replicated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	EVRInputSetup VRInputOption;
	/* If true, shoulders don't rotate to follow hands */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool LockShouldersRotation;
	/* This flag only works if VRInputOption is DirectVRInput. If true, component uses Pawn Actor Transform to locate body
	in world space. Otherwise it retuns body relative to Pawn Origin. World space calculation is required for unnatural locomotion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool FollowPawnTransform;
	/* If true, VRIKBody will try to detect continuous head rotations. Otherwise hip is aligned to Head Yaw by timeout */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool DetectContinuousHeadYawRotation;
	/* Enable twisting to decrease errors in torso orientation by rotiting ribcage. False value locks ribcage and pelvis rotations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool EnableTorsoTwist;
	/* How strongly should torso follow head rotation? */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	float TorsoRotationSensitivity;
	/* Check for hard attachment of torso rotation to head rotation */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool bLockTorsoToHead;

	/* Left palm location relative to Motion Controller Component transform. Default value works fine if you hold HTC Vive controller
	as a pistol. For other cases reset this transform to zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	FTransform LeftPalmOffset;
	/* Right palm location relative to Motion Controller Component transform. Default value works fine if you hold HTC Vive controller
	as a pistol. For other cases reset this transform to zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	FTransform RightPalmOffset;	
	/* Tick Interval of the timer, calculating correlation betwen hands and head */
	UPROPERTY(EditAnywhere, Category = "IKBody | Setup")
	float TimerInterval;
	/** Enable this flag if your pawn can has pitch and roll rotaion */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Setup")
	bool bDoPawnOrientationAdjustment;

	/* Relative Y-axis body size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float BodyWidth;
	/* Relative X-axis body size */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float BodyThickness;
	/* Approximate head radius */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float HeadHalfWidth;
	/* Approximate head height */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float HeadHeight;
	/* Approximate distance from pelvis to neck */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float SpineLength;
	/* Approximate distance from collarbone to palm */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float HandLength;
	/* Distance from feet to ground, useful to correct skeletal mesh feet bones Z-offset */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float FootOffsetToGround;

	/* Don't modify this param if using calibration (X is forward, Z is up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	FVector NeckToHeadsetOffset;
	/* Component Space Offset from Ribcage to Neck (X is forward, Z is up) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	FVector RibcageToNeckOffset;
	/* Max possible angle between head and ribcage Yaw rotations (used to correct pelvis rotation) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float MaxHeadRotation;
	/* Angle between head and ribcage Yaw rotations to  trigger ribcage rotation (torso twist, EnableTorsoTwist must be enabled); 0 to disable */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IKBody | Body Params")
	float SoftHeadRotationLimit;

	/* If true, the component calculates body state on local PC and send it to server for other connected users.
	If false, component sends Head and Hands transforms to server, and every client perform body calculation
	for each player locally. Choose first option (true) if CPU performance is a priority in your project. Choose remote
	calculations (false) to optimize a project for network bandwidth. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IKBody | Networking")
	bool ReplicateFullBodyState;

	/* Only available if VR Input Option is 'Input from components' or components are initialized by Initialize(...)
	function. This flag orders component to move remote Motion Controllers and Head component with a body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IKBody | Networking")
	bool ReplicateComponentsMovement;

	/* If ReplicateInWorldSpace is set to false (by default), all body data would be converted to actor space
	before replication and reconverted to world space on remote machines. This operation adds a lot of transforms
	calculations, but allow to use sliding (Onward-style) locomotion which implies that pawn locations
	on different PCs are slightly asynchronous at the same moment. Set ReplicateInWorldSpace to true if
	you don't use sliding player locomotion to optimize CPU usage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IKBody | Networking")
	bool ReplicateInWorldSpace;

	/* Interpolation speed for bones transforms received via replication. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IKBody | Networking")
	float SmoothingInterpolationSpeed;

	/* Body Calibration Complete successfully */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodySimple OnCalibrationComplete;

	// TODO: add player movements analysis
	/* Event called when player starts a jump, NOT IN USE */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodySimple OnJumpStarted;
	/* Event called when player ends a jump, NOT IN USE */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodySimple OnGrounded;
	/* Event called when player finishing squatting, NOT IN USE */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodySimple OnSitDown;
	/* Event called when player stands up, NOT IN USE */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodySimple OnStandUp;
	/* Not in use! */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodyRepeatable OnHeadShake;
	/* Not in use! */
	UPROPERTY(BlueprintAssignable, Category = "IKBody")
	FIKBodyRepeatable OnHeadNod;

	/* Calibrate Body Params at T-Pose (hand to the left and right) */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool CalibrateBodyAtTPose();
	/* Calibrate Body Params at I-Pose (hand down) */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool CalibrateBodyAtIPose();
	/* Calibrate Body Params at T or I pose (detected automatically) */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool AutoCalibrateBodyAtPose();

	/* This function activates recording of HMD and motion controllers locations and orientation. If use replication or Input from Components, use Initialize(...) instead.
	@param RootComp in a Parent Component for VR Camera and controllers, equal to player area centre at the floor.
	@return true if successful. */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool ActivateInput(USceneComponent* RootComp = nullptr);

	/* This function stops recording of HMD and motion controllers locations and orientation */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void DeactivateInput();

	/* Call this function on BeginPlay to setup component references if VRInputOption is equal to 'Input From Components' and in a multiplayer games.
	For networking use only Motion Controller components as hand controllers or make sure this components are properly initialized
	(tracking must be enabled on local PCs and disabled on remotes) manually. */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void Initialize(USceneComponent* Camera, class UPrimitiveComponent* RightController, class UPrimitiveComponent* LeftController);

	/* Main function to calculate Body State.
	@param DeltaTime - world tick delta time
	@return a struct describing current body state */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void ComputeFrame(float DeltaTime, FIKBodyData& ReturnValue);

	/* Returns a last body state struct calculated by ComputeFrame(...) function */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	FIKBodyData GetLastFrameData();

	/* Legacy function! Only works for default UE4 Skeleton.
	Converts calculated body parts orientation to skeleton bones
	orientation. It's useful to directly set bone transforms in Anim Blueprint
	using 'Modify (Transform) Bone' node. Keep in mind that transforms in the
	returned struct are still in world space. */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	FIKBodyData ConvertDataToSkeletonFriendly(const FIKBodyData& WorldSpaceIKBody);

	/* Call this function to set Pitch and Roll of Pelvis to zero and Yaw equal to Head Yaw */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void ResetTorso();

	/* Returns calculated or calibrated character height */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	float GetCharacterHeight() const { return CharacterHeight; };

	/* Returns calculated or calibrated legs length */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	float GetCharacterLegsLength() const { return LegsLength; };

	/* Detaches Palm from Motion Controller and attaches to primitive component (for example, two-handed rifle) */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool AttachHandToComponent(EControllerHand Hand, UPrimitiveComponent* Component, const FName& SocketName, const FTransform& RelativeTransform);

	/* Reattach hand palm to Motion Controller Component */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void DetachHandFromComponent(EControllerHand Hand);

	/* Head/hands Input if VRInputOption is 'Input from Variables' */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void UpdateInputTransforms(const FTransform& HeadTransform, const FTransform& RightHandTransform, const FTransform& LeftHandTransform) { InputHMD = HeadTransform; InputHandRight = RightHandTransform; InputHandLeft = LeftHandTransform; };

	/* @return The object describing body calibration results. Use it to save and restore body params if you need to respawn player. */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	FCalibratedBody GetCalibratedBody() const;

	/* Load body calibration params */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void RestoreCalibratedBody(const FCalibratedBody& BodyParams);

	/* Mark body as non-calibrated. Function (replicated) keeps existing body params, but allow to recalibrate body if necessary. */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void ResetCalibrationStatus();

	/* Check body calibration params */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	bool IsValidCalibrationData(const FCalibratedBody& BodyParams) const { return (BodyParams.fBodyWidth != 0.f && BodyParams.CharacterHeight != 0.f && BodyParams.CharacterHeightClear != 0.f && BodyParams.ArmSpanClear != 0.f); };

	/* Clear vertical distance from ground to camera without any adjustments */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	float GetClearCharacterHeight() const { return CharacterHeightClear; };
	
	/* Clear horizontal distance between palms without any adjustments */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	float GetClearArmSpan() const { return ArmSpanClear; };

	/* Return true if calibration was complete or calibration data is loaded by RestoreCalibratedBody */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	bool IsBodyCalibrated() const { return (bCalibratedT && bCalibratedI); };

	/* @return Joint Targets for elbows to use in TwoBodyIK animation node (in world space) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	void GetElbowJointTarget(FVector& RightElbowTarget, FVector& LeftElbowTarget) {
		RightElbowTarget = SkeletonTransformData.ElbowJointTargetRight;
		LeftElbowTarget = SkeletonTransformData.ElbowJointTargetLeft;
	};

	/* @return Joint Targets for knees to use in TwoBodyIK animation node (in world space) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	void GetKneeJointTarget(FVector& RightKneeTarget, FVector& LeftKneeTarget);

	/* @return true if component was initialized by ActivateInput(...) or Initialize(...) function */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "IKBody")
	bool IsInitialized() { return bIsInitialized; };

	/* Manually set body orientation (for example, lying at the ground) */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void SetManualBodyOrientation(EBodyOrientation NewOrientation) { bManualBodyOrientation = true; SkeletonTransformData.BodyOrientation = NewOrientation; };

	/* Restore automatic body calibration */
	UFUNCTION(BlueprintCallable, Category = "IKBody")
	void UseAutoBodyOrientation() { bManualBodyOrientation = false; };

	/* Returns delta rotation between FloorComp rotation and zero rotator */
	void GetFloorRotationAdjustment(FTransform& SimulatedBaseTransform, FTransform& RealBaseTransform);

protected:
	/* Load body calibration params (replicated version) */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRestoreCalibratedBody(const FCalibratedBody& BodyParams);
	bool ServerRestoreCalibratedBody_Validate(const FCalibratedBody& BodyParams) { return true; };
	void ServerRestoreCalibratedBody_Implementation(const FCalibratedBody& BodyParams) { DoRestoreCalibratedBody(BodyParams); ClientRestoreCalibratedBody(BodyParams); };

	/* Restore body calibration params on clients (replicated version) */
	UFUNCTION(NetMulticast, Reliable)
	void ClientRestoreCalibratedBody(const FCalibratedBody& BodyParams);
	void ClientRestoreCalibratedBody_Implementation(const FCalibratedBody& BodyParams) { DoRestoreCalibratedBody(BodyParams); };

	// Restoring body calibration from calbirated body struct variable
	UFUNCTION()
	void DoRestoreCalibratedBody(const FCalibratedBody& BodyParams);

	/* Reset body calibration state (replicated version) */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerResetCalibrationStatus();
	bool ServerResetCalibrationStatus_Validate() { return true; };
	void ServerResetCalibrationStatus_Implementation() { DoResetCalibrationStatus(); ClientResetCalibrationStatus(); };

	/* Reset body calibration state on clients (replicated version) */
	UFUNCTION(NetMulticast, Reliable)
	void ClientResetCalibrationStatus();
	void ClientResetCalibrationStatus_Implementation() { DoResetCalibrationStatus(); };

	// Reset body calibration state
	UFUNCTION()
	void DoResetCalibrationStatus();

	// Replication notifies
	UFUNCTION()
	void OnRep_InputBodyState();
	UFUNCTION()
	void OnRep_InputHMD();
	UFUNCTION()
	void OnRep_InputHandRight();
	UFUNCTION()
	void OnRep_InputHandLeft();

private:
	/* HMD camera component pointer */
	UPROPERTY()
	USceneComponent* CameraComponent;
	/* Motion Controller component pointer for right hand */
	UPROPERTY()
	UPrimitiveComponent* RightHandController;
	/* Motion Controller component pointer for left hand */
	UPROPERTY()
	UPrimitiveComponent* LeftHandController;
	// Pointer to player's pawn
	UPROPERTY()
	APawn* OwningPawn;
	UPROPERTY()
	USceneComponent* FloorBaseComp;

	// Global State
	UPROPERTY()
	bool bIsInitialized;
	UPROPERTY()
	bool bDebugOutput;
	UPROPERTY()
	bool bManualBodyOrientation;

	// Replicated body state and VR Input
	UPROPERTY(ReplicatedUsing = OnRep_InputBodyState)
	FNetworkIKBodyData NT_SkeletonTransformData;
	UPROPERTY(ReplicatedUsing = OnRep_InputHMD)
	FNetworkTransform NT_InputHMD;
	UPROPERTY(ReplicatedUsing = OnRep_InputHandRight)
	FNetworkTransform NT_InputHandRight;
	UPROPERTY(ReplicatedUsing = OnRep_InputHandLeft)
	FNetworkTransform NT_InputHandLeft;

	// Calculation result
	UPROPERTY()
	FIKBodyData SkeletonTransformData;
	UPROPERTY()
	FIKBodyData SkeletonTransformDataRelative;

	// Current Input for manual input from variables
	UPROPERTY()
	FTransform InputHMD;
	UPROPERTY()
	FTransform InputHandRight;
	UPROPERTY()
	FTransform InputHandLeft;

	// Target (not current) replicated transforms. Current body state is interpolated to target.
	UPROPERTY()
	FIKBodyData SkeletonTransformData_Target;
	UPROPERTY()
	FTransform InputHMD_Target;
	UPROPERTY()
	FTransform InputHandRight_Target;
	UPROPERTY()
	FTransform InputHandLeft_Target;

	// Previous headset transform
	UPROPERTY()
	FVector vPrevCamLocation;
	UPROPERTY()
	FRotator rPrevCamRotator;

	// Hands Attachment
	// In some interactions hands need to be attached to scene components instead of motion controllers
	UPROPERTY()
	bool HandAttachedRight;
	UPROPERTY()
	bool HandAttachedLeft;
	UPROPERTY()
	UPrimitiveComponent* HandParentRight;
	UPROPERTY()
	UPrimitiveComponent* HandParentLeft;
	UPROPERTY()
	FTransform HandAttachTransformRight;
	UPROPERTY()
	FTransform HandAttachTransformLeft;
	UPROPERTY()
	FName HandAttachSocketRight;
	UPROPERTY()
	FName HandAttachSocketLeft;

	// Timers
	UPROPERTY()
	FTimerHandle hVRInputTimer;
	UPROPERTY()
	FTimerHandle hResetFootLTimer;
	UPROPERTY()
	FTimerHandle hResetFootRTimer;
	UPROPERTY()
	FTimerHandle hTorsoYawTimer;

	// Feet animation
	UPROPERTY()
	FTransform FootTargetTransformL;
	UPROPERTY()
	FTransform FootTargetTransformR;
	UPROPERTY()
	FTransform FootTickTransformL;
	UPROPERTY()
	FTransform FootTickTransformR;
	UPROPERTY()
	FTransform FootLastTransformL;
	UPROPERTY()
	FTransform FootLastTransformR;
	UPROPERTY()
	float FeetCyclePhase;
	UPROPERTY()
	float FeetMovingStartedTime;
	UPROPERTY()
	float fDeltaTimeL;
	UPROPERTY()
	float fDeltaTimeR;
	UPROPERTY()
	bool bResetFeet;

	// Reset ribcage yaw rotation after timeout
	UPROPERTY()
	uint8 nYawControlCounter;
	UPROPERTY()
	bool bYawInterpToHead;
	UPROPERTY()
	bool bResetTorso;
	UPROPERTY()
	FVector SavedTorsoDirection;

	// Initial auto calibration setup
	UPROPERTY()
	bool ResetFootLocationL;
	UPROPERTY()
	bool ResetFootLocationR;
	UPROPERTY()
	uint8 nModifyHeightState; // 0 - waiting; 1 - modify on; 2 - don't modify
	UPROPERTY()
	float ModifyHeightStartTime;
	
	// Internal current body state
	UPROPERTY()
	bool bTorsoYawRotation;
	UPROPERTY()
	bool bTorsoPitchRotation;
	UPROPERTY()
	bool bTorsoRollRotation;
	UPROPERTY()
	bool bIsSitting;
	UPROPERTY()
	bool bIsStanding;
	UPROPERTY()
	bool bIsWalking;
	UPROPERTY()
	float JumpingOffset;
	UPROPERTY()
	float CharacterHeight;
	UPROPERTY()
	float CharacterHeightClear;
	UPROPERTY()
	float ArmSpanClear;
	UPROPERTY()
	float LegsLength;
	UPROPERTY()
	uint8 nIsRotating;
	UPROPERTY()
	float StartRotationYaw;
	UPROPERTY()
	float UpperarmLength;
	UPROPERTY()
	float ThighLength;

	// Calibration Setup
	UPROPERTY()
	FTransform CalT_Head;
	UPROPERTY()
	FTransform CalT_ControllerL;
	UPROPERTY()
	FTransform CalT_ControllerR;
	UPROPERTY()
	FTransform CalI_Head;
	UPROPERTY()
	FTransform CalI_ControllerL;
	UPROPERTY()
	FTransform CalI_ControllerR;
	UPROPERTY()
	bool bCalibratedT;
	UPROPERTY()
	bool bCalibratedI;
	UPROPERTY()
	float TracedFloorLevel;
	UPROPERTY()
	float TracedFloorLevelR;
	UPROPERTY()
	float TracedFloorLevelL;

	// Variables to calculate body state
	UPROPERTY()
	float HeadPitchBeforeSitting;
	UPROPERTY()
	FVector HeadLocationBeforeSitting;
	UPROPERTY()
	float RecalcCharacteHeightTime;
	UPROPERTY()
	float RecalcCharacteHeightTimeJumping;
	UPROPERTY()
	float ShoulderYawOffsetRight;
	UPROPERTY()
	float ShoulderYawOffsetLeft;
	UPROPERTY()
	float ShoulderPitchOffsetRight;
	UPROPERTY()
	float ShoulderPitchOffsetLeft;

	// Motion Controller interface pointer
	IMotionController* MControllers;

	UPROPERTY()
	FRotator PrevFrameActorRot;

	// Saved VR Input Data
	UPROPERTY()
	uint8 CurrentVRInputIndex;

	FVRInputData VRInputData[CORR_SAVE_POINTS_NUM + 4];

	UPROPERTY()
	float PreviousElbowRollRight;
	UPROPERTY()
	float PreviousElbowRollLeft;

	// Data Correltaion arrays
	float cor_rot_a[3][CORR_POINTS_NUM];
	float cor_rot_b[3][CORR_POINTS_NUM];
	float cor_rot_c[3][CORR_POINTS_NUM];
	float cor_loc_a[3][CORR_POINTS_NUM];
	float cor_loc_b[3][CORR_POINTS_NUM];
	float cor_loc_c[3][CORR_POINTS_NUM];
	float cor_rotv_a[3][CORR_POINTS_NUM];
	float cor_rotv_b[3][CORR_POINTS_NUM];
	float cor_rotv_c[3][CORR_POINTS_NUM];
	float cor_linear[3][CORR_POINTS_NUM];

	//////////////////////////////////////////////////////////////////////////////

	UFUNCTION()
	void VRInputTimer_Tick();

	UFUNCTION()
	void RibcageYawTimer_Tick();

	UFUNCTION()
	void ResetFootsTimerL_Tick();

	UFUNCTION()
	void ResetFootsTimerR_Tick();

	UFUNCTION()
	void GetCorrelationKoef(int32 StartIndex = -1);

	UFUNCTION()
	void CalcShouldersWithOffset(EControllerHand Hand);

	UFUNCTION()
	void CalcHandIKTransforms(EControllerHand Hand);

	UFUNCTION()
	void CalcFeetInstantaneousTransforms(float DeltaTime, float FloorZ);

	UFUNCTION()
	void CalcFeetIKTransforms2(float DeltaTime, float FloorZ);

	inline float GetAngleToInterp(const float Current, const float Target);
	inline float GetFloorCoord();
	inline FTransform GetHMDTransform(bool bRelative = false);
	inline FTransform GetHandTransform(EControllerHand Hand, bool UseDefaults = true, bool PureTransform = true, bool bRelative = false);
	inline bool ConvertBodyToRelative();
	inline bool RestoreBodyFromRelative();
	inline float DistanceToLine(FVector LineA, FVector LineB, FVector Point);
	inline bool IsHandInFront(const FVector& Forward, const FVector& Ribcage, const FVector& Hand);
	FORCEINLINE EBodyOrientation ComputeCurrentBodyOrientation(const FTransform& Head, const FTransform& HandRight, const FTransform& HandLeft);
	FORCEINLINE FVector GetForwardDirection(const FVector ForwardVector, const FVector UpVector);
	FORCEINLINE float GetForwardYaw(const FVector ForwardVector, const FVector UpVector);
	FORCEINLINE bool GetKneeJointTargetForBase(const FTransform& BaseTransform, const float GroundZ, FVector& RightJointTarget, FVector& LeftJointTarget);

	// perform calibration
	UFUNCTION()
	void CalibrateSkeleton();

	// single line trace to floor
	UFUNCTION()
	void TraceFloor(const FVector HeadLocation);

	// simple two-bone IK
	UFUNCTION()
	void CalculateTwoBoneIK(const FVector& ChainStart, const FVector& ChainEnd, const FVector& JointTarget, const float UpperBoneSize, const float LowerBoneSize, FTransform& UpperBone, FTransform& LowerBone, float BendSide = 1.f);

	// helper function to calculate elbow IK targets
	UFUNCTION()
	void ClampElbowRotation(const FVector InternalSide, const FVector UpSide, const FVector HandUpVector, FVector& CurrentAngle);

	// send body state to server
	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerUpdateBodyState(const FNetworkIKBodyData& State);
	bool ServerUpdateBodyState_Validate(const FNetworkIKBodyData& State) { return true; };
	void ServerUpdateBodyState_Implementation(const FNetworkIKBodyData& State) {
		NT_SkeletonTransformData = State;
		OnRep_InputBodyState();
	};

	// send VR input to server
	UFUNCTION(Server, Unreliable, WithValidation)
	void ServerUpdateInputs(const FNetworkTransform& Head, const FNetworkTransform& HandRight, const FNetworkTransform& HandLeft);
	bool ServerUpdateInputs_Validate(const FNetworkTransform& Head, const FNetworkTransform& HandRight, const FNetworkTransform& HandLeft) { return true; };
	void ServerUpdateInputs_Implementation(const FNetworkTransform& Head, const FNetworkTransform& HandRight, const FNetworkTransform& HandLeft) {
		NT_InputHMD = Head; NT_InputHandRight = HandRight; NT_InputHandLeft = HandLeft;
		OnRep_InputHMD(); OnRep_InputHandRight(); OnRep_InputHandLeft();
	};

	// pack calc/input to networking structs
	UFUNCTION()
	void PackDataForReplication(const FTransform& Head, const FTransform& HandRight, const FTransform& HandLeft);
};