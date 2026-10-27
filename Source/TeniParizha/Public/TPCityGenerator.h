// Тени Парижа — процедурный квартал Парижа 1789 года (фасады, карнизы, балконы, мансарды, мостовые, площадь).
// Владелец модуля: агент «город».
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPCityGenerator.generated.h"

class UProceduralMeshComponent;

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

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient) TArray<TObjectPtr<UProceduralMeshComponent>> Chunks;
	UPROPERTY() TArray<FVector> WalkPoints;
	UPROPERTY() TArray<FVector> LampPoints;
	UPROPERTY() TArray<FVector> HayPoints;
};
