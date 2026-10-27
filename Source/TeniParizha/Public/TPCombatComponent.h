// Тени Парижа — бой: серии ударов, блоки, контратаки по сигналу, уклонение, тихое устранение со спины.
// Владелец модуля: агент «бой и стража».
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPTypes.h"
#include "TPCombatComponent.generated.h"

class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPDamaged, float, Amount, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTPDefeated, AActor*, Instigator);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float MaxHealth = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float Health = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой") float AttackDamage = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TArray<TObjectPtr<UAnimMontage>> AttackMontages;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> CounterMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> BlockMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> HitMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Бой|Анимации") TObjectPtr<UAnimMontage> DodgeMontage;

	UPROPERTY(BlueprintAssignable, Category = "Бой") FTPDamaged OnDamaged;
	UPROPERTY(BlueprintAssignable, Category = "Бой") FTPDefeated OnDefeated;

	/** замах (с), пока > 0 — удар можно контратаковать; выставляет ИИ стражи */
	float WindUp = 0.f;
	/** доля «боевой стойки» 0..1 */
	float CombatWeight = 0.f;
};
