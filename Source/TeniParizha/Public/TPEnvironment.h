// Тени Парижа — окружение: солнце и луна, атмосфера, высотный туман, небесный свет, смена дня и ночи, уличные фонари.
// Бюджет ПК 2014 года: солнце — единственный свет с тенями (2–3 каскада), без объёмных облаков и объёмного тумана,
// небесный свет пересъёмывается раз в игровой час (в сумерках — раз в четверть часа), горят 8 ближайших фонарей без теней.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UPointLightComponent;
class UPostProcessComponent;
class ATPCityGenerator;

UCLASS()
class TENIPARIZHA_API ATPEnvironment : public AActor
{
	GENERATED_BODY()
public:
	ATPEnvironment();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	/** время суток, часы 0..24 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Время") float TimeOfDay = 16.5f;
	/** сколько реальных минут длятся игровые сутки (0 — время стоит) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Время") float DayLengthMinutes = 48.f;
	UFUNCTION(BlueprintCallable, Category = "Время") void AddHours(float H) { TimeOfDay = FMath::Fmod(TimeOfDay + H + 24.f, 24.f); ApplyTime(true); }
	UFUNCTION(BlueprintPure, Category = "Время") bool IsNight() const;

	/** сколько фонарей одновременно светят настоящим светом (остальные — только светящееся стекло) */
	UPROPERTY(EditAnywhere, Category = "Фонари", meta = (ClampMin = "0", ClampMax = "8")) int32 MaxLiveLamps = 8;

protected:
	/** применить время суток; bForce — без порога (после ручной смены времени) */
	void ApplyTime(bool bForce = false);
	void CreateLampPool();
	/** выбрать ближайшие к камере фонари и плавно погасить дальние */
	void UpdateLamps();
	ATPCityGenerator* FindCity();
	FVector GetViewPoint() const;

	UPROPERTY(VisibleAnywhere) USceneComponent* Root = nullptr;
	UPROPERTY(VisibleAnywhere) UDirectionalLightComponent* Sun = nullptr;
	UPROPERTY(VisibleAnywhere) UDirectionalLightComponent* Moon = nullptr;
	UPROPERTY(VisibleAnywhere) USkyAtmosphereComponent* Atmosphere = nullptr;
	UPROPERTY(VisibleAnywhere) USkyLightComponent* SkyLight = nullptr;
	UPROPERTY(VisibleAnywhere) UExponentialHeightFogComponent* Fog = nullptr;
	UPROPERTY(VisibleAnywhere) UPostProcessComponent* Post = nullptr;
	/** пул живых фонарей: переезжают к ближайшим точкам города */
	UPROPERTY(Transient) TArray<UPointLightComponent*> Lamps;
	UPROPERTY(Transient) TWeakObjectPtr<ATPCityGenerator> City;

	/** индекс точки фонаря у каждого света пула (-1 — свободен) */
	TArray<int32> LampPointOf;
	/** последняя яркость каждого света пула (чтобы не дёргать рендер без нужды) */
	TArray<float> LampLastIntensity;
	float LampLevel = -1.f;
	/** время, при котором свет последний раз применялся, и час последней пересъёмки неба */
	float AppliedTime = -100.f;
	float CaptureSlot = -100.f;
};
