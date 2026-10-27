#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Subsystems/Subsystem.h"
class ULocalPlayer;
class ULocalPlayerSubsystem : public USubsystem
{
public:
	typedef USubsystem Super; typedef ULocalPlayerSubsystem ThisClass; static UClass* StaticClass();
	ULocalPlayerSubsystem();
	template<typename LocalPlayerType = ULocalPlayer> LocalPlayerType* GetLocalPlayer() const { return nullptr; }
};
