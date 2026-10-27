#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Subsystems/Subsystem.h"
class UGameInstance;
class UGameInstanceSubsystem : public USubsystem
{
public:
	typedef USubsystem Super; typedef UGameInstanceSubsystem ThisClass; static UClass* StaticClass();
	UGameInstanceSubsystem();
	UGameInstance* GetGameInstance() const;
};
