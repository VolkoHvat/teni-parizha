// Тени Парижа — одежда 1789 года на скелете манекена.
// Части наряда (TPOutfitGen.h) строятся в UStaticMesh один раз на наряд (кэш) и цепляются к костям скелетного меша;
// сам манекен скрывается и служит только каркасом анимации (кости продолжают считаться и без отрисовки).
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TPOutfitGen.h"
#include "TPOutfitComponent.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS(ClassGroup = (TeniParizha), meta = (BlueprintSpawnableComponent))
class TENIPARIZHA_API UTPOutfitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTPOutfitComponent();

	/** Одеть скелетный меш Body нарядом Desc. false — у меша нет нужных костей (тогда манекен остаётся видимым). */
	bool ApplyOutfit(USkeletalMeshComponent* Body, const TPOutfit::FDesc& Desc);

	/** Снять наряд и снова показать манекен. */
	UFUNCTION(BlueprintCallable, Category = "Одежда")
	void ClearOutfit();

	/** Готовые наряды: герой, стражник, горожанин (bFemale — горожанка; Variant — вариант кроя и цветов). */
	UFUNCTION(BlueprintCallable, Category = "Одежда")
	bool ApplyHeroOutfit(USkeletalMeshComponent* Body);

	UFUNCTION(BlueprintCallable, Category = "Одежда")
	bool ApplyGuardOutfit(USkeletalMeshComponent* Body);

	UFUNCTION(BlueprintCallable, Category = "Одежда")
	bool ApplyCitizenOutfit(USkeletalMeshComponent* Body, bool bFemale, int32 Variant);

	/** Треугольников в надетом наряде (0 — не надет). */
	UFUNCTION(BlueprintPure, Category = "Одежда")
	int32 GetNumTriangles() const { return NumTriangles; }

	/** Скрывать манекен под одеждой. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Одежда")
	bool bHideBody = true;

	/** Части отбрасывают тень (для толпы вдали можно выключить). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Одежда")
	bool bCastShadow = true;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	TArray<UStaticMeshComponent*> Parts;

	UPROPERTY(Transient)
	USkeletalMeshComponent* Target = nullptr;

	int32 NumTriangles = 0;
};
