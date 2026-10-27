// Тени Парижа — окружение: солнце и луна, атмосфера, облака, туман, небесный свет (Lumen), смена дня и ночи, уличные фонари.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPEnvironment.generated.h"

class UDirectionalLightComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;
class UExponentialHeightFogComponent;
class UVolumetricCloudComponent;
class UPointLightComponent;
class UPostProcessComponent;

UCLASS()
class TENIPARIZHA_API ATPEnvironment : public AActor
{
	GENERATED_BODY()
public:
	ATPEnvironment();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& T) override;

	/** время суток, часы 0..24 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Время") float TimeOfDay = 16.5f;
	/** сколько реальных минут длятся игровые сутки (0 — время стоит) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Время") float DayLengthMinutes = 48.f;
	UFUNCTION(BlueprintCallable, Category = "Время") void AddHours(float H) { TimeOfDay = FMath::Fmod(TimeOfDay + H + 24.f, 24.f); ApplyTime(); }
	UFUNCTION(BlueprintPure, Category = "Время") bool IsNight() const;

protected:
	void ApplyTime();
	void SpawnLamps();
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Sun;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UDirectionalLightComponent> Moon;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyAtmosphereComponent> Atmosphere;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USkyLightComponent> SkyLight;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UExponentialHeightFogComponent> Fog;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UVolumetricCloudComponent> Clouds;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPostProcessComponent> Post;
	UPROPERTY(Transient) TArray<TObjectPtr<UPointLightComponent>> Lamps;
	float LampLevel = -1.f;
};
