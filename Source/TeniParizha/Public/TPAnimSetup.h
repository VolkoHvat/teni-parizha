// Тени Парижа — настройка анимации мешей: класс UTPAnimInstance и оптимизация толпы (стража, жители).
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TPAnimSetup.generated.h"

class USkeletalMeshComponent;
struct FAnimUpdateRateParameters;

UCLASS()
class TENIPARIZHA_API UTPAnimSetup : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** назначает мешу UTPAnimInstance (если у меша ещё нет класса анимации или bReplace) */
	UFUNCTION(BlueprintCallable, Category = "Анимация")
	static void ApplyAnimation(USkeletalMeshComponent* Mesh, bool bReplace = false);

	/** герой: поза считается всегда (камера и паркур опираются на кости), без пропуска кадров */
	UFUNCTION(BlueprintCallable, Category = "Анимация")
	static void ConfigureHero(USkeletalMeshComponent* Mesh);

	/**
	 * NPC: поза только когда меш виден (OnlyTickPoseWhenRendered), редкое обновление мелких на экране
	 * (URO: каждый 2–4-й кадр с интерполяцией), невидимые — раз в 8 кадров, без ткани и без пересчёта границ.
	 * Вызывать в конструкторе актёра или до регистрации меша; после регистрации тоже работает.
	 */
	UFUNCTION(BlueprintCallable, Category = "Анимация")
	static void ConfigureCrowd(USkeletalMeshComponent* Mesh);

	/** пороги редкого обновления толпы (вызывается движком при создании параметров URO) */
	static void ApplyCrowdUpdateRate(FAnimUpdateRateParameters* Params);
};
