// Тени Парижа — свободный бег (паркур) в духе Assassin's Creed.
// Владелец модуля: агент «паркур». Публичный интерфейс ниже используют персонаж и другие системы — не менять без согласования.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPTypes.h"
#include "TPTraversalComponent.generated.h"

class ACharacter;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPTraversalStateChanged, ETPTraversal, OldState, ETPTraversal, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPLanded, float, FallHeightCm, bool, bSoftSurface);

/**
 * Компонент паркура. Персонаж каждый кадр передаёт ввод через SetInput(); компонент сам управляет
 * CharacterMovement (скорость, режим движения, положение в висе/лазании), находит стены, уступы и карнизы
 * трассировками по любой геометрии и проигрывает монтажи анимаций, если они заданы.
 */
UCLASS(ClassGroup = (TeniParizha), meta = (BlueprintSpawnableComponent))
class TENIPARIZHA_API UTPTraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTPTraversalComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** ввод этого кадра (однократные флаги сбрасываются после обработки) */
	void SetInput(const FTPMoveInput& In);

	UFUNCTION(BlueprintPure, Category = "Паркур") ETPTraversal GetState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "Паркур") const FTPLedge& GetLedge() const { return Ledge; }
	UFUNCTION(BlueprintPure, Category = "Паркур") bool IsFreeRunning() const { return Input.bSprint; }
	/** урон от падений, накопленный с прошлого вызова */
	float ConsumeDamage();
	/** телепорт (возрождение) */
	void ResetTo(const FVector& Location, float Yaw);

	UPROPERTY(BlueprintAssignable, Category = "Паркур") FTPTraversalStateChanged OnStateChanged;
	UPROPERTY(BlueprintAssignable, Category = "Паркур") FTPLanded OnLanded;

	/* ---- способности (открываются наградами) ---- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Способности") bool bAutoClimb = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Способности") bool bAutoVault = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Способности") bool bAutoLeap = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Способности") bool bLedgeGrab = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Способности") bool bRoll = true;

	/* ---- параметры (см, см/с) ---- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float WalkSpeed = 160.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float RunSpeed = 460.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float SprintSpeed = 720.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float ClimbSpeed = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float ShimmySpeed = 110.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float JumpVelocity = 620.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float LeapVelocity = 680.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float RollHeight = 320.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float HurtHeight = 700.f;

	/* ---- монтажи анимаций (необязательны: без них работает логика, поза — из AnimBP) ---- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageClimbUp;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageVault;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageRoll;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageHardLand;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageTurn;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageHangIdle;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageClimbLoop;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageShimmyLeft;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageShimmyRight;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageWallRun;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") TObjectPtr<UAnimMontage> MontageDropToHang;

	/** показывать отладочные трассировки */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") bool bDebugDraw = false;

protected:
	UPROPERTY(Transient) TObjectPtr<ACharacter> Owner;
	ETPTraversal State = ETPTraversal::Ground;
	FTPLedge Ledge;
	FTPMoveInput Input;
	float PendingDamage = 0.f;
	float StateTime = 0.f;
	void SetState(ETPTraversal NewState);
};
