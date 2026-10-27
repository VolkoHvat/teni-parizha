// Макет NavigationSystem.h (UE 4.27, модуль NavigationSystem)
#pragma once
// TP_STUB_MODULE: NavigationSystem
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameFramework/NavMovementComponent.h"
class ANavigationData; class UNavigationQueryFilter; class UNavigationPath; class UWorld; class AActor;
typedef uint64 NavNodeRef;
struct FNavLocation { FVector Location; NavNodeRef NodeRef; FNavLocation(); explicit FNavLocation(const FVector& InLocation, NavNodeRef InNodeRef = 0); bool HasNodeRef() const; operator FVector() const; };
struct FNavigationQueryFilter;
typedef TSharedPtr<const FNavigationQueryFilter, ESPMode::ThreadSafe> FSharedConstNavQueryFilter;
#define INVALID_NAVEXTENT (FVector::ZeroVector)
class UNavigationSystemBase : public UObject { public: typedef UObject Super; static UClass* StaticClass(); };
class UNavigationSystemV1 : public UNavigationSystemBase
{
public:
	typedef UNavigationSystemBase Super; typedef UNavigationSystemV1 ThisClass; static UClass* StaticClass();
	UNavigationSystemV1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	template<typename TNavSys> static TNavSys* GetCurrent(UWorld* World) { return nullptr; }
	template<typename TNavSys> static TNavSys* GetCurrent(UObject* WorldContextObject) { return nullptr; }
	static UNavigationSystemV1* GetNavigationSystem(UObject* WorldContextObject);
	bool ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent = INVALID_NAVEXTENT, const FNavAgentProperties* AgentProperties = nullptr, FSharedConstNavQueryFilter QueryFilter = nullptr);
	bool GetRandomPoint(FNavLocation& ResultLocation, ANavigationData* NavData = nullptr, FSharedConstNavQueryFilter QueryFilter = nullptr);
	bool GetRandomReachablePointInRadius(const FVector& Origin, float Radius, FNavLocation& ResultLocation, ANavigationData* NavData = nullptr, FSharedConstNavQueryFilter QueryFilter = nullptr) const;
	bool GetRandomPointInNavigableRadius(const FVector& Origin, float Radius, FNavLocation& ResultLocation, ANavigationData* NavData = nullptr, FSharedConstNavQueryFilter QueryFilter = nullptr) const;
	static bool K2_ProjectPointToNavigation(UObject* WorldContextObject, const FVector& Point, FVector& ProjectedLocation, ANavigationData* NavData, TSubclassOf<UNavigationQueryFilter> FilterClass, const FVector QueryExtent = FVector::ZeroVector);
	static bool K2_GetRandomReachablePointInRadius(UObject* WorldContextObject, const FVector& Origin, FVector& RandomLocation, float Radius, ANavigationData* NavData = nullptr, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr);
	static UNavigationPath* FindPathToLocationSynchronously(UObject* WorldContextObject, const FVector& PathStart, const FVector& PathEnd, AActor* PathfindingContext = nullptr, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr);
	ANavigationData* GetDefaultNavDataInstance() const; ANavigationData* GetMainNavData() const;
	void Build(); bool IsNavigationBuilt(const class AWorldSettings* Settings) const; bool IsNavigationBuildInProgress();
	static void UpdateComponentInNavOctree(class UActorComponent& Comp);
	static void UpdateActorInNavOctree(AActor& Actor);
};
