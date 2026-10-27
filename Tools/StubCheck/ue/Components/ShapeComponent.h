#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
class UShapeComponent : public UPrimitiveComponent
{
public:
	typedef UPrimitiveComponent Super; typedef UShapeComponent ThisClass; static UClass* StaticClass();
	UShapeComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UBodySetup* ShapeBodySetup;
	FColor ShapeColor;
	uint8 bDrawOnlyIfSelected : 1; uint8 bUseArchetypeBodySetup : 1; uint8 bDynamicObstacle : 1;
	TSubclassOf<class UNavAreaBase> AreaClass;
};
