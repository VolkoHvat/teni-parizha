#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"
class UBoxComponent : public UShapeComponent
{
public:
	typedef UShapeComponent Super; typedef UBoxComponent ThisClass; static UClass* StaticClass();
	UBoxComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void InitBoxExtent(const FVector& InBoxExtent);
	void SetBoxExtent(FVector InBoxExtent, bool bUpdateOverlaps = true);
	FVector GetScaledBoxExtent() const; FVector GetUnscaledBoxExtent() const;
	float LineThickness;
protected:
	FVector BoxExtent;
};
