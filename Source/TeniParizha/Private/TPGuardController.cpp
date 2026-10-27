#include "TPGuardController.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

ATPGuardController::ATPGuardController()
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
}

void ATPGuardController::StopGuard()
{
	if (GetMoveStatus() != EPathFollowingStatus::Idle) StopMovement();
	LastGoal = FVector(1e9f);
}

bool ATPGuardController::IsBlocked(const FVector& From, const FVector& Dir) const
{
	// луч на высоте колен (центр капсулы − 40 см ≈ 50 см над землёй — выше ступеньки)
	const FVector Start = From - FVector(0.f, 0.f, 40.f);
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TPGuardAvoid), false, GetPawn());
	return GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + Dir * 90.f, ECC_Visibility, Q);
}

bool ATPGuardController::MoveGuardTo(const FVector& Goal, float Acceptance, float DeltaTime)
{
	APawn* P = GetPawn();
	if (!P) return false;
	const FVector Loc = P->GetActorLocation();
	FVector Flat = Goal - Loc;
	Flat.Z = 0.f;
	if (Flat.Size() <= Acceptance)
	{
		StopGuard();
		return true;
	}

	RepathTimer -= DeltaTime;
	if (bDirect)
	{
		DirectTimer -= DeltaTime;
		if (DirectTimer <= 0.f) { bDirect = false; RepathTimer = 0.f; } // снова попробовать навигацию
	}

	if (!bDirect)
	{
		const bool bIdle = GetMoveStatus() == EPathFollowingStatus::Idle;
		const float GoalShift2 = FVector::DistSquared2D(Goal, LastGoal);
		// новая цель — сразу; цель сдвинулась немного или путь кончился раньше цели — не чаще раза в 0.5 с
		if (GoalShift2 > FMath::Square(120.f) || (RepathTimer <= 0.f && (bIdle || GoalShift2 > FMath::Square(30.f))))
		{
			// путь раз за разом обрывается, не дойдя до цели (навмеш не покрывает её) — идти напрямую
			ShortPaths = (bIdle && GoalShift2 <= FMath::Square(120.f)) ? ShortPaths + 1 : 0;
			RepathTimer = 0.5f;
			LastGoal = Goal;
			EPathFollowingRequestResult::Type R = EPathFollowingRequestResult::Failed;
			if (ShortPaths < 3)
			{
				// Dest, AcceptanceRadius, bStopOnOverlap, bUsePathfinding, bProjectDestinationToNavigation, bCanStrafe
				R = MoveToLocation(Goal, FMath::Max(10.f, Acceptance * 0.8f), false, true, true, false);
			}
			if (R == EPathFollowingRequestResult::Failed)
			{
				ShortPaths = 0;
				bDirect = true;
				DirectTimer = 2.5f;
			}
			else if (R == EPathFollowingRequestResult::AlreadyAtGoal)
			{
				return true;
			}
		}
		// застрял на пути (навмеш устарел, толпа) — немного пройти напрямую
		if (!bDirect && GetMoveStatus() == EPathFollowingStatus::Moving)
		{
			StuckTime = P->GetVelocity().Size2D() < 15.f ? StuckTime + DeltaTime : 0.f;
			if (StuckTime > 1.5f) { StuckTime = 0.f; bDirect = true; DirectTimer = 1.5f; }
		}
	}

	if (bDirect)
	{
		if (GetMoveStatus() != EPathFollowingStatus::Idle) StopMovement();
		FVector Dir = Flat.GetSafeNormal();
		if (AvoidTime > 0.f)
		{
			AvoidTime -= DeltaTime;
			Dir = Dir.RotateAngleAxis(AvoidYaw, FVector::UpVector);
		}
		else if (IsBlocked(Loc, Dir))
		{
			// обход: ищем свободное направление по очереди справа и слева
			static const float Angles[] = { 50.f, -50.f, 90.f, -90.f, 135.f, -135.f };
			for (float A : Angles)
			{
				const FVector Alt = Dir.RotateAngleAxis(A, FVector::UpVector);
				if (!IsBlocked(Loc, Alt)) { AvoidYaw = A; AvoidTime = 0.6f; Dir = Alt; break; }
			}
		}
		P->AddMovementInput(Dir, 1.f);
	}
	return false;
}
