// Макет Tickable.h (UE 4.27): FTickableGameObject — тик для не-акторов.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
class UWorld;
enum class ETickableTickType : uint8 { Conditional, Always, Never };
class FTickableObjectBase
{
public:
	virtual ~FTickableObjectBase();
	virtual void Tick(float DeltaTime) = 0;
	virtual ETickableTickType GetTickableTickType() const;
	virtual bool IsTickable() const;
	virtual TStatId GetStatId() const = 0;
};
class FTickableGameObject : public FTickableObjectBase
{
public:
	FTickableGameObject(); virtual ~FTickableGameObject();
	virtual bool IsTickableWhenPaused() const;
	virtual bool IsTickableInEditor() const;
	virtual UWorld* GetTickableGameObjectWorld() const;
	static void TickObjects(UWorld* World, int32 InTickType, bool bIsPaused, float DeltaSeconds);
};
