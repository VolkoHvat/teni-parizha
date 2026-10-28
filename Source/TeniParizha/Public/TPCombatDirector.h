// Тени Парижа — «режиссёр боя»: список стражи, жетон атаки (нападает только один стражник одновременно),
// общая тревога, свидетели, паника горожан, замедление времени на контратаке, расстановка стражи при старте.
// Владелец модуля: агент «бой и стража».
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "Tickable.h"
#include "TPCombatDirector.generated.h"

class ATPGuard;
class ACharacter;

/** сообщение для игрока («Вас заметили!», «Контратака! ЛКМ — добить» …) — интерфейс показывает его всплывающей строкой */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTPCombatMessage, const FString&, Text);
/** шум драки или выстрела: горожане в радиусе разбегаются */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPCombatPanic, FVector, Location, float, Radius);

// В UE 4.27 нет UTickableWorldSubsystem: тик даёт FTickableGameObject (CDO не тикает — см. GetTickableTickType)
UCLASS()
class TENIPARIZHA_API UTPCombatDirector : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/* ---- FTickableGameObject ---- */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

	/** вся стража уровня (для интерфейса и миникарты) */
	UFUNCTION(BlueprintPure, Category = "Стража") TArray<ATPGuard*> GetGuards() const;
	/** поднять тревогу: все стражники в радиусе бросаются в погоню */
	UFUNCTION(BlueprintCallable, Category = "Стража") void AlertAll(FVector Location, float Radius);
	/** происшествие в точке (герой сбил прохожего и т. п.): стражники, которые видят точку, настораживаются на Amount (0..1) */
	UFUNCTION(BlueprintCallable, Category = "Стража") void Witness(FVector Location, float Amount);
	/** шум драки — событие OnPanic для горожан */
	UFUNCTION(BlueprintCallable, Category = "Стража") void Panic(FVector Location, float Radius);
	/** сбросить тревогу (возрождение героя) */
	UFUNCTION(BlueprintCallable, Category = "Стража") void ResetAlert();
	/** замедление времени на RealSeconds реальных секунд */
	UFUNCTION(BlueprintCallable, Category = "Бой") void RequestSlowMo(float RealSeconds, float Dilation = 0.35f);
	/** укрытие героя: 1 — обычная заметность, 0.35 — в толпе, 0 — спрятан (сено). Действует Duration секунд, задавать каждый кадр/периодически. */
	UFUNCTION(BlueprintCallable, Category = "Стража") void SetHeroConcealment(float Scale, float Duration = 0.25f);
	/** итоговая заметность героя: укрытие × толпа (3+ жителя рядом — как в первой версии, ×0.35) */
	UFUNCTION(BlueprintPure, Category = "Стража") float GetHeroConcealment() const { return Concealment * CrowdScale; }
	/** герой затерялся в толпе (для подсказки в интерфейсе) */
	UFUNCTION(BlueprintPure, Category = "Стража") bool IsHeroInCrowd() const { return CrowdScale < 1.f; }
	UFUNCTION(BlueprintPure, Category = "Стража") int32 CountChasing() const;
	/** наибольший уровень тревоги среди стражи 0..1 (индикатор над головой/на экране) */
	UFUNCTION(BlueprintPure, Category = "Стража") float GetMaxAlert() const;
	/** ближайший замахнувшийся стражник (подсказка «E — контратака») */
	UFUNCTION(BlueprintPure, Category = "Стража") ATPGuard* GetTelegraphingGuard(FVector Location, float Radius) const;
	/** рядом преследующая стража или в героя целится мушкетёр (разрешает уклонение) */
	UFUNCTION(BlueprintPure, Category = "Стража") bool IsHeroThreatened(FVector Location, float Radius) const;
	bool IsGuardChasingNear(const FVector& Location, float Radius, float MaxHeight) const;

	/** сообщение игроку (повтор того же текста не чаще раза в 2.5 с); показывается через ATPHUD::Toast и OnCombatMessage */
	void Message(const FString& Text);

	/* ---- жетон атаки ---- */
	bool CanTakeAttackToken(const AActor* Who, bool bIgnoreCooldown = false) const;
	bool TryTakeAttackToken(AActor* Who, bool bIgnoreCooldown = false);
	/** отпустить жетон; Cooldown < 0 — не менять паузу до следующего нападения */
	void ReleaseAttackToken(const AActor* Who, float Cooldown);
	AActor* GetAttackTokenOwner() const { return TokenOwner.Get(); }

	void RegisterGuard(ATPGuard* Guard);
	void UnregisterGuard(ATPGuard* Guard);

	UPROPERTY(BlueprintAssignable, Category = "Стража") FTPCombatMessage OnCombatMessage;
	UPROPERTY(BlueprintAssignable, Category = "Стража") FTPCombatPanic OnPanic;

	/** сколько стражников создать при старте (консоль: tp.GuardCount N; -1 — случайно Min..Max, 0 — не создавать) */
	int32 MinGuards = 12;
	int32 MaxGuards = 16;

protected:
	void TrySpawnGuards();
	void UpdateCrowd();
	ACharacter* GetHero() const;

	TArray<TWeakObjectPtr<ATPGuard>> Guards;
	TWeakObjectPtr<AActor> TokenOwner;
	float TokenCooldown = 0.f;
	bool bSlowMo = false;
	double SlowMoEnd = 0.0;
	float Concealment = 1.f;
	double ConcealUntil = 0.0;
	float CrowdScale = 1.f;
	float CrowdTimer = 0.f;
	FTimerHandle SpawnTimer;
	int32 SpawnAttempts = 0;
	bool bSpawned = false;
	/** расстановка стражи запущена (ждём HasBegunPlay в Tick — так не зависим от OnWorldBeginPlay) */
	bool bSpawnScheduled = false;
	bool bInitialized = false;
	FString LastMessage;
	double LastMessageTime = -100.0;
};
