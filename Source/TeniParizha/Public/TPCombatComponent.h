// Тени Парижа — бой: серии ударов, блоки, контратаки по сигналу, уклонение, тихое устранение со спины.
// Владелец модуля: агент «бой и стража».
// Один и тот же компонент стоит у героя и у стражи (ATPGuard):
//  * у героя — автоприцел, выпад к цели, серии из трёх ударов, контратака, уклонение, восстановление, возрождение;
//  * у стражника — замах (WindUp, его можно контратаковать), удар, блоки, «пробитие защиты», оглушение.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPTypes.h"
#include "TPCombatComponent.generated.h"

class UAnimMontage;
class ATPGuard;
class UTPCombatDirector;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPDamaged, float, Amount, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTPDefeated, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTPRespawned);

UCLASS(ClassGroup = (TeniParizha), meta = (BlueprintSpawnableComponent))
class TENIPARIZHA_API UTPCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTPCombatComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** действие игрока или ИИ */
	UFUNCTION(BlueprintCallable, Category = "Бой") void RequestAction(ETPCombatAction Action, FVector Direction);
	/** получить удар; возвращает фактический урон (0 — заблокирован/уклонился) */
	UFUNCTION(BlueprintCallable, Category = "Бой") float ReceiveHit(float Damage, AActor* Attacker, bool bHeavy);
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsInCombat() const { return CombatWeight > 0.5f; }
	UFUNCTION(BlueprintPure, Category = "Бой") float GetHealth() const { return Health; }
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsDefeated() const { return Health <= 0.f; }
	/** противник замахивается и его можно контратаковать (для подсказки «Q») */
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsTelegraphing() const { return WindUp > 0.f; }
	UFUNCTION(BlueprintCallable, Category = "Бой") void ResetHealth() { Health = MaxHealth; }

	/* ---- дополнения (не ломают прежний интерфейс) ---- */

	/** нанести урон без проверки блока и уклонения (добивание, контратака, выстрел в упор) */
	UFUNCTION(BlueprintCallable, Category = "Бой") float ApplyHit(float Damage, AActor* Attacker, bool bHeavy);
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsAttacking() const { return AttackTime > 0.f; }
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsInvulnerable() const { return InvulnTime > 0.f; }
	/** защита пробита / после контратаки — «добить» обычным ударом */
	UFUNCTION(BlueprintPure, Category = "Бой") bool IsOpenForFinisher() const { return OpenTime > 0.f; }
	/** номер удара в серии 0..2 (2 — тяжёлый) */
	UFUNCTION(BlueprintPure, Category = "Бой") int32 GetComboIndex() const { return Combo; }
	UFUNCTION(BlueprintPure, Category = "Бой") AActor* GetAttackTarget() const;
	UFUNCTION(BlueprintPure, Category = "Бой") float GetHealthFraction() const { return MaxHealth > 0.f ? Health / MaxHealth : 0.f; }
	/** безопасная точка возрождения героя */
	UFUNCTION(BlueprintPure, Category = "Бой") FVector GetSafePoint() const { return SafePoint; }
	UFUNCTION(BlueprintCallable, Category = "Бой") void SetSafePoint(FVector Location, float Yaw);
	/** возрождение героя у безопасной точки (вызывается автоматически при Health <= 0) */
	UFUNCTION(BlueprintCallable, Category = "Бой") void Respawn();

	/** герой возродился (для затемнения экрана в интерфейсе) */
	UPROPERTY(BlueprintAssignable, Category = "Бой") FTPRespawned OnRespawned;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float AttackDamage = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> CounterMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> BlockMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> HitMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> DodgeMontage;

	/* ---- параметры баланса ---- */
	/** радиус автоприцела героя в бою / вне боя, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float TargetRange = 350.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float TargetRangeCalm = 260.f;
	/** дальность попадания удара, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float HitReach = 270.f;
	/** множители урона: лёгкий удар серии и третий (тяжёлый) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float LightMul = 0.9f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float HeavyMul = 1.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float CounterDamage = 38.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float FinisherDamage = 70.f;
	/** радиус поиска замахнувшегося противника для контратаки, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float CounterRange = 380.f;
	/** скорость уклонения, см/с; неуязвимость, с */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float DodgeSpeed = 750.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float DodgeInvuln = 0.45f;
	/** восстановление здоровья вне боя: ед./с и задержка после урона, с */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float RegenRate = 7.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float RegenDelay = 5.f;
	/** блок стражника: базовая вероятность и прибавка за каждый пропущенный удар */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float BlockChance = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float BlockChancePerHit = 0.22f;
	/** вероятность пробить блок тяжёлым (третьим) ударом */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float GuardBreakChance = 0.6f;
	/** длительность замаха стражника (окно контратаки), с */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float WindUpTime = 0.85f;
	/** дальность удара стражника, см */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Баланс") float StrikeReach = 260.f;

	UPROPERTY(BlueprintAssignable, Category = "Бой") FTPDamaged OnDamaged;
	UPROPERTY(BlueprintAssignable, Category = "Бой") FTPDefeated OnDefeated;

	/** замах (с), пока > 0 — удар можно контратаковать; выставляет ИИ стражи */
	float WindUp = 0.f;
	/** доля «боевой стойки» 0..1 */
	float CombatWeight = 0.f;

	/* ---- состояние (читают ИИ стражи и интерфейс) ---- */
	float AttackTime = 0.f;      // остаток текущего удара, с
	float AttackDuration = 0.4f;
	float ChainTime = 0.f;       // окно продолжения серии
	float InvulnTime = 0.f;      // неуязвимость (уклонение, контратака)
	float DodgeTime = 0.f;
	float ReactTime = 0.f;       // отдача после пропущенного удара / блока противника
	float StaggerTime = 0.f;     // оглушение (стража)
	float BlockTime = 0.f;       // стоит в блоке (стража)
	float OpenTime = 0.f;        // защита пробита — можно добить
	float Swing = 0.f;           // идёт удар после замаха (стража)
	int32 HitCount = 0;          // пропущенные удары подряд (стража)
	float HitCountTime = 0.f;

protected:
	ATPGuard* GetGuardOwner() const;
	UTPCombatDirector* GetDirector() const;
	bool CanActOnGround() const;
	AActor* FindTarget(const FVector& Direction, float Range, bool bUnawareOnly) const;
	void StartAttack(AActor* Target);
	void TickAttack(float DeltaTime);
	void ResolveAttack(AActor* Target);
	void TryCounter();
	void TryDodge(const FVector& Direction);
	void TickHero(float DeltaTime);
	void TickGuard(float DeltaTime);
	void GuardStrike();
	void HeroDefeated(AActor* Instigator);
	void FaceTowards(const FVector& Point, float DeltaTime, float Speed);
	void PlayMontage(UAnimMontage* Montage, float Rate = 1.f);

	TWeakObjectPtr<AActor> AttackTarget;
	int32 Combo = 0;
	bool bAttackHit = true;
	bool bQueuedAttack = false;
	float CombatHold = 0.f;      // задержка выхода из боевой стойки
	float RegenTimer = 0.f;
	float SafeTimer = 0.f;
	FVector SafePoint = FVector::ZeroVector;
	float SafeYaw = 0.f;
	bool bHasSafePoint = false;
	bool bRespawning = false;
};
