// Макет Materials/MaterialInstanceDynamic.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Materials/MaterialInstance.h"
class UMaterialInstanceDynamic : public UMaterialInstance
{
public:
	typedef UMaterialInstance Super; typedef UMaterialInstanceDynamic ThisClass; static UClass* StaticClass();
	UMaterialInstanceDynamic(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static UMaterialInstanceDynamic* Create(class UMaterialInterface* ParentMaterial, class UObject* InOuter);
	static UMaterialInstanceDynamic* Create(class UMaterialInterface* ParentMaterial, class UObject* InOuter, FName Name);
	void SetScalarParameterValue(FName ParameterName, float Value);
	void SetScalarParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, float Value);
	bool InitializeScalarParameterAndGetIndex(const FName& ParameterName, float Value, int32& OutParameterIndex);
	bool SetScalarParameterByIndex(int32 ParameterIndex, float Value);
	void SetVectorParameterValue(FName ParameterName, const FLinearColor& Value);
	void SetVectorParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, const FLinearColor& Value);
	void SetTextureParameterValue(FName ParameterName, class UTexture* Value);
	void SetTextureParameterValueByInfo(const FMaterialParameterInfo& ParameterInfo, class UTexture* Value);
	float K2_GetScalarParameterValue(FName ParameterName);
	FLinearColor K2_GetVectorParameterValue(FName ParameterName);
	class UTexture* K2_GetTextureParameterValue(FName ParameterName);
	void K2_InterpolateMaterialInstanceParams(UMaterialInstance* SourceA, UMaterialInstance* SourceB, float Alpha);
	void K2_CopyMaterialInstanceParameters(UMaterialInterface* Source, bool bQuickParametersOnly = false);
	void CopyParameterOverrides(UMaterialInstance* MaterialInstance);
	void CopyMaterialUniformParameters(UMaterialInterface* Source);
	void ClearParameterValues();
};
