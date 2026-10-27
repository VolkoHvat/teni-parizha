#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
class UMaterial : public UMaterialInterface
{
public:
	typedef UMaterialInterface Super; typedef UMaterial ThisClass; static UClass* StaticClass();
	UMaterial(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TEnumAsByte<enum EMaterialDomain> MaterialDomain; TEnumAsByte<enum EBlendMode> BlendMode;
	uint32 TwoSided : 1; uint32 bUsedWithSkeletalMesh : 1; uint32 bUsedWithInstancedStaticMeshes : 1; uint32 bUsedWithParticleSprites : 1; uint32 bUsedWithStaticLighting : 1;
	uint32 bUsedWithMorphTargets : 1; uint32 bUsedWithSplineMeshes : 1; uint32 bFullyRough : 1; uint32 bUseFullPrecision : 1; uint32 bTangentSpaceNormal : 1; uint32 DitheredLODTransition : 1;
	float OpacityMaskClipValue;
	static UMaterial* GetDefaultMaterial(EMaterialDomain Domain);
	void SetShadingModel(EMaterialShadingModel NewModel);
	virtual UMaterial* GetMaterial() override;
	virtual EBlendMode GetBlendMode() const override;
};
