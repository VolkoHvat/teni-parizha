#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
class UWorld;
class UGameViewportClient : public UObject
{
public:
	typedef UObject Super; typedef UGameViewportClient ThisClass; static UClass* StaticClass();
	UGameViewportClient(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	void GetViewportSize(FVector2D& ViewportSize) const;
	virtual UWorld* GetWorld() const override;
	bool IsFullScreenViewport() const; void SetSuppressTransitionMessage(bool bSuppress);
	bool GetMousePosition(FVector2D& MousePosition) const;
	void SetMouseCaptureMode(EMouseCaptureMode Mode);
};
