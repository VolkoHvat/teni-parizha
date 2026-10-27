// Макет Navigation/PathFollowingComponent.h (UE 4.27, модуль AIModule)
#pragma once
// TP_STUB_MODULE: AIModule
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
namespace EPathFollowingStatus { enum Type { Idle, Waiting, Paused, Moving }; }
namespace EPathFollowingResult { enum Type { Success, Blocked, OffPath, Aborted, Skipped_DEPRECATED, Invalid }; }
namespace EPathFollowingRequestResult { enum Type { Failed, AlreadyAtGoal, RequestSuccessful }; }
namespace FPathFollowingResultFlags { typedef uint16 Type; const Type None = 0; const Type Success = 1; const Type Blocked = 2; const Type OffPath = 4; const Type UserAbort = 8; const Type OwnerFinished = 16; const Type InvalidPath = 32; const Type MovementStop = 64; const Type NewRequest = 128; const Type ForcedScript = 256; const Type AlreadyAtGoal = 512; }
struct FPathFollowingResult
{
	TEnumAsByte<EPathFollowingResult::Type> Code; FPathFollowingResultFlags::Type Flags;
	FPathFollowingResult(); FPathFollowingResult(FPathFollowingResultFlags::Type InFlags); FPathFollowingResult(EPathFollowingResult::Type ResultCode, FPathFollowingResultFlags::Type ExtraFlags);
	bool HasFlag(FPathFollowingResultFlags::Type Flag) const; bool IsSuccess() const; bool IsFailure() const; bool IsInterrupted() const; FString ToString() const;
};
struct FPathFollowingRequestResult { FAIRequestID MoveId; TEnumAsByte<EPathFollowingRequestResult::Type> Code; FPathFollowingRequestResult(); operator EPathFollowingRequestResult::Type() const; };
enum class EPathFollowingVelocityMode : uint8 { Reset, Keep };
enum class EPathFollowingReachMode : uint8 { ExactLocation, OverlapAgent, OverlapGoal, OverlapAgentAndGoal };
class UPathFollowingComponent : public UActorComponent
{
public:
	typedef UActorComponent Super; typedef UPathFollowingComponent ThisClass; static UClass* StaticClass();
	UPathFollowingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	DECLARE_MULTICAST_DELEGATE_TwoParams(FMoveComplete, FAIRequestID, const FPathFollowingResult&);
	FMoveComplete OnRequestFinished;
	EPathFollowingStatus::Type GetStatus() const;
	virtual void AbortMove(const UObject& Instigator, FPathFollowingResultFlags::Type AbortFlags, FAIRequestID RequestID = FAIRequestID::CurrentRequest, EPathFollowingVelocityMode VelocityMode = EPathFollowingVelocityMode::Reset);
	virtual FAIRequestID PauseMove(FAIRequestID RequestID = FAIRequestID::CurrentRequest, EPathFollowingVelocityMode VelocityMode = EPathFollowingVelocityMode::Reset);
	virtual void ResumeMove(FAIRequestID RequestID = FAIRequestID::CurrentRequest);
	FAIRequestID GetCurrentRequestId() const; FVector GetCurrentDirection() const; FVector GetCurrentTargetLocation() const; FVector GetCurrentAcceleration() const;
	bool HasReached(const FVector& TestPoint, EPathFollowingReachMode ReachMode, float InAcceptanceRadius = -1.f) const;
	bool HasPartialPath() const; bool DidMoveReachGoal() const;
	float GetRemainingPathCost() const; const AActor* GetMoveGoal() const;
	void SetAcceptanceRadius(float InAcceptanceRadius); float GetAcceptanceRadius() const;
	void SetBlockDetection(float DistanceThreshold, float Interval, int32 NumSamples); void SetBlockDetectionState(bool bEnable);
};
