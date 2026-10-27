// Макет Subsystems/WorldSubsystem.h (UE 4.27). UTickableWorldSubsystem отсутствует (появился в UE5): для тика — FTickableGameObject из Tickable.h.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Subsystems/Subsystem.h"
class UWorld;
class UWorldSubsystem : public USubsystem
{
public:
	typedef USubsystem Super; typedef UWorldSubsystem ThisClass; static UClass* StaticClass();
	UWorldSubsystem();
	virtual UWorld* GetWorld() const override final;
	UWorld& GetWorldRef() const;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void PostInitialize();
	virtual void OnWorldBeginPlay(UWorld& InWorld);
};
