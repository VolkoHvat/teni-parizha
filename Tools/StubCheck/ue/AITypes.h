// Макет AITypes.h (UE 4.27, модуль AIModule)
#pragma once
// TP_STUB_MODULE: AIModule
#include "CoreMinimal.h"
#include "UObject/Object.h"
class AActor; class UNavigationQueryFilter;
namespace EAIFocusPriority { typedef uint8 Type; const Type Default = 0; const Type Move = 1; const Type Gameplay = 2; const Type LastFocusPriority = Gameplay; }
namespace EAIOptionFlag { enum Type { Default, Enable, Disable, MAX }; }
struct FAIRequestID
{
	FAIRequestID(uint32 InRequestID = InvalidRequestID);
	bool IsEquivalent(uint32 OtherID) const; bool IsEquivalent(FAIRequestID Other) const; bool IsValid() const; uint32 GetID() const;
	operator uint32() const; bool operator==(const FAIRequestID& Other) const;
	static const uint32 AnyRequestID = 0; static const uint32 InvalidRequestID = 0xffffffff;
	static const FAIRequestID AnyRequest; static const FAIRequestID CurrentRequest; static const FAIRequestID InvalidRequest;
};
struct FAIMoveRequest
{
	FAIMoveRequest(); FAIMoveRequest(const AActor* InGoalActor); FAIMoveRequest(const FVector& InGoalLocation);
	FAIMoveRequest& SetGoalActor(const AActor* InGoalActor); FAIMoveRequest& SetGoalLocation(const FVector& InGoalLocation);
	FAIMoveRequest& SetUsePathfinding(bool bPathfinding); FAIMoveRequest& SetAllowPartialPath(bool bAllowPartial); FAIMoveRequest& SetProjectGoalLocation(bool bProject);
	FAIMoveRequest& SetNavigationFilter(TSubclassOf<UNavigationQueryFilter> Filter); FAIMoveRequest& SetAcceptanceRadius(float Radius); FAIMoveRequest& SetReachTestIncludesAgentRadius(bool bIncludeRadius);
	FAIMoveRequest& SetReachTestIncludesGoalRadius(bool bIncludeRadius); FAIMoveRequest& SetCanStrafe(bool bStrafe);
	bool IsValid() const; bool IsMoveToActorRequest() const; AActor* GetGoalActor() const; FVector GetGoalLocation() const; float GetAcceptanceRadius() const;
};
