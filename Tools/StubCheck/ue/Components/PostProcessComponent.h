#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Engine/Scene.h"
class UPostProcessComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef UPostProcessComponent ThisClass; static UClass* StaticClass();
	UPostProcessComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FPostProcessSettings Settings; float Priority; float BlendRadius; float BlendWeight; uint32 bEnabled : 1; uint32 bUnbound : 1;
	void AddOrUpdateBlendable(TScriptInterface<class IBlendableInterface> InBlendableObject, float InWeight = 1.0f);
};
