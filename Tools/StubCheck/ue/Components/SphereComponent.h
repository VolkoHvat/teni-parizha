#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
class USphereComponent : public UShapeComponent
{
public:
	typedef UShapeComponent Super; typedef USphereComponent ThisClass; static UClass* StaticClass();
	USphereComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void InitSphereRadius(float InSphereRadius);
	void SetSphereRadius(float InSphereRadius, bool bUpdateOverlaps = true);
	float GetScaledSphereRadius() const; float GetUnscaledSphereRadius() const; float GetShapeScale() const;
protected:
	float SphereRadius;
};
