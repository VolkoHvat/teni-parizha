// Макет GameFramework/Pawn.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class AController; class APlayerState; class UPawnMovementComponent; class UInputComponent; class APlayerController;
class APawn : public AActor
{
public:
	typedef AActor Super; typedef APawn ThisClass; static UClass* StaticClass();
	APawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	uint32 bUseControllerRotationPitch : 1; uint32 bUseControllerRotationYaw : 1; uint32 bUseControllerRotationRoll : 1;
	uint32 bCanAffectNavigationGeneration : 1; uint32 bProcessingOutsideWorldBounds : 1;
	float BaseEyeHeight;
	TEnumAsByte<EAutoReceiveInput::Type> AutoPossessPlayer;
	EAutoPossessAI AutoPossessAI;
	TSubclassOf<AController> AIControllerClass;
	AController* Controller;
	AController* LastHitBy;
	virtual void PossessedBy(AController* NewController);
	virtual void UnPossessed();
	virtual void PawnClientRestart();
	virtual void Restart();
	virtual void SpawnDefaultController();
	virtual void DetachFromControllerPendingDestroy();
	virtual void TurnOff();
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime = 0.f);
	virtual FRotator GetControlRotation() const;
	virtual FRotator GetViewRotation() const;
	virtual FRotator GetBaseAimRotation() const;
	virtual FVector GetPawnViewLocation() const;
	virtual UPawnMovementComponent* GetMovementComponent() const;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue = 1.0f, bool bForce = false);
	virtual void AddControllerYawInput(float Val);
	virtual void AddControllerPitchInput(float Val);
	virtual void AddControllerRollInput(float Val);
	virtual bool IsMoveInputIgnored() const;
	virtual FVector GetNavAgentLocation() const;
	virtual bool IsLocallyControlled() const;
	virtual bool IsPlayerControlled() const;
	virtual bool IsBotControlled() const;
	virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
	virtual FVector GetVelocity() const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitializeComponents() override;
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	AController* GetController() const;
	template<class T> T* GetController() const { static_assert(sizeof(T) > 0, "GetController<T>: неполный тип"); return nullptr; }
	APlayerState* GetPlayerState() const;
	template<class T> T* GetPlayerState() const { return nullptr; }
	APlayerController* GetLocalViewingPlayerController() const;
	FVector GetPendingMovementInputVector() const; FVector GetLastMovementInputVector() const; FVector ConsumeMovementInputVector();
	void Internal_AddMovementInput(FVector WorldAccel, bool bForce = false);
	FVector Internal_GetPendingMovementInputVector() const; FVector Internal_GetLastMovementInputVector() const; FVector Internal_ConsumeMovementInputVector();
	void PawnMakeNoise(float Loudness, FVector NoiseLocation, bool bUseNoiseMakerLocation = true, AActor* NoiseMaker = nullptr);
	bool IsControlled() const;
	bool InFreeCam() const;
protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
	virtual UInputComponent* CreatePlayerInputComponent();
	virtual void DestroyPlayerInputComponent();
	FVector ControlInputVector; FVector LastControlInputVector;
};
