// Макет WorldCollision.h (UE 4.27, модуль Engine): FOverlapResult здесь (в UE5 — Engine/OverlapResult.h).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
struct FOverlapResult
{
	TWeakObjectPtr<AActor> Actor; TWeakObjectPtr<UPrimitiveComponent> Component; int32 ItemIndex; uint32 bBlockingHit : 1;
	FOverlapResult();
	AActor* GetActor() const; UPrimitiveComponent* GetComponent() const;
};
struct FOverlapDatum; struct FTraceDatum; struct FTraceHandle { bool IsValid() const; };
enum class EAsyncTraceType : uint8 { Test, Single, Multi };
DECLARE_DELEGATE_TwoParams(FTraceDelegate, const FTraceHandle&, FTraceDatum&);
DECLARE_DELEGATE_TwoParams(FOverlapDelegate, const FTraceHandle&, FOverlapDatum&);
