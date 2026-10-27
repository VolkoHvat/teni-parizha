// Макет EngineUtils.h (UE 4.27): TActorIterator, TActorRange.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/World.h"
enum class EActorIteratorFlags { AllActors = 0, SkipPendingKill = 1, OnlySelectedActors = 2, OnlyActiveLevels = 4 };
ENUM_CLASS_FLAGS(EActorIteratorFlags)
template<typename ActorType>
class TActorIterator
{
public:
	explicit TActorIterator(const UWorld* InWorld, TSubclassOf<ActorType> InClass = ActorType::StaticClass(), EActorIteratorFlags InFlags = EActorIteratorFlags::OnlyActiveLevels | EActorIteratorFlags::SkipPendingKill)
	{ static_assert(std::is_base_of<AActor, ActorType>::value, "TActorIterator<T>: T должен быть актором (нужен #include)"); }
	ActorType* operator*() const; ActorType* operator->() const;
	TActorIterator& operator++(); explicit operator bool() const; bool operator!() const;
	UWorld* GetWorld() const;
};
class FActorIterator : public TActorIterator<AActor> { public: explicit FActorIterator(const UWorld* InWorld, const EActorIteratorFlags InFlags = EActorIteratorFlags::OnlyActiveLevels | EActorIteratorFlags::SkipPendingKill); };
template<typename ActorType>
class TActorRange
{
public:
	explicit TActorRange(const UWorld* InWorld, TSubclassOf<ActorType> InClass = ActorType::StaticClass(), EActorIteratorFlags InFlags = EActorIteratorFlags::OnlyActiveLevels | EActorIteratorFlags::SkipPendingKill);
	struct It { ActorType* operator*() const; It& operator++(); bool operator!=(const It&) const; };
	It begin() const; It end() const;
};
