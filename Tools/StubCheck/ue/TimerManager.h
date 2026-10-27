// Макет TimerManager.h (UE 4.27): у SetTimer с делегатом/лямбдой параметр InbLoop обязателен.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
class FTimerManager
{
public:
	template<class UserClass> void SetTimer(FTimerHandle& InOutHandle, UserClass* InObj, typename TTPIdentity<void (UserClass::*)()>::Type InTimerMethod, float InRate, bool InbLoop = false, float InFirstDelay = -1.f);
	template<class UserClass> void SetTimer(FTimerHandle& InOutHandle, UserClass* InObj, typename TTPIdentity<void (UserClass::*)() const>::Type InTimerMethod, float InRate, bool InbLoop = false, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, FTimerDelegate const& InDelegate, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, FTimerDynamicDelegate const& InDynDelegate, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	void SetTimer(FTimerHandle& InOutHandle, TFunction<void(void)>&& Callback, float InRate, bool InbLoop, float InFirstDelay = -1.f);
	template<class UserClass> FTimerHandle SetTimerForNextTick(UserClass* inObj, typename TTPIdentity<void (UserClass::*)()>::Type inTimerMethod);
	FTimerHandle SetTimerForNextTick(FTimerDelegate const& InDelegate);
	FTimerHandle SetTimerForNextTick(TFunction<void(void)>&& Callback);
	void ClearTimer(FTimerHandle& InHandle);
	void ClearAllTimersForObject(void const* Object);
	void PauseTimer(FTimerHandle InHandle); void UnPauseTimer(FTimerHandle InHandle);
	float GetTimerRate(FTimerHandle InHandle) const; bool IsTimerActive(FTimerHandle InHandle) const; bool IsTimerPaused(FTimerHandle InHandle) const;
	bool IsTimerPending(FTimerHandle InHandle) const; bool TimerExists(FTimerHandle InHandle) const;
	float GetTimerElapsed(FTimerHandle InHandle) const; float GetTimerRemaining(FTimerHandle InHandle) const;
	bool HasBeenTickedThisFrame() const;
};
