#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/NavMovementComponent.h"
class APawn;
class UPawnMovementComponent : public UNavMovementComponent
{
public:
	typedef UNavMovementComponent Super; typedef UPawnMovementComponent ThisClass; static UClass* StaticClass();
	UPawnMovementComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void AddInputVector(FVector WorldVector, bool bForce = false);
	FVector GetPendingInputVector() const; FVector GetLastInputVector() const; virtual FVector ConsumeInputVector();
	virtual bool IsMoveInputIgnored() const;
	APawn* GetPawnOwner() const;
	virtual void NotifyBumpedPawn(APawn* BumpedPawn);
	virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
protected:
	APawn* PawnOwner;
};
