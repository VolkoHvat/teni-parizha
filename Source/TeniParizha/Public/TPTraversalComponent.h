// Тени Парижа — свободный бег (паркур) в духе Assassin's Creed.
// Владелец модуля: агент «паркур». Публичный интерфейс ниже используют персонаж и другие системы — не менять без согласования.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPTypes.h"
#include "TPTraversalSensors.h"
#include "TPTraversalComponent.generated.h"

class ACharacter;
class UAnimMontage;
class UCharacterMovementComponent;

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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageClimbUp = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageVault = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageRoll = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageHardLand = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageTurn = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageHangIdle = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageClimbLoop = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageShimmyLeft = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageShimmyRight = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageWallRun = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageDropToHang = nullptr;

	/** показывать отладочные трассировки */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") bool bDebugDraw = false;

	/* ---- дополнительные параметры (агент «паркур») ---- */
	/** скорость крадущегося шага (присед), см/с */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float CrouchSpeed = 160.f;
	/** урон за каждый сантиметр падения выше HurtHeight */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур") float FallDamagePerCm = 0.12f;
	/** центр капсулы в висе ниже кромки уступа, см (руки манекена — у кромки) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Вис") float HangCenterBelowEdge = 115.f;
	/** отступ центра капсулы от стены в висе и при лазании, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Вис") float HangWallOffset = 40.f;
	/** половина расстояния между руками на кромке (для IK), см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Вис") float HandSpread = 22.f;

	/* длительности действий без монтажей, с (если монтаж задан — берётся его длительность) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float VaultTime = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float ClimbUpTime = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float DropTime = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float HopTime = 0.75f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float RollTime = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float TurnTime = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float LandSoftTime = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float LandHardTime = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Тайминги") float MantleTime = 0.6f;

	/** забраться с земли на невысокий уступ (~130–180 см), на который можно встать (необязателен) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Паркур|Анимации") UAnimMontage* MontageMantle = nullptr;

	/* ---- данные для анимации (UTPTraversalAnimInstance) ---- */
	/** скорость проигрывания цикла лазания (0 — стоит на стене) */
	UFUNCTION(BlueprintPure, Category = "Паркур|Анимация") float GetClimbAnimRate() const { return ClimbAnimRate; }
	/** -1 влево, +1 вправо (перебежка в висе / сдвиг при лазании) */
	UFUNCTION(BlueprintPure, Category = "Паркур|Анимация") float GetShimmyDir() const { return ShimmyDir; }
	/** наклон корпуса в повороте на бегу, градусы (+ вправо) */
	UFUNCTION(BlueprintPure, Category = "Паркур|Анимация") float GetLeanAngle() const { return LeanAngle; }
	UFUNCTION(BlueprintPure, Category = "Паркур|Анимация") bool IsCrouching() const { return Input.bCrouch; }
	/** высота последнего падения, см */
	UFUNCTION(BlueprintPure, Category = "Паркур") float GetLastFallHeight() const { return LastFallHeight; }
	/** мировые точки рук на кромке/стене; возвращает вес IK 0..1 */
	UFUNCTION(BlueprintPure, Category = "Паркур|Анимация") float GetHandIK(FVector& OutLeft, FVector& OutRight) const;

protected:
	UPROPERTY(Transient) ACharacter* Owner = nullptr;
	ETPTraversal State = ETPTraversal::Ground;
	FTPLedge Ledge;
	FTPMoveInput Input;
	float PendingDamage = 0.f;
	float StateTime = 0.f;
	void SetState(ETPTraversal NewState);

	/* ---- внутреннее состояние ---- */
	FTPTraversalSensors Sensors;
	float CapsuleRadius = 38.f;
	float CapsuleHalfHeight = 92.f;
	/** верхняя точка ног в полёте (для высоты падения) */
	float AirTopZ = 0.f;
	/** персонаж действительно оторвался от земли после толчка */
	bool bLeftGround = false;
	float LastFallHeight = 0.f;
	/** не хвататься снова за только что пройденный карниз (высота кромки) */
	float IgnoreTopZ = FTPTraversalSensors::NoFloor;
	/** кинематическое действие: откуда/куда (центр капсулы), поворот, длительность, высота дуги */
	FVector ActFrom = FVector::ZeroVector;
	FVector ActTo = FVector::ZeroVector;
	FQuat ActFromRot = FQuat::Identity;
	FQuat ActToRot = FQuat::Identity;
	float ActDur = 0.5f;
	float ActArc = 0.f;
	/** перемах закончится движением вперёд (false — запрыгнул на препятствие) */
	bool bActKeepSpeed = true;
	/** перемах-запрыгивание на широкое препятствие (сначала вверх, потом вперёд) */
	bool bActMount = false;
	/** подтягивание начато из виса (иначе — с земли) */
	bool bActFromHang = true;
	/** доли времени действия: подъём заканчивается в ActZEnd, перенос вперёд начинается в ActXStart */
	float ActZEnd = 0.65f;
	float ActXStart = 0.45f;
	FTPLedge NextLedge;
	float WallRunVz = 0.f;
	float TurnFromYaw = 0.f;
	float StateDuration = 0.f;
	/** анимационные величины */
	float ClimbAnimRate = 0.f;
	float ShimmyDir = 0.f;
	float LeanAngle = 0.f;
	FVector ClimbWallNormal = FVector::ZeroVector;

	/* ---- состояния ---- */
	void UpdateGround(float Dt);
	void UpdateAir(float Dt);
	void UpdateLand(float Dt);
	void UpdateRoll(float Dt);
	void UpdateTurn(float Dt);
	void UpdateClimb(float Dt);
	void UpdateHang(float Dt);
	void UpdateWallRun(float Dt);
	void UpdateClimbUp(float Dt);
	void UpdateVault(float Dt);
	void UpdateDrop(float Dt);
	void UpdateHop(float Dt);

	/* ---- переходы ---- */
	void EnterGround();
	void EnterAir(const FVector& Velocity, float GrabDelay = 0.f);
	void EnterJump(const FVector& LaunchVelocity, bool bOverrideXY);
	void EnterHang(const FTPLedge& L);
	void EnterClimb(const FVector& WallNormal);
	void StartHop(const FTPLedge& L, float Duration);
	bool TryStartVault(const FVector& Dir, float Reach);
	/** уступ найден с земли: высокий — перескок в вис, низкий — подтянуться сразу (если можно встать) */
	bool TryHangOrMantle(const FTPLedge& L);
	/** подтянуться на уступ L (из виса или с земли) */
	void StartClimbUp(const FTPLedge& L, bool bFromHang);
	/** прыжок из виса вбок к уступу на расстоянии до ~2 м (Side = ±1) */
	bool TryLateralHop(float Side);
	void HandleLanding();

	/* ---- вспомогательное ---- */
	UCharacterMovementComponent* GetMove() const;
	FVector GetFeet() const;
	FVector GetFacing() const;
	void HangTransform(const FTPLedge& L, FVector& OutCenter, FQuat& OutRot) const;
	void AttachLedge(const FTPLedge& L);
	void SetKinematic();
	void PlaceActor(const FVector& Center, const FQuat& Rot);
	bool EnsureCapsuleFree();
	float PlayMontage(UAnimMontage* Montage, float Rate = 1.f);
	void PlayLoop(UAnimMontage* Montage);
	void StopLoops(float BlendOut = 0.2f);
	/** проиграть монтаж действия (если задан) и вернуть длительность действия: его длину или Default */
	float ActionDuration(UAnimMontage* Montage, float Default);
};
