// Макет CollisionQueryParams.h (UE 4.27, модуль Engine)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
class AActor; class UPrimitiveComponent;
#define SCENE_QUERY_STAT_ONLY(QueryName) TStatId()
#define SCENE_QUERY_STAT_NAME_ONLY(QueryName) FName(TEXT(#QueryName))
#define SCENE_QUERY_STAT(QueryName) SCENE_QUERY_STAT_NAME_ONLY(QueryName), SCENE_QUERY_STAT_ONLY(QueryName)
enum class EQueryMobilityType { Any, Static, Dynamic };
struct FCollisionQueryParams
{
	FName TraceTag; FName OwnerTag;
	bool bTraceComplex; bool bFindInitialOverlaps; bool bReturnFaceIndex; bool bReturnPhysicalMaterial; bool bIgnoreBlocks; bool bIgnoreTouches; bool bSkipNarrowPhase;
	EQueryMobilityType MobilityType;
	FCollisionQueryParams();
	FCollisionQueryParams(FName InTraceTag, bool bInTraceComplex = false, const AActor* InIgnoreActor = nullptr);
	FCollisionQueryParams(FName InTraceTag, const TStatId& InStatId, bool bInTraceComplex = false, const AActor* InIgnoreActor = nullptr);
	void AddIgnoredActor(const AActor* InIgnoreActor);
	void AddIgnoredActor(const uint32 InIgnoreActorID);
	void AddIgnoredActors(const TArray<AActor*>& InIgnoreActors);
	void AddIgnoredActors(const TArray<const AActor*>& InIgnoreActors);
	void AddIgnoredActors(const TArray<TWeakObjectPtr<const AActor>>& InIgnoreActors);
	void AddIgnoredComponent(const UPrimitiveComponent* InIgnoreComponent);
	void AddIgnoredComponents(const TArray<UPrimitiveComponent*>& InIgnoreComponents);
	void AddIgnoredComponent_LikelyDuplicatedRoot(const UPrimitiveComponent* InIgnoreComponent);
	void ClearIgnoredComponents(); void ClearIgnoredActors();
	FString ToString() const;
	static FCollisionQueryParams DefaultQueryParam;
};
struct FComponentQueryParams : public FCollisionQueryParams { FComponentQueryParams(); FComponentQueryParams(FName InTraceTag, const AActor* InIgnoreActor = nullptr); static FComponentQueryParams DefaultComponentQueryParams; };
struct FCollisionResponseParams
{
	FCollisionResponseContainer CollisionResponse;
	FCollisionResponseParams(ECollisionResponse DefaultResponse = ECR_Block); FCollisionResponseParams(const FCollisionResponseContainer& ResponseContainer);
	static FCollisionResponseParams DefaultResponseParam;
};
struct FCollisionObjectQueryParams
{
	enum InitType { AllObjects, AllStaticObjects, AllDynamicObjects };
	int32 ObjectTypesToQuery; int32 IgnoreMask;
	FCollisionObjectQueryParams(); FCollisionObjectQueryParams(ECollisionChannel QueryChannel); FCollisionObjectQueryParams(const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes);
	FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType QueryType); FCollisionObjectQueryParams(int32 InObjectTypesToQuery);
	void AddObjectTypesToQuery(ECollisionChannel QueryChannel); void RemoveObjectTypesToQuery(ECollisionChannel QueryChannel);
	int32 GetQueryBitfield() const; bool IsValid() const;
	static bool IsValidObjectQuery(ECollisionChannel QueryChannel);
	static FCollisionObjectQueryParams DefaultObjectQueryParam;
};
