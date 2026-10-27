#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
struct FScalarParameterValue { FMaterialParameterInfo ParameterInfo; float ParameterValue; };
struct FVectorParameterValue { FMaterialParameterInfo ParameterInfo; FLinearColor ParameterValue; };
struct FTextureParameterValue { FMaterialParameterInfo ParameterInfo; UTexture* ParameterValue; };
class UMaterialInstance : public UMaterialInterface
{
public:
	typedef UMaterialInterface Super; typedef UMaterialInstance ThisClass; static UClass* StaticClass();
	UMaterialInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UMaterialInterface* Parent;
	TArray<FScalarParameterValue> ScalarParameterValues; TArray<FVectorParameterValue> VectorParameterValues; TArray<FTextureParameterValue> TextureParameterValues;
	uint8 bHasStaticPermutationResource : 1; uint8 bOverrideSubsurfaceProfile : 1;
};
