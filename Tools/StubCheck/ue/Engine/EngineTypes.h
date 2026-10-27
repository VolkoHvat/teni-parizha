// Макет Engine/EngineTypes.h + EngineBaseTypes.h (UE 4.27). FHitResult здесь (в UE5 — Engine/HitResult.h).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"

class AActor; class UActorComponent; class USceneComponent; class UPrimitiveComponent; class UPhysicalMaterial; class UWorld; class UDamageType; class AController; class APawn;

// ---------- EngineBaseTypes ----------
enum ELevelTick { LEVELTICK_TimeOnly = 0, LEVELTICK_ViewportsOnly = 1, LEVELTICK_All = 2, LEVELTICK_PauseTick = 3 };
enum ETickingGroup { TG_PrePhysics, TG_StartPhysics, TG_DuringPhysics, TG_EndPhysics, TG_PostPhysics, TG_PostUpdateWork, TG_LastDemotable, TG_NewlySpawned, TG_MAX };
namespace EEndPlayReason { enum Type { Destroyed, LevelTransition, EndPlayInEditor, RemovedFromWorld, Quit }; }
enum ENetMode { NM_Standalone, NM_DedicatedServer, NM_ListenServer, NM_Client, NM_MAX };
enum ENetRole { ROLE_None, ROLE_SimulatedProxy, ROLE_AutonomousProxy, ROLE_Authority, ROLE_MAX };
enum class EMouseCaptureMode : uint8 { NoCapture, CapturePermanently, CapturePermanently_IncludingInitialMouseDown, CaptureDuringMouseDown, CaptureDuringRightMouseDown };
enum class EMouseLockMode : uint8 { DoNotLock, LockOnCapture, LockAlways, LockInFullscreen };
enum EInputEvent { IE_Pressed = 0, IE_Released = 1, IE_Repeat = 2, IE_DoubleClick = 3, IE_Axis = 4, IE_MAX = 5 };
namespace EAutoReceiveInput { enum Type { Disabled, Player0, Player1, Player2, Player3, Player4, Player5, Player6, Player7 }; }
enum class EAutoPossessAI : uint8 { Disabled, PlacedInWorld, Spawned, PlacedInWorldOrSpawned };

struct FTickFunction
{
	virtual ~FTickFunction();
	ETickingGroup TickGroup; ETickingGroup EndTickGroup;
	uint8 bTickEvenWhenPaused : 1; uint8 bCanEverTick : 1; uint8 bStartWithTickEnabled : 1; uint8 bAllowTickOnDedicatedServer : 1; uint8 bHighPriority : 1; uint8 bRunOnAnyThread : 1;
	float TickInterval;
	void SetTickFunctionEnable(bool bInEnabled); bool IsTickFunctionEnabled() const; bool IsTickFunctionRegistered() const;
	void AddPrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction);
	void RemovePrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction);
	void SetPriorityIncludingPrerequisites(bool bInHighPriority);
	void UpdateTickIntervalAndCoolDown(float NewTickInterval);
	virtual void ExecuteTick(float DeltaTime, ELevelTick TickType, ENamedThreads::Type CurrentThread, const class FGraphEventRef& MyCompletionGraphEvent);
	virtual FString DiagnosticMessage();
};
struct FActorTickFunction : public FTickFunction { AActor* Target; };
struct FActorComponentTickFunction : public FTickFunction { UActorComponent* Target; };

// ---------- EngineTypes ----------
UENUM() namespace EWorldType { enum Type { None, Game, Editor, PIE, EditorPreview, GamePreview, GameRPC, Inactive }; }
UENUM() namespace EComponentMobility { enum Type { Static, Stationary, Movable }; }
enum class ETeleportType : uint8 { None, TeleportPhysics, ResetPhysics };
enum class ESpawnActorCollisionHandlingMethod : uint8 { Undefined, AlwaysSpawn, AdjustIfPossibleButAlwaysSpawn, AdjustIfPossibleButDontSpawnIfColliding, DontSpawnIfColliding };
enum class EAttachmentRule : uint8 { KeepRelative, KeepWorld, SnapToTarget };
enum class EDetachmentRule : uint8 { KeepRelative, KeepWorld };
namespace EAttachLocation { enum Type { KeepRelativeOffset, KeepWorldPosition, SnapToTarget, SnapToTargetIncludingScale }; }
struct FAttachmentTransformRules
{
	static FAttachmentTransformRules KeepRelativeTransform; static FAttachmentTransformRules KeepWorldTransform;
	static FAttachmentTransformRules SnapToTargetNotIncludingScale; static FAttachmentTransformRules SnapToTargetIncludingScale;
	FAttachmentTransformRules(EAttachmentRule InRule, bool bInWeldSimulatedBodies);
	FAttachmentTransformRules(EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule, bool bInWeldSimulatedBodies);
	EAttachmentRule LocationRule; EAttachmentRule RotationRule; EAttachmentRule ScaleRule; bool bWeldSimulatedBodies;
};
struct FDetachmentTransformRules
{
	static FDetachmentTransformRules KeepRelativeTransform; static FDetachmentTransformRules KeepWorldTransform;
	FDetachmentTransformRules(EDetachmentRule InRule, bool bInCallModify);
	FDetachmentTransformRules(const FAttachmentTransformRules& AttachmentRules, bool bInCallModify);
	EDetachmentRule LocationRule; EDetachmentRule RotationRule; EDetachmentRule ScaleRule; bool bCallModify;
};

enum ECollisionChannel
{
	ECC_WorldStatic, ECC_WorldDynamic, ECC_Pawn, ECC_Visibility, ECC_Camera, ECC_PhysicsBody, ECC_Vehicle, ECC_Destructible,
	ECC_EngineTraceChannel1, ECC_EngineTraceChannel2, ECC_EngineTraceChannel3, ECC_EngineTraceChannel4, ECC_EngineTraceChannel5, ECC_EngineTraceChannel6,
	ECC_GameTraceChannel1, ECC_GameTraceChannel2, ECC_GameTraceChannel3, ECC_GameTraceChannel4, ECC_GameTraceChannel5, ECC_GameTraceChannel6,
	ECC_GameTraceChannel7, ECC_GameTraceChannel8, ECC_GameTraceChannel9, ECC_GameTraceChannel10, ECC_GameTraceChannel11, ECC_GameTraceChannel12,
	ECC_GameTraceChannel13, ECC_GameTraceChannel14, ECC_GameTraceChannel15, ECC_GameTraceChannel16, ECC_GameTraceChannel17, ECC_GameTraceChannel18,
	ECC_OverlapAll_Deprecated, ECC_MAX
};
#define ECC_TO_BITFIELD(x) (1 << (x))
enum ECollisionResponse { ECR_Ignore, ECR_Overlap, ECR_Block, ECR_MAX };
namespace ECollisionEnabled { enum Type { NoCollision, QueryOnly, PhysicsOnly, QueryAndPhysics }; }
enum EObjectTypeQuery { ObjectTypeQuery1, ObjectTypeQuery2, ObjectTypeQuery3, ObjectTypeQuery4, ObjectTypeQuery5, ObjectTypeQuery6, ObjectTypeQuery7, ObjectTypeQuery8, ObjectTypeQuery_MAX };
enum ETraceTypeQuery { TraceTypeQuery1, TraceTypeQuery2, TraceTypeQuery3, TraceTypeQuery4, TraceTypeQuery_MAX };
namespace EDrawDebugTrace { enum Type { None, ForOneFrame, ForDuration, Persistent }; }
enum EMovementMode { MOVE_None, MOVE_Walking, MOVE_NavWalking, MOVE_Falling, MOVE_Swimming, MOVE_Flying, MOVE_Custom, MOVE_MAX };
enum EPhysicalSurface { SurfaceType_Default, SurfaceType1, SurfaceType2, SurfaceType3, SurfaceType4, SurfaceType5, SurfaceType_Max };
enum ERadialImpulseFalloff { RIF_Constant, RIF_Linear, RIF_MAX };
enum ECanBeCharacterBase { ECB_No, ECB_Yes, ECB_Owner, ECB_MAX };
enum class ELightUnits : uint8 { Unitless, Candelas, Lumens };
enum EBlendMode { BLEND_Opaque, BLEND_Masked, BLEND_Translucent, BLEND_Additive, BLEND_Modulate, BLEND_AlphaComposite, BLEND_AlphaHoldout, BLEND_MAX };
enum EMaterialShadingModel { MSM_Unlit, MSM_DefaultLit, MSM_Subsurface, MSM_PreintegratedSkin, MSM_ClearCoat, MSM_SubsurfaceProfile, MSM_TwoSidedFoliage, MSM_Hair, MSM_Cloth, MSM_Eye, MSM_MAX };
enum EOcclusionCombineMode { OCM_Minimum, OCM_Multiply, OCM_MAX };
enum ESceneDepthPriorityGroup { SDPG_World, SDPG_Foreground, SDPG_MAX };
enum class EFlushLevelStreamingType : uint8 { None, Full, Visibility };
enum ERelativeTransformSpace { RTS_World, RTS_Actor, RTS_Component, RTS_ParentBoneSpace };

struct FCollisionResponseContainer
{
	FCollisionResponseContainer(); FCollisionResponseContainer(ECollisionResponse DefaultResponse);
	bool SetResponse(ECollisionChannel Channel, ECollisionResponse NewResponse); bool SetAllChannels(ECollisionResponse NewResponse);
	ECollisionResponse GetResponse(ECollisionChannel Channel) const;
	static const FCollisionResponseContainer& GetDefaultResponseContainer();
};
struct FWalkableSlopeOverride { FWalkableSlopeOverride(); };
struct FLightingChannels { FLightingChannels(); uint8 bChannel0 : 1; uint8 bChannel1 : 1; uint8 bChannel2 : 1; };

struct FHitResult
{
	int32 FaceIndex; float Time; float Distance;
	FVector_NetQuantize Location; FVector_NetQuantize ImpactPoint;
	FVector_NetQuantizeNormal Normal; FVector_NetQuantizeNormal ImpactNormal;
	FVector_NetQuantize TraceStart; FVector_NetQuantize TraceEnd;
	float PenetrationDepth; int32 Item; uint8 ElementIndex;
	uint8 bBlockingHit : 1; uint8 bStartPenetrating : 1;
	TWeakObjectPtr<UPhysicalMaterial> PhysMaterial;
	TWeakObjectPtr<AActor> Actor;
	TWeakObjectPtr<UPrimitiveComponent> Component;
	FName BoneName; FName MyBoneName; int32 MyItem;
	FHitResult(); explicit FHitResult(float InTime); explicit FHitResult(EForceInit); explicit FHitResult(ENoInit);
	FHitResult(class AActor* InActor, class UPrimitiveComponent* InComponent, FVector const& HitLoc, FVector const& HitNorm);
	void Init(); void Init(FVector Start, FVector End); void Reset(float InTime = 1.f, bool bPreserveTraceData = true);
	AActor* GetActor() const; UPrimitiveComponent* GetComponent() const;
	bool IsValidBlockingHit() const;
	static FHitResult GetReversedHit(const FHitResult& Hit);
	FString ToString() const;
};

struct FDamageEvent
{
	FDamageEvent(); FDamageEvent(TSubclassOf<UDamageType> InDamageTypeClass); virtual ~FDamageEvent();
	TSubclassOf<UDamageType> DamageTypeClass;
	static const int32 ClassID = 0; virtual int32 GetTypeID() const; bool IsOfType(int32 InID) const;
	virtual void GetBestHitInfo(AActor const* HitActor, AActor const* HitInstigator, FHitResult& OutHitInfo, FVector& OutImpulseDir) const;
};
struct FPointDamageEvent : public FDamageEvent
{
	float Damage; FVector_NetQuantizeNormal ShotDirection; FHitResult HitInfo;
	FPointDamageEvent(); FPointDamageEvent(float InDamage, FHitResult const& InHitInfo, FVector const& InShotDirection, TSubclassOf<UDamageType> InDamageTypeClass);
	static const int32 ClassID = 1;
};
struct FRadialDamageParams { float BaseDamage; float MinimumDamage; float InnerRadius; float OuterRadius; float DamageFalloff; };
struct FRadialDamageEvent : public FDamageEvent { FRadialDamageParams Params; FVector Origin; TArray<FHitResult> ComponentHits; static const int32 ClassID = 2; };

struct FTimerHandle
{
	FTimerHandle(); bool IsValid() const; void Invalidate();
	bool operator==(const FTimerHandle& Other) const; bool operator!=(const FTimerHandle& Other) const;
	FString ToString() const;
};
uint32 GetTypeHash(const FTimerHandle& H);
DECLARE_DELEGATE(FTimerDelegate);
DECLARE_DYNAMIC_DELEGATE(FTimerDynamicDelegate);

struct FRepMovement { FVector LinearVelocity; FVector AngularVelocity; FVector Location; FRotator Rotation; uint8 bSimulatedPhysicSleep : 1; uint8 bRepPhysics : 1; };
struct FComponentReference { FName ComponentProperty; TWeakObjectPtr<AActor> OtherActor; UActorComponent* GetComponent(AActor* OwningActor) const; };
struct FBasedPosition { FBasedPosition(); explicit FBasedPosition(class AActor* InBase, const FVector& InPosition); };

namespace EComponentSocketType { enum Type { Invalid, Bone, Socket }; }
enum class EVisibilityBasedAnimTickOption : uint8 { AlwaysTickPoseAndRefreshBones, AlwaysTickPose, OnlyTickMontagesWhenNotRendered, OnlyTickPoseWhenRendered };
namespace EAnimationMode { enum Type { AnimationBlueprint, AnimationSingleNode, AnimationCustomMode }; }
enum ENetDormancy { DORM_Never, DORM_Awake, DORM_DormantAll, DORM_DormantPartial, DORM_Initial, DORM_MAX };
