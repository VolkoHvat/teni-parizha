#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
class UCapsuleComponent : public UShapeComponent
{
public:
	typedef UShapeComponent Super; typedef UCapsuleComponent ThisClass; static UClass* StaticClass();
	UCapsuleComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void InitCapsuleSize(float InRadius, float InHalfHeight);
	void SetCapsuleSize(float InRadius, float InHalfHeight, bool bUpdateOverlaps = true);
	void SetCapsuleRadius(float Radius, bool bUpdateOverlaps = true);
	void SetCapsuleHalfHeight(float HalfHeight, bool bUpdateOverlaps = true);
	float GetScaledCapsuleRadius() const; float GetScaledCapsuleHalfHeight() const; float GetScaledCapsuleHalfHeight_WithoutHemisphere() const;
	float GetUnscaledCapsuleRadius() const; float GetUnscaledCapsuleHalfHeight() const; float GetUnscaledCapsuleHalfHeight_WithoutHemisphere() const;
	void GetScaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const;
	void GetScaledCapsuleSize_WithoutHemisphere(float& OutRadius, float& OutHalfHeightWithoutHemisphere) const;
	void GetUnscaledCapsuleSize(float& OutRadius, float& OutHalfHeight) const;
	float GetShapeScale() const;
protected:
	float CapsuleHalfHeight; float CapsuleRadius;
};
