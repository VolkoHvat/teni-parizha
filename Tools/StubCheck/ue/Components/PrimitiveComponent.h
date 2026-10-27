// Макет Components/PrimitiveComponent.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "PhysicsEngine/BodyInstance.h"
class UMaterialInterface; class UMaterialInstanceDynamic; class UPhysicalMaterial; class UPrimitiveComponent; class UBodySetup;
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams(FComponentHitSignature, UPrimitiveComponent, OnComponentHit, UPrimitiveComponent*, HitComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, FVector, NormalImpulse, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SixParams(FComponentBeginOverlapSignature, UPrimitiveComponent, OnComponentBeginOverlap, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep, const FHitResult&, SweepResult);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentEndOverlapSignature, UPrimitiveComponent, OnComponentEndOverlap, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentOnClickedSignature, UPrimitiveComponent, OnClicked, UPrimitiveComponent*, TouchedComponent, struct FKey, ButtonPressed);
class UPrimitiveComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef UPrimitiveComponent ThisClass; static UClass* StaticClass();
	UPrimitiveComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float MinDrawDistance; float LDMaxDrawDistance; float CachedMaxDrawDistance;
	TEnumAsByte<ESceneDepthPriorityGroup> DepthPriorityGroup;
	uint8 bAlwaysCreatePhysicsState : 1; uint8 bMultiBodyOverlap : 1; uint8 bReturnMaterialOnMove : 1; uint8 bAllowCullDistanceVolume : 1;
	uint8 bVisibleInReflectionCaptures : 1; uint8 bVisibleInRealTimeSkyCaptures : 1; uint8 bVisibleInRayTracing : 1; uint8 bRenderInMainPass : 1; uint8 bRenderInDepthPass : 1;
	uint8 bReceivesDecals : 1; uint8 bOwnerNoSee : 1; uint8 bOnlyOwnerSee : 1; uint8 bTreatAsBackgroundForOcclusion : 1; uint8 bUseAsOccluder : 1; uint8 bSelectable : 1;
	uint8 CastShadow : 1; uint8 bAffectDynamicIndirectLighting : 1; uint8 bAffectDistanceFieldLighting : 1; uint8 bCastDynamicShadow : 1; uint8 bCastStaticShadow : 1;
	uint8 bCastVolumetricTranslucentShadow : 1; uint8 bSelfShadowOnly : 1; uint8 bCastFarShadow : 1; uint8 bCastInsetShadow : 1; uint8 bCastCinematicShadow : 1;
	uint8 bCastHiddenShadow : 1; uint8 bCastShadowAsTwoSided : 1; uint8 bLightAttachmentsAsGroup : 1; uint8 bReceiveMobileCSMShadows : 1; uint8 bSingleSampleShadowFromStationaryLights : 1;
	uint8 bIgnoreRadialImpulse : 1; uint8 bIgnoreRadialForce : 1; uint8 bApplyImpulseOnDamage : 1; uint8 bRenderCustomDepth : 1; uint8 bHasPerInstanceHitProxies : 1;
	uint8 AlwaysLoadOnClient : 1; uint8 AlwaysLoadOnServer : 1; uint8 bUseEditorCompositing : 1;
	int32 CustomDepthStencilValue; int32 TranslucencySortPriority; int32 VisibilityId; float BoundsScale;
	FLightingChannels LightingChannels;
	TEnumAsByte<enum ECanBeCharacterBase> CanCharacterStepUpOn;
	FBodyInstance BodyInstance;
	FComponentHitSignature OnComponentHit;
	FComponentBeginOverlapSignature OnComponentBeginOverlap;
	FComponentEndOverlapSignature OnComponentEndOverlap;
	FComponentOnClickedSignature OnClicked;

	virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType);
	virtual void SetCollisionProfileName(FName InCollisionProfileName, bool bUpdateOverlaps = true);
	FName GetCollisionProfileName() const;
	virtual void SetCollisionObjectType(ECollisionChannel Channel);
	void SetCollisionResponseToChannel(ECollisionChannel Channel, ECollisionResponse NewResponse);
	void SetCollisionResponseToAllChannels(ECollisionResponse NewResponse);
	void SetCollisionResponseToChannels(const FCollisionResponseContainer& NewReponses);
	virtual ECollisionEnabled::Type GetCollisionEnabled() const override;
	virtual ECollisionResponse GetCollisionResponseToChannel(ECollisionChannel Channel) const override;
	virtual ECollisionChannel GetCollisionObjectType() const override;
	const FCollisionResponseContainer& GetCollisionResponseToChannels() const;
	bool IsCollisionEnabled() const; bool IsQueryCollisionEnabled() const; bool IsPhysicsCollisionEnabled() const;
	void SetGenerateOverlapEvents(bool bInGenerateOverlapEvents); bool GetGenerateOverlapEvents() const;
	virtual void SetNotifyRigidBodyCollision(bool bNewNotifyRigidBodyCollision);
	void IgnoreActorWhenMoving(AActor* Actor, bool bShouldIgnore);
	void IgnoreComponentWhenMoving(UPrimitiveComponent* Component, bool bShouldIgnore);
	void ClearMoveIgnoreActors(); void ClearMoveIgnoreComponents();
	TArray<AActor*> CopyArrayOfMoveIgnoreActors();
	void GetOverlappingActors(TArray<AActor*>& OverlappingActors, TSubclassOf<AActor> ClassFilter = nullptr) const;
	void GetOverlappingActors(TSet<AActor*>& OverlappingActors, TSubclassOf<AActor> ClassFilter = nullptr) const;
	void GetOverlappingComponents(TArray<UPrimitiveComponent*>& OutOverlappingComponents) const;
	bool IsOverlappingActor(const AActor* Other) const; bool IsOverlappingComponent(const UPrimitiveComponent* OtherComp) const;
	virtual bool LineTraceComponent(FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionQueryParams& Params);
	virtual bool SweepComponent(FHitResult& OutHit, const FVector Start, const FVector End, const FQuat& ShapeWorldRotation, const FCollisionShape& CollisionShape, bool bTraceComplex = false);

	virtual void SetSimulatePhysics(bool bSimulate);
	void SetEnableGravity(bool bGravityEnabled); bool IsGravityEnabled() const;
	virtual void AddImpulse(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);
	virtual void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None);
	virtual void AddRadialImpulse(FVector Origin, float Radius, float Strength, enum ERadialImpulseFalloff Falloff, bool bVelChange = false);
	virtual void AddForce(FVector Force, FName BoneName = NAME_None, bool bAccelChange = false);
	virtual void AddForceAtLocation(FVector Force, FVector Location, FName BoneName = NAME_None);
	virtual void AddTorqueInRadians(FVector Torque, FName BoneName = NAME_None, bool bAccelChange = false);
	virtual void AddAngularImpulseInRadians(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);
	virtual void AddAngularImpulseInDegrees(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false);
	virtual void SetPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	FVector GetPhysicsLinearVelocity(FName BoneName = NAME_None);
	FVector GetPhysicsLinearVelocityAtPoint(FVector Point, FName BoneName = NAME_None);
	virtual void SetAllPhysicsLinearVelocity(FVector NewVel, bool bAddToCurrent = false);
	virtual void SetPhysicsAngularVelocityInDegrees(FVector NewAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	virtual void SetPhysicsAngularVelocityInRadians(FVector NewAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	FVector GetPhysicsAngularVelocityInDegrees(FName BoneName = NAME_None) const;
	virtual void SetPhysicsMaxAngularVelocityInDegrees(float NewMaxAngVel, bool bAddToCurrent = false, FName BoneName = NAME_None);
	virtual void WakeAllRigidBodies();
	void WakeRigidBody(FName BoneName = NAME_None);
	void PutRigidBodyToSleep(FName BoneName = NAME_None);
	bool RigidBodyIsAwake(FName BoneName = NAME_None) const;
	void SetMassOverrideInKg(FName BoneName = NAME_None, float MassInKg = 1.f, bool bOverrideMass = true);
	float GetMass() const;
	void SetMassScale(FName BoneName = NAME_None, float InMassScale = 1.f);
	void SetLinearDamping(float InDamping); float GetLinearDamping() const;
	void SetAngularDamping(float InDamping); float GetAngularDamping() const;
	virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial);
	virtual FBodyInstance* GetBodyInstance(FName BoneName = NAME_None, bool bGetWelded = true) const;
	virtual UBodySetup* GetBodySetup();

	virtual int32 GetNumMaterials() const;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const;
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material);
	virtual void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material);
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const;
	virtual UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamic(int32 ElementIndex);
	virtual UMaterialInstanceDynamic* CreateAndSetMaterialInstanceDynamicFromMaterial(int32 ElementIndex, UMaterialInterface* Parent);
	virtual UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial = nullptr, FName OptionalName = NAME_None);
	virtual int32 GetMaterialIndex(FName MaterialSlotName) const;
	virtual TArray<FName> GetMaterialSlotNames() const;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const;

	void SetCastShadow(bool NewCastShadow);
	void SetCastInsetShadow(bool bInCastInsetShadow);
	void SetSingleSampleShadowFromStationaryLights(bool bNewSingleSampleShadowFromStationaryLights);
	void SetRenderCustomDepth(bool bValue);
	void SetCustomDepthStencilValue(int32 Value);
	void SetRenderInMainPass(bool bValue);
	void SetOwnerNoSee(bool bNewOwnerNoSee); void SetOnlyOwnerSee(bool bNewOnlyOwnerSee);
	void SetReceivesDecals(bool bNewReceivesDecals);
	void SetCullDistance(float NewCullDistance);
	void SetLightingChannels(bool bChannel0, bool bChannel1, bool bChannel2);
	void SetTranslucentSortPriority(int32 NewTranslucentSortPriority);
	void SetBoundsScale(float NewBoundsScale = 1.f);
	void SetDepthPriorityGroup(ESceneDepthPriorityGroup NewDepthPriorityGroup);
	void SetCachedMaxDrawDistance(const float NewCachedMaxDrawDistance);
	void SetVisibleInRayTracing(bool bNewVisibleInRayTracing);
	float GetLastRenderTime() const; float GetLastRenderTimeOnScreen() const;
	bool WasRecentlyRendered(float Tolerance = 0.2f) const;
	void SetCustomPrimitiveDataFloat(int32 DataIndex, float Value);
	void SetCustomPrimitiveDataVector4(int32 DataIndex, FVector4 Value);
	void SetScalarParameterForCustomPrimitiveData(FName ParameterName, float Value);
	void SetVectorParameterForCustomPrimitiveData(FName ParameterName, FVector4 Value);
	bool CanCharacterStepUp(class APawn* Pawn) const;
	virtual bool CanEditSimulatePhysics();
	virtual void PutAllRigidBodiesToSleep();
	void SetAllMassScale(float InMassScale = 1.f);
	void SetUseCCD(bool InUseCCD, FName BoneName = NAME_None);
	void SetAllUseCCD(bool InUseCCD);
};

