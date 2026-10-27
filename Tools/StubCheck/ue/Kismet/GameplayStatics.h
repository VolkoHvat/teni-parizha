// Макет Kismet/GameplayStatics.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/EngineTypes.h"
class APlayerController; class APawn; class ACharacter; class APlayerCameraManager; class AGameModeBase; class AGameStateBase; class UGameInstance; class USaveGame; class USoundBase;
class USoundAttenuation; class USoundConcurrency; class UAudioComponent; class UParticleSystem; class UParticleSystemComponent; class UDamageType; class AController; class AActor; class UInterface;
class UGameplayStatics : public UBlueprintFunctionLibrary
{
public:
	typedef UBlueprintFunctionLibrary Super; typedef UGameplayStatics ThisClass; static UClass* StaticClass();
	static UGameInstance* GetGameInstance(const UObject* WorldContextObject);
	static APlayerController* GetPlayerController(const UObject* WorldContextObject, int32 PlayerIndex);
	static APlayerController* GetPlayerControllerFromID(const UObject* WorldContextObject, int32 ControllerID);
	static ACharacter* GetPlayerCharacter(const UObject* WorldContextObject, int32 PlayerIndex);
	static APawn* GetPlayerPawn(const UObject* WorldContextObject, int32 PlayerIndex);
	static APlayerCameraManager* GetPlayerCameraManager(const UObject* WorldContextObject, int32 PlayerIndex);
	static AGameModeBase* GetGameMode(const UObject* WorldContextObject);
	static AGameStateBase* GetGameState(const UObject* WorldContextObject);
	static int32 GetNumPlayerControllers(const UObject* WorldContextObject);
	static void GetAllActorsOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, TArray<AActor*>& OutActors);
	static void GetAllActorsWithTag(const UObject* WorldContextObject, FName Tag, TArray<AActor*>& OutActors);
	static void GetAllActorsOfClassWithTag(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, FName Tag, TArray<AActor*>& OutActors);
	static void GetAllActorsWithInterface(const UObject* WorldContextObject, TSubclassOf<UInterface> Interface, TArray<AActor*>& OutActors);
	static AActor* GetActorOfClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass);
	static AActor* FindNearestActor(FVector Origin, const TArray<AActor*>& ActorsToCheck, float& Distance);
	static float ApplyDamage(AActor* DamagedActor, float BaseDamage, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	static float ApplyPointDamage(AActor* DamagedActor, float BaseDamage, FVector const& HitFromDirection, FHitResult const& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<UDamageType> DamageTypeClass);
	static bool ApplyRadialDamage(const UObject* WorldContextObject, float BaseDamage, const FVector& Origin, float DamageRadius, TSubclassOf<UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser = nullptr, AController* InstigatedByController = nullptr, bool bDoFullDamage = false, ECollisionChannel DamagePreventionChannel = ECC_Visibility);
	static void PlaySound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, AActor* OwningActor = nullptr, bool bIsUISound = true);
	static UAudioComponent* SpawnSound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true);
	static UAudioComponent* CreateSound2D(const UObject* WorldContextObject, USoundBase* Sound, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundConcurrency* ConcurrencySettings = nullptr, bool bPersistAcrossLevelTransition = false, bool bAutoDestroy = true);
	static void PlaySoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, AActor* OwningActor = nullptr);
	static void PlaySoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr);
	static UAudioComponent* SpawnSoundAtLocation(const UObject* WorldContextObject, USoundBase* Sound, FVector Location, FRotator Rotation = FRotator::ZeroRotator, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, class USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);
	static UAudioComponent* SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName = NAME_None, FVector Location = FVector(ForceInit), FRotator Rotation = FRotator::ZeroRotator, EAttachLocation::Type LocationType = EAttachLocation::KeepRelativeOffset, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);
	static UAudioComponent* SpawnSoundAttached(USoundBase* Sound, USceneComponent* AttachToComponent, FName AttachPointName, FVector Location, EAttachLocation::Type LocationType, bool bStopWhenAttachedToDestroyed = false, float VolumeMultiplier = 1.f, float PitchMultiplier = 1.f, float StartTime = 0.f, USoundAttenuation* AttenuationSettings = nullptr, USoundConcurrency* ConcurrencySettings = nullptr, bool bAutoDestroy = true);
	static bool SaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex);
	static USaveGame* LoadGameFromSlot(const FString& SlotName, const int32 UserIndex);
	static bool DoesSaveGameExist(const FString& SlotName, const int32 UserIndex);
	static bool DeleteGameInSlot(const FString& SlotName, const int32 UserIndex);
	static USaveGame* CreateSaveGameObject(TSubclassOf<USaveGame> SaveGameClass);
	static bool SaveGameToMemory(USaveGame* SaveGameObject, TArray<uint8>& OutSaveData);
	static USaveGame* LoadGameFromMemory(const TArray<uint8>& InSaveData);
	static float GetGlobalTimeDilation(const UObject* WorldContextObject);
	static void SetGlobalTimeDilation(const UObject* WorldContextObject, float TimeDilation);
	static bool SetGamePaused(const UObject* WorldContextObject, bool bPaused);
	static bool IsGamePaused(const UObject* WorldContextObject);
	static float GetTimeSeconds(const UObject* WorldContextObject); static float GetUnpausedTimeSeconds(const UObject* WorldContextObject);
	static float GetRealTimeSeconds(const UObject* WorldContextObject); static float GetAudioTimeSeconds(const UObject* WorldContextObject);
	static float GetWorldDeltaSeconds(const UObject* WorldContextObject);
	static void OpenLevel(const UObject* WorldContextObject, FName LevelName, bool bAbsolute = true, FString Options = FString(TEXT("")));
	static FString GetCurrentLevelName(const UObject* WorldContextObject, bool bRemovePrefixString = true);
	static bool ProjectWorldToScreen(APlayerController const* Player, const FVector& WorldPosition, FVector2D& ScreenPosition, bool bPlayerViewportRelative = false);
	static bool DeprojectScreenToWorld(APlayerController const* Player, const FVector2D& ScreenPosition, FVector& WorldPosition, FVector& WorldDirection);
	static FString GetPlatformName();
	static bool SuggestProjectileVelocity_CustomArc(const UObject* WorldContextObject, FVector& OutLaunchVelocity, FVector StartPos, FVector EndPos, float OverrideGravityZ = 0, float ArcParam = 0.5f);
	static void SetPlayerControllerID(APlayerController* Player, int32 ControllerId);
	static UObject* SpawnObject(TSubclassOf<UObject> ObjectClass, UObject* Outer);
	static AActor* BeginDeferredActorSpawnFromClass(const UObject* WorldContextObject, TSubclassOf<AActor> ActorClass, const FTransform& SpawnTransform, ESpawnActorCollisionHandlingMethod CollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::Undefined, AActor* Owner = nullptr);
	static AActor* FinishSpawningActor(AActor* Actor, const FTransform& SpawnTransform);
	static void SetBaseSoundMix(const UObject* WorldContextObject, class USoundMix* InSoundMix);
	static void PushSoundMixModifier(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier);
	static void SetSoundMixClassOverride(const UObject* WorldContextObject, class USoundMix* InSoundMixModifier, class USoundClass* InSoundClass, float Volume = 1.0f, float Pitch = 1.0f, float FadeInTime = 1.0f, bool bApplyToChildren = true);
	static void PlayWorldCameraShake(const UObject* WorldContextObject, TSubclassOf<class UCameraShakeBase> Shake, FVector Epicenter, float InnerRadius, float OuterRadius, float Falloff = 1.f, bool bOrientShakeTowardsEpicenter = false);
	static bool AreAnyListenersWithinRange(const UObject* WorldContextObject, const FVector& Location, float MaximumRange);
};
