// Тени Парижа — базовый класс анимации героя (переменные паркура для AnimBP).
#include "TPTraversalAnimInstance.h"
#include "TPTraversalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UTPTraversalAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ResolveOwner();
}

void UTPTraversalAnimInstance::ResolveOwner()
{
	OwnerCharacter = Cast<ACharacter>(TryGetPawnOwner());
	Traversal = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UTPTraversalComponent>() : nullptr;
}

void UTPTraversalAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	if (!OwnerCharacter || !Traversal) ResolveOwner();
	if (!OwnerCharacter) return;

	const FVector V = OwnerCharacter->GetVelocity();
	Speed = static_cast<float>(V.Size2D());
	VerticalSpeed = static_cast<float>(V.Z);
	const UCharacterMovementComponent* Move = OwnerCharacter->GetCharacterMovement();
	bInAir = Move && Move->IsFalling();

	float TargetIK = 0.f;
	if (Traversal)
	{
		State = Traversal->GetState();
		bHanging = State == ETPTraversal::Hang || State == ETPTraversal::Hop || State == ETPTraversal::Drop;
		bClimbing = State == ETPTraversal::Climb || State == ETPTraversal::WallRun;
		bInAir = bInAir || State == ETPTraversal::Jump || State == ETPTraversal::Air;
		bCrouching = Traversal->IsCrouching();
		bFreeRunning = Traversal->IsFreeRunning();
		ClimbSpeed = Traversal->GetClimbAnimRate();
		ShimmyDir = Traversal->GetShimmyDir();
		LeanAngle = Traversal->GetLeanAngle();
		FVector L, R;
		TargetIK = Traversal->GetHandIK(L, R);
		if (TargetIK > 0.f)
		{
			LeftHandIK = L;
			RightHandIK = R;
		}
	}
	else
	{
		State = bInAir ? ETPTraversal::Air : ETPTraversal::Ground;
		bHanging = bClimbing = bCrouching = bFreeRunning = false;
		ClimbSpeed = ShimmyDir = LeanAngle = 0.f;
	}
	// плавное включение/выключение IK рук
	HandIKAlpha = FMath::FInterpTo(HandIKAlpha, TargetIK, DeltaSeconds, 12.f);
}
