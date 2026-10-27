// Макет AIController.h (UE 4.27, модуль AIModule)
#pragma once
// TP_STUB_MODULE: AIModule
#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
class UNavigationQueryFilter; class UBlackboardComponent; class UBehaviorTree; class UBrainComponent; class UAIPerceptionComponent; class UBlackboardData;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIMoveCompletedSignature, FAIRequestID, RequestID, EPathFollowingResult::Type, Result);
class AAIController : public AController
{
public:
	typedef AController Super; typedef AAIController ThisClass; static UClass* StaticClass();
	AAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	uint32 bStartAILogicOnPossess : 1; uint32 bStopAILogicOnUnposses : 1; uint32 bLOSflag : 1; uint32 bSkipExtraLOSChecks : 1; uint32 bAllowStrafe : 1;
	uint32 bWantsPlayerState : 1; uint32 bSetControlRotationFromPawnOrientation : 1;
	UBrainComponent* BrainComponent;
	FAIMoveCompletedSignature ReceiveMoveCompleted;
	EPathFollowingRequestResult::Type MoveToActor(AActor* Goal, float AcceptanceRadius = -1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bCanStrafe = true, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr, bool bAllowPartialPath = true);
	EPathFollowingRequestResult::Type MoveToLocation(const FVector& Dest, float AcceptanceRadius = -1, bool bStopOnOverlap = true, bool bUsePathfinding = true, bool bProjectDestinationToNavigation = false, bool bCanStrafe = true, TSubclassOf<UNavigationQueryFilter> FilterClass = nullptr, bool bAllowPartialPath = true);
	virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest, TSharedPtr<struct FNavigationPath>* OutPath = nullptr);
	virtual void StopMovement() override;
	EPathFollowingStatus::Type GetMoveStatus() const;
	bool HasPartialPath() const;
	FVector GetImmediateMoveDestination() const;
	UPathFollowingComponent* GetPathFollowingComponent() const;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result);
	virtual void SetFocalPoint(FVector NewFocus, EAIFocusPriority::Type InPriority = EAIFocusPriority::Gameplay);
	virtual void SetFocus(AActor* NewFocus, EAIFocusPriority::Type InPriority = EAIFocusPriority::Gameplay);
	virtual void ClearFocus(EAIFocusPriority::Type InPriority);
	FVector GetFocalPoint() const; FVector GetFocalPointForPriority(EAIFocusPriority::Type InPriority) const; AActor* GetFocusActor() const;
	virtual FVector GetFocalPointOnActor(const AActor* Actor) const;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true);
	virtual bool LineOfSightTo(const AActor* Other, FVector ViewPoint = FVector(ForceInit), bool bAlternateChecks = false) const override;
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset);
	bool UseBlackboard(UBlackboardData* BlackboardAsset, UBlackboardComponent*& BlackboardComponent);
	UBlackboardComponent* GetBlackboardComponent();
	UAIPerceptionComponent* GetAIPerceptionComponent();
	virtual void Tick(float DeltaTime) override;
	virtual void PostInitializeComponents() override;
	virtual void GetPlayerViewPoint(FVector& out_Location, FRotator& out_rotation) const override;
	void SetMoveBlockDetection(bool bEnable);
protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
};
