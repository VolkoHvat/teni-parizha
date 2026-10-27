// Макет GameFramework/Controller.h (UE 4.27): Possess() не виртуальный, переопределяют OnPossess().
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class APawn; class ACharacter; class APlayerState;
class AController : public AActor
{
public:
	typedef AActor Super; typedef AController ThisClass; static UClass* StaticClass();
	AController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	APlayerState* PlayerState;
	uint8 bAttachToPawn : 1; uint8 bIsPlayerController : 1;
	FName StateName;
	APawn* GetPawn() const;
	template<class T> T* GetPawn() const { static_assert(sizeof(T) > 0, "GetPawn<T>: неполный тип"); return nullptr; }
	ACharacter* GetCharacter() const;
	template<class T> T* GetPlayerState() const { return nullptr; }
	virtual FRotator GetControlRotation() const;
	virtual void SetControlRotation(const FRotator& NewRotation);
	virtual void SetInitialLocationAndRotation(const FVector& NewLocation, const FRotator& NewRotation);
	virtual FRotator GetDesiredRotation() const;
	void Possess(APawn* InPawn);
	void UnPossess();
	virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const;
	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const;
	virtual void StopMovement();
	virtual void SetIgnoreMoveInput(bool bNewMoveInput); virtual void ResetIgnoreMoveInput(); virtual bool IsMoveInputIgnored() const;
	virtual void SetIgnoreLookInput(bool bNewLookInput); virtual void ResetIgnoreLookInput(); virtual bool IsLookInputIgnored() const;
	virtual void ResetIgnoreInputFlags();
	bool IsLocalController() const; bool IsLocalPlayerController() const; bool IsPlayerController() const;
	virtual AActor* GetViewTarget() const;
	virtual void ChangeState(FName NewState); bool IsInState(FName InStateName) const; FName GetStateName() const;
	virtual void GameHasEnded(class AActor* EndGameFocus = nullptr, bool bIsWinner = false);
	virtual void FailedToSpawnPawn();
	virtual void InitPlayerState();
	virtual void CleanupPlayerState();
	virtual void PawnPendingDestroy(APawn* inPawn);
	virtual void SetPawn(APawn* InPawn);
	virtual void Reset() override;
	virtual void Destroyed() override;
	virtual void PostInitializeComponents() override;
protected:
	virtual void OnPossess(APawn* InPawn);
	virtual void OnUnPossess();
	FRotator ControlRotation;
};
