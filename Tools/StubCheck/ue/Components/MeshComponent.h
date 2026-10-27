#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
class UMeshComponent : public UPrimitiveComponent
{
public:
	typedef UPrimitiveComponent Super; typedef UMeshComponent ThisClass; static UClass* StaticClass();
	UMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<UMaterialInterface*> OverrideMaterials;
	virtual TArray<UMaterialInterface*> GetMaterials() const;
	virtual int32 GetNumMaterials() const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;
	virtual void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) override;
	virtual int32 GetMaterialIndex(FName MaterialSlotName) const override;
	virtual TArray<FName> GetMaterialSlotNames() const override;
	virtual bool IsMaterialSlotNameValid(FName MaterialSlotName) const override;
	void EmptyOverrideMaterials();
	bool HasOverrideMaterials();
	void SetScalarParameterValueOnMaterials(const FName ParameterName, const float ParameterValue);
	void SetVectorParameterValueOnMaterials(const FName ParameterName, const FVector ParameterValue);
	void PrestreamTextures(float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups = 0);
};
