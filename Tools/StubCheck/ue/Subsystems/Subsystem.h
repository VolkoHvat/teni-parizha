#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class FSubsystemCollectionBase;
class USubsystem : public UObject
{
public:
	typedef UObject Super; typedef USubsystem ThisClass; static UClass* StaticClass();
	USubsystem();
	virtual bool ShouldCreateSubsystem(UObject* Outer) const;
	virtual void Initialize(FSubsystemCollectionBase& Collection);
	virtual void Deinitialize();
};
class FSubsystemCollectionBase
{
public:
	USubsystem* InitializeDependency(TSubclassOf<USubsystem> SubsystemClass);
	template<class TSubsystemClass> TSubsystemClass* InitializeDependency() { static_assert(std::is_base_of<USubsystem, TSubsystemClass>::value, "InitializeDependency<T>: нужна подсистема (и #include)"); return nullptr; }
};
