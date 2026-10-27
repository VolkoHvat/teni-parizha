// Макет Engine/World.h (UE 4.27): трассировки, спавн, таймеры, подсистемы.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "GameFramework/Actor.h"
class APlayerController; class AGameModeBase; class AGameStateBase; class UGameInstance; class ULevel; class AWorldSettings; class FTimerManager; class UNavigationSystemBase; class ULineBatchComponent; class FSceneInterface; class ULocalPlayer; class UWorldSubsystem; class APawn;
struct FActorSpawnParameters
{
	FActorSpawnParameters();
	FName Name; AActor* Template; AActor* Owner; APawn* Instigator; ULevel* OverrideLevel;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;
	uint8 bNoFail : 1; uint8 bDeferConstruction : 1; uint8 bAllowDuringConstructionScript : 1;
	enum class ESpawnActorNameMode : uint8 { Required_Fatal, Required_ErrorAndReturnNull, Required_ReturnNull, Requested };
	ESpawnActorNameMode NameMode;
	EObjectFlags ObjectFlags;
	bool IsRemoteOwned() const;
};
typedef TArray<TWeakObjectPtr<APlayerController>>::TConstIterator FConstPlayerControllerIterator;
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActorSpawned, AActor*);
class UWorld : public UObject
{
public:
	typedef UObject Super; typedef UWorld ThisClass; static UClass* StaticClass();
	UWorld(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	ULevel* PersistentLevel; ULineBatchComponent* LineBatcher; ULineBatchComponent* PersistentLineBatcher; ULineBatchComponent* ForegroundLineBatcher;
	TEnumAsByte<EWorldType::Type> WorldType;
	FSceneInterface* Scene;
	float TimeSeconds; float UnpausedTimeSeconds; float RealTimeSeconds; float AudioTimeSeconds; float DeltaTimeSeconds; float PauseDelay;
	uint8 bIsTearingDown : 1; uint8 bDebugDrawAllTraceTags : 1; uint8 bShouldSimulatePhysics : 1;
	FName DebugDrawTraceTag;

	float GetTimeSeconds() const; float GetUnpausedTimeSeconds() const; float GetRealTimeSeconds() const; float GetAudioTimeSeconds() const; float GetDeltaSeconds() const;
	float TimeSince(float Time) const;
	APlayerController* GetFirstPlayerController() const;
	template<class T> T* GetFirstPlayerController() const { static_assert(sizeof(T) > 0, "GetFirstPlayerController<T>: неполный тип"); return nullptr; }
	ULocalPlayer* GetFirstLocalPlayerFromController() const;
	FConstPlayerControllerIterator GetPlayerControllerIterator() const;
	int32 GetNumPlayerControllers() const;
	UGameInstance* GetGameInstance() const;
	template<class T> T* GetGameInstance() const { static_assert(sizeof(T) > 0, "GetGameInstance<T>: неполный тип"); return nullptr; }
	AGameModeBase* GetAuthGameMode() const;
	template<class T> T* GetAuthGameMode() const { static_assert(sizeof(T) > 0, "GetAuthGameMode<T>: неполный тип"); return nullptr; }
	AGameStateBase* GetGameState() const;
	template<class T> T* GetGameState() const { return nullptr; }
	AWorldSettings* GetWorldSettings(bool bCheckStreamingPersistent = false, bool bChecked = true) const;
	FTimerManager& GetTimerManager() const;
	template<class TSubsystemClass> TSubsystemClass* GetSubsystem() const { static_assert(sizeof(TSubsystemClass) > 0, "GetSubsystem<T>: неполный тип"); return nullptr; }
	UNavigationSystemBase* GetNavigationSystem();
	const UNavigationSystemBase* GetNavigationSystem() const;
	float GetGravityZ() const; float GetDefaultGravityZ() const;
	bool IsGameWorld() const; bool IsEditorWorld() const; bool IsPreviewWorld() const; bool IsPlayInEditor() const; bool IsPlayInPreview() const;
	bool IsPaused() const; bool IsCameraMoveable() const; bool HasBegunPlay() const; bool AreActorsInitialized() const;
	ENetMode GetNetMode() const; bool IsNetMode(ENetMode Mode) const;
	ULevel* GetCurrentLevel() const; const TArray<ULevel*>& GetLevels() const; FString GetMapName() const;
	int32 GetActorCount();
	bool DestroyActor(AActor* Actor, bool bNetForce = false, bool bShouldModifyLevel = true);
	bool EncroachingBlockingGeometry(const AActor* TestActor, FVector TestLocation, FRotator TestRotation, FVector* ProposedAdjustment = nullptr);
	bool FindTeleportSpot(const AActor* TestActor, FVector& PlaceLocation, FRotator PlaceRotation);
	FDelegateHandle AddOnActorSpawnedHandler(const FOnActorSpawned::FDelegate& InHandler);
	void RemoveOnActorSpawnedHandler(FDelegateHandle InHandle);
	bool ServerTravel(const FString& InURL, bool bAbsolute = false, bool bShouldSkipGameNotify = false);
	void SendAllEndOfFrameUpdates();

	AActor* SpawnActor(UClass* Class, FVector const* Location = nullptr, FRotator const* Rotation = nullptr, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	AActor* SpawnActor(UClass* Class, FTransform const* Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	AActor* SpawnActorAbsolute(UClass* Class, FTransform const& AbsoluteTransform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template<class T> T* SpawnActor(const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActor<T>: T должен быть актором (нужен #include)"); return nullptr; }
	template<class T> T* SpawnActor(FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActor<T>: T должен быть актором (нужен #include)"); return nullptr; }
	template<class T> T* SpawnActor(UClass* Class, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActor<T>: T должен быть актором (нужен #include)"); return nullptr; }
	template<class T> T* SpawnActor(UClass* Class, FVector const& Location, FRotator const& Rotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActor<T>: T должен быть актором (нужен #include)"); return nullptr; }
	template<class T> T* SpawnActor(UClass* Class, FTransform const& Transform, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters()) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActor<T>: T должен быть актором (нужен #include)"); return nullptr; }
	template<class T> T* SpawnActorAbsolute(FVector const& AbsoluteLocation, FRotator const& AbsoluteRotation, const FActorSpawnParameters& SpawnParameters = FActorSpawnParameters());
	template<class T> T* SpawnActorDeferred(UClass* Class, FTransform const& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined) { static_assert(std::is_base_of<AActor, T>::value, "SpawnActorDeferred<T>: T должен быть актором (нужен #include)"); return nullptr; }

	bool LineTraceTestByChannel(const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool LineTraceTestByObjectType(const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceTestByProfile(const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceSingleByChannel(struct FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool LineTraceSingleByObjectType(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceSingleByProfile(struct FHitResult& OutHit, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceMultiByChannel(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool LineTraceMultiByObjectType(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool LineTraceMultiByProfile(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool SweepTestByChannel(const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepTestByObjectType(const FVector& Start, const FVector& End, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool SweepSingleByChannel(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepSingleByObjectType(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool SweepSingleByProfile(struct FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rot, FName ProfileName, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool SweepMultiByChannel(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool SweepMultiByObjectType(TArray<struct FHitResult>& OutHits, const FVector& Start, const FVector& End, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool OverlapBlockingTestByChannel(const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapAnyTestByChannel(const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapAnyTestByObjectType(const FVector& Pos, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool OverlapMultiByChannel(TArray<struct FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, ECollisionChannel TraceChannel, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam) const;
	bool OverlapMultiByObjectType(TArray<struct FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	bool OverlapMultiByProfile(TArray<struct FOverlapResult>& OutOverlaps, const FVector& Pos, const FQuat& Rot, FName ProfileName, const FCollisionShape& CollisionShape, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam) const;
	FTraceHandle AsyncLineTraceByChannel(EAsyncTraceType InTraceType, const FVector& Start, const FVector& End, ECollisionChannel TraceChannel, const FCollisionQueryParams& Params = FCollisionQueryParams::DefaultQueryParam, const FCollisionResponseParams& ResponseParam = FCollisionResponseParams::DefaultResponseParam, FTraceDelegate* InDelegate = nullptr, uint32 UserData = 0);
	virtual UWorld* GetWorld() const override;
};
