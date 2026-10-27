// Тени Парижа — процедурный квартал Парижа 1789 года (фасады, карнизы, балконы, мансарды, мостовые, площадь).
// Владелец модуля: агент «город».
//
// Геометрия строится в TPCityBuilder (порт lu_city.c) и выгружается в несколько UProceduralMeshComponent-«ячеек» 24×24 м:
// в каждой ячейке по секции на материал (без столкновений) и одна невидимая секция столкновений из простых коробок
// (стены домов, карнизы-уступы, балконы, парапеты, скаты крыш) — по ней ходит CharacterMovement и трассирует паркур.
// Телеги с сеном — отдельный компонент с тегом "TPSoft". Текстуры генерируются на C++ (TPTexGen, порт lu_texgen.c).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPCityGenerator.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class TENIPARIZHA_API ATPCityGenerator : public AActor
{
	GENERATED_BODY()

public:
	ATPCityGenerator();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;

	/** построить (или перестроить) квартал */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Город") void Generate();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Город") int32 Seed = 1789;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Город") bool bGenerateOnConstruction = true;

	/** точка старта игрока (центр площади) и точки прогулок для горожан — в мире */
	UFUNCTION(BlueprintPure, Category = "Город") FVector GetPlayerStart() const;
	UFUNCTION(BlueprintPure, Category = "Город") const TArray<FVector>& GetWalkPoints() const { return WalkPoints; }
	UFUNCTION(BlueprintPure, Category = "Город") const TArray<FVector>& GetLampPoints() const { return LampPoints; }
	UFUNCTION(BlueprintPure, Category = "Город") const TArray<FVector>& GetHayPoints() const { return HayPoints; }

	/** яркость стёкол фонарей 0..1 (например, ночью 1, днём 0); материал общий для всех фонарей */
	UFUNCTION(BlueprintCallable, Category = "Город") void SetLampGlow(float Level);

	/** удалить сгенерированную геометрию */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Город") void ClearCity();

	/** размер процедурных текстур (степень двойки, 128..1024) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Город", meta = (ClampMin = "128", ClampMax = "1024")) int32 TextureSize = 512;

	/** статистика последней генерации */
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Город") int32 NumTriangles = 0;
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Город") int32 NumBuildings = 0;

	/** имя тега компонентов квартала и тега мягких предметов */
	static const FName TagCityChunk;
	static const FName TagSoft;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;
	UPROPERTY() TArray<FVector> WalkPoints;
	UPROPERTY() TArray<FVector> LampPoints;
	UPROPERTY() TArray<FVector> HayPoints;
	/** старт игрока в мире (сохраняется вместе с уровнем, чтобы был известен до генерации) */
	UPROPERTY() FVector PlayerStartPoint = FVector(6000.f, 5400.f, 6.f);

private:
	/** есть ли живая сгенерированная геометрия */
	bool HasCity() const;
	/** пересчитать мировые точки из локальных (после перемещения актора) */
	void UpdateWorldPoints();

	/** точки в локальных координатах актора (см) */
	TArray<FVector> LocalWalk, LocalLamps, LocalHay;
	FVector LocalStart = FVector(6000.f, 5400.f, 6.f);
	int32 GeneratedSeed = MIN_int32;
	int32 GeneratedTexSize = 0;
};
