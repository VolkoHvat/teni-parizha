// Тени Парижа — отметки гонок Лиги крыш: светящиеся кольца-ворота, флажки и маяк вершины.
// Собраны из простых мешей /Engine/BasicShapes — без собственных ассетов.
// Владелец модуля: агент «жители, сюжет и интерфейс».
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TPStoryGate.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

UENUM()
enum class ETPGateKind : uint8
{
	Ring,   // кольцо-ворота
	Flag,   // флажок (охота за флажками)
	Summit  // вершина (восхождение)
};

UCLASS(NotBlueprintable)
class TENIPARIZHA_API ATPStoryGate : public AActor
{
	GENERATED_BODY()

public:
	ATPStoryGate();
	virtual void Tick(float DeltaSeconds) override;

	/** вид отметки и направление (кольцо разворачивается поперёк направления бега) */
	void Setup(ETPGateKind InKind, const FVector& FacingDir);
	/** 0 — скрыта, 1 — следующая (яркая, с лучом), 2 — после следующей (тусклая) */
	void SetHighlight(int32 Level);

protected:
	UPROPERTY(VisibleAnywhere) USceneComponent* Root = nullptr;
	UPROPERTY(VisibleAnywhere) USceneComponent* RingPivot = nullptr;
	UPROPERTY(VisibleAnywhere) TArray<UStaticMeshComponent*> Segments;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Pole = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Cloth = nullptr;
	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Beam = nullptr;
	UPROPERTY(VisibleAnywhere) UPointLightComponent* Glow = nullptr;
	UPROPERTY(Transient) UMaterialInstanceDynamic* MainMat = nullptr;
	UPROPERTY(Transient) UMaterialInstanceDynamic* BeamMat = nullptr;
	UPROPERTY(Transient) UMaterialInstanceDynamic* PoleMat = nullptr;

	ETPGateKind Kind = ETPGateKind::Ring;
	int32 Highlight = 1;
	float Age = 0.f;
	FLinearColor BaseColor = FLinearColor(1.f, .78f, .25f);

	void ApplyColors();
	static void SetColorParams(UMaterialInstanceDynamic* M, const FLinearColor& C, float Emissive);
};
