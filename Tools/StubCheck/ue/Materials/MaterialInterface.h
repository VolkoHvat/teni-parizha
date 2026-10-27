#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
class UMaterial; class UTexture; class UPhysicalMaterial;
enum EMaterialDomain { MD_Surface, MD_DeferredDecal, MD_LightFunction, MD_Volume, MD_PostProcess, MD_UI, MD_RuntimeVirtualTexture, MD_MAX };
enum EMaterialParameterAssociation { LayerParameter, BlendParameter, GlobalParameter };
struct FMaterialParameterInfo { FName Name; TEnumAsByte<EMaterialParameterAssociation> Association; int32 Index; FMaterialParameterInfo(); FMaterialParameterInfo(FName InName, EMaterialParameterAssociation InAssociation = GlobalParameter, int32 InIndex = INDEX_NONE); };
struct FHashedMaterialParameterInfo { FHashedMaterialParameterInfo(); FHashedMaterialParameterInfo(const FMaterialParameterInfo& Rhs); FHashedMaterialParameterInfo(FName InName, EMaterialParameterAssociation InAssociation = GlobalParameter, int32 InIndex = INDEX_NONE); };
class UMaterialInterface : public UObject
{
public:
	typedef UObject Super; typedef UMaterialInterface ThisClass; static UClass* StaticClass();
	UMaterialInterface(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual UMaterial* GetMaterial();
	virtual const UMaterial* GetMaterial() const;
	UMaterial* GetBaseMaterial();
	virtual EBlendMode GetBlendMode() const;
	virtual bool IsTwoSided() const;
	virtual bool IsMasked() const;
	virtual float GetOpacityMaskClipValue() const;
	virtual UPhysicalMaterial* GetPhysicalMaterial() const;
	virtual bool GetScalarParameterValue(const FHashedMaterialParameterInfo& ParameterInfo, float& OutValue, bool bOveriddenOnly = false) const;
	virtual bool GetVectorParameterValue(const FHashedMaterialParameterInfo& ParameterInfo, FLinearColor& OutValue, bool bOveriddenOnly = false) const;
	virtual bool GetTextureParameterValue(const FHashedMaterialParameterInfo& ParameterInfo, UTexture*& OutValue, bool bOveriddenOnly = false) const;
	static void InitDefaultMaterials();
};
