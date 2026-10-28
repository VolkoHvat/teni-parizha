// Бой в духе AC3/Unity, перенесён из legacy/teni_v1/game2/game.c (combat_update, guard_hit, combat_target, combat_threat).
#include "TPCombatComponent.h"
#include "TeniParizha.h"
#include "TPGuard.h"
#include "TPCombatDirector.h"
#include "TPTraversalComponent.h"
#include "TPCityGenerator.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace TPCombatImpl
{
	/** герой занят паркуром (лазает, висит, перемахивает) — бить нельзя */
	bool IsBusyTraversing(const AActor* Owner)
	{
		const UTPTraversalComponent* T = Owner ? Owner->FindComponentByClass<UTPTraversalComponent>() : nullptr;
		if (!T) return false;
		switch (T->GetState())
		{
		case ETPTraversal::Climb:
		case ETPTraversal::Hang:
		case ETPTraversal::ClimbUp:
		case ETPTraversal::Drop:
		case ETPTraversal::Vault:
		case ETPTraversal::WallRun:
		case ETPTraversal::Roll:
			return true;
		default:
			return false;
		}
	}
}

UTPCombatComponent::UTPCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTPCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Health <= 0.f || Health > MaxHealth) Health = MaxHealth;

	if (!GetGuardOwner())
	{
		// начальная безопасная точка — площадь квартала (как у ATPCharacter::BeginPlay)
		const AActor* O = GetOwner();
		SafePoint = O->GetActorLocation();
		SafeYaw = O->GetActorRotation().Yaw;
		if (UWorld* W = GetWorld())
			for (TActorIterator<ATPCityGenerator> It(W); It; ++It) { SafePoint = It->GetPlayerStart() + FVector(0.f, 0.f, 100.f); break; }
		bHasSafePoint = true;
	}
}

AActor* UTPCombatComponent::GetAttackTarget() const
{
	return AttackTarget.Get();
}

ATPGuard* UTPCombatComponent::GetGuardOwner() const
{
	return Cast<ATPGuard>(GetOwner());
}

UTPCombatDirector* UTPCombatComponent::GetDirector() const
{
	UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UTPCombatDirector>() : nullptr;
}

bool UTPCombatComponent::CanActOnGround() const
{
	const ACharacter* C = Cast<ACharacter>(GetOwner());
	if (!C) return true;
	if (const UTPTraversalComponent* T = C->FindComponentByClass<UTPTraversalComponent>())
	{
		const ETPTraversal S = T->GetState();
		return S == ETPTraversal::Ground || S == ETPTraversal::Land || S == ETPTraversal::Turn;
	}
	const UCharacterMovementComponent* M = C->GetCharacterMovement();
	return !M || M->IsMovingOnGround();
}

void UTPCombatComponent::PlayMontage(UAnimMontage* Montage, float Rate)
{
	if (!Montage) return;
	if (ACharacter* C = Cast<ACharacter>(GetOwner())) C->PlayAnimMontage(Montage, Rate);
}

void UTPCombatComponent::FaceTowards(const FVector& Point, float DeltaTime, float Speed)
{
	AActor* O = GetOwner();
	if (!O) return;
	FVector D = Point - O->GetActorLocation();
	D.Z = 0.f;
	if (D.IsNearlyZero()) return;
	const FRotator Want(0.f, D.Rotation().Yaw, 0.f);
	O->SetActorRotation(FMath::RInterpTo(O->GetActorRotation(), Want, DeltaTime, Speed));
}

void UTPCombatComponent::SetSafePoint(FVector Location, float Yaw)
{
	SafePoint = Location;
	SafeYaw = Yaw;
	bHasSafePoint = true;
}

/* ---------------- выбор цели ---------------- */

AActor* UTPCombatComponent::FindTarget(const FVector& Direction, float Range, bool bUnawareOnly) const
{
	const UTPCombatDirector* Dir = GetDirector();
	const AActor* O = GetOwner();
	if (!Dir || !O) return nullptr;
	const FVector Me = O->GetActorLocation();
	FVector Fwd = Direction.GetSafeNormal2D();
	if (Fwd.IsNearlyZero()) Fwd = O->GetActorForwardVector().GetSafeNormal2D();

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (ATPGuard* G : Dir->GetGuards())
	{
		if (!G || G == O || G->IsKO() || !G->Combat || G->Combat->IsDefeated()) continue;
		if (bUnawareOnly && G->State == ETPGuardState::Chase) continue;
		FVector To = G->GetActorLocation() - Me;
		const float Dz = To.Z;
		To.Z = 0.f;
		const float D = To.Size();
		if (D > Range || FMath::Abs(Dz) > 160.f) continue;
		const float F = D > 1.f ? FVector::DotProduct(Fwd, To / D) : 1.f;
		if (F < -0.5f && D > 150.f) continue; // далеко за спиной — не цель
		// ближний и впереди лучше; преследующие и открытые для добивания — в приоритете
		const float Score = D - F * 120.f - (G->State == ETPGuardState::Chase ? 50.f : 0.f) - (G->Combat->OpenTime > 0.f ? 100.f : 0.f);
		if (Score < BestScore) { BestScore = Score; Best = G; }
	}
	return Best;
}

/* ---------------- действия ---------------- */

void UTPCombatComponent::RequestAction(ETPCombatAction Action, FVector Direction)
{
	if (IsDefeated() || bRespawning) return;

	// стражник: «атака» — замах (WindUp), удар по его окончании; жетон — у режиссёра
	if (ATPGuard* G = GetGuardOwner())
	{
		if (Action != ETPCombatAction::Attack) return;
		if (G->IsKO() || WindUp > 0.f || Swing > 0.f || StaggerTime > 0.f || OpenTime > 0.f || BlockTime > 0.f) return;
		if (UTPCombatDirector* Dir = GetDirector())
		{
			if (!Dir->TryTakeAttackToken(G)) return;
			Dir->Panic(G->GetActorLocation(), 1200.f);
		}
		WindUp = WindUpTime;
		AttackTarget = UGameplayStatics::GetPlayerPawn(this, 0);
		return;
	}

	switch (Action)
	{
	case ETPCombatAction::Attack:
	case ETPCombatAction::Takedown:
	{
		if (ReactTime > 0.f || TPCombatImpl::IsBusyTraversing(GetOwner())) return;
		if (AttackTime > 0.f)
		{
			// буфер ввода: следующий удар серии во второй половине текущего
			if (AttackTime < AttackDuration * 0.5f) bQueuedAttack = true;
			return;
		}
		AActor* Target = nullptr;
		if (Action == ETPCombatAction::Takedown) Target = FindTarget(Direction, 250.f, true);
		if (!Target) Target = FindTarget(Direction, CombatWeight > 0.5f ? TargetRange : TargetRangeCalm, false);
		StartAttack(Target);
		break;
	}
	case ETPCombatAction::Counter:
		TryCounter();
		break;
	case ETPCombatAction::Dodge:
		TryDodge(Direction);
		break;
	default:
		break;
	}
}

void UTPCombatComponent::StartAttack(AActor* Target)
{
	Combo = ChainTime > 0.f ? (Combo + 1) % 3 : 0;
	AttackDuration = Combo == 2 ? 0.55f : 0.38f;
	AttackTime = AttackDuration;
	bAttackHit = false;
	AttackTarget = Target;
	ChainTime = AttackDuration + 0.5f;
	if (AttackMontages.Num() > 0) PlayMontage(AttackMontages[Combo % AttackMontages.Num()]);
}

void UTPCombatComponent::TickAttack(float DeltaTime)
{
	if (AttackTime <= 0.f) return;
	AActor* O = GetOwner();
	AttackTime -= DeltaTime;
	const float Prog = 1.f - FMath::Max(AttackTime, 0.f) / FMath::Max(AttackDuration, 0.01f);

	// выпад к цели в первой половине удара
	AActor* T = AttackTarget.Get();
	const ATPGuard* TG = Cast<ATPGuard>(T);
	if (T && O && !(TG && TG->IsKO()))
	{
		FVector To = T->GetActorLocation() - O->GetActorLocation();
		To.Z = 0.f;
		const float D = To.Size();
		if (D > 1.f)
		{
			FaceTowards(T->GetActorLocation(), DeltaTime, 18.f);
			if (D > 135.f && Prog < 0.5f)
			{
				const float Step = FMath::Min(D - 135.f, 600.f * DeltaTime);
				O->AddActorWorldOffset(To / D * Step, true);
			}
		}
	}
	// попадание в середине движения
	if (!bAttackHit && Prog > 0.42f)
	{
		bAttackHit = true;
		ResolveAttack(T);
	}
	if (AttackTime <= 0.f)
	{
		AttackTime = 0.f;
		if (bQueuedAttack && O)
		{
			bQueuedAttack = false;
			RequestAction(ETPCombatAction::Attack, O->GetActorForwardVector());
		}
	}
}

void UTPCombatComponent::ResolveAttack(AActor* Target)
{
	if (!Target) return; // удар в воздух
	AActor* O = GetOwner();
	ATPGuard* TG = Cast<ATPGuard>(Target);
	if (!O || (TG && TG->IsKO())) return;
	UTPCombatComponent* TC = Target->FindComponentByClass<UTPCombatComponent>();
	if (!TC || TC->IsDefeated()) return;

	FVector To = Target->GetActorLocation() - O->GetActorLocation();
	const float Dz = To.Z;
	To.Z = 0.f;
	if (To.Size() > HitReach || FMath::Abs(Dz) > 160.f) return; // увернулся / далеко

	// тихое устранение: стражник не в погоне, удар со спины
	if (TG && TG->State != ETPGuardState::Chase)
	{
		const FVector FromGuard = (-To).GetSafeNormal();
		const float Rel = FVector::DotProduct(TG->GetActorForwardVector().GetSafeNormal2D(), FromGuard);
		if (Rel < -0.15f)
		{
			const FVector Where = TG->GetActorLocation();
			TG->KnockOut(TG->TakedownKODuration, O, true);
			if (UTPCombatDirector* Dir = GetDirector())
			{
				Dir->Message(TEXT("Стражник обезврежен без шума"));
				// другие стражники, которые видели устранение, настораживаются
				Dir->Witness(Where, 0.6f);
			}
			return;
		}
	}
	const bool bHeavy = Combo == 2;
	TC->ReceiveHit(AttackDamage * (bHeavy ? HeavyMul : LightMul), O, bHeavy);
}

void UTPCombatComponent::TryCounter()
{
	AActor* O = GetOwner();
	UTPCombatDirector* Dir = GetDirector();
	if (!O || !Dir || ReactTime > 0.f || !CanActOnGround()) return;

	ATPGuard* Best = nullptr;
	float BestD = CounterRange;
	for (ATPGuard* G : Dir->GetGuards())
	{
		if (!G || G->IsKO() || !G->Combat || G->Combat->WindUp <= 0.f) continue;
		const FVector D = G->GetActorLocation() - O->GetActorLocation();
		const float D2 = D.Size2D();
		if (D2 < BestD && FMath::Abs(D.Z) < 160.f) { BestD = D2; Best = G; }
	}
	if (!Best)
	{
		ReactTime = 0.25f; // промах — короткая заминка, как в первой версии
		return;
	}

	UTPCombatComponent* GC = Best->Combat;
	GC->WindUp = 0.f;
	Dir->ReleaseAttackToken(Best, 0.9f);
	FVector Face = Best->GetActorLocation() - O->GetActorLocation();
	Face.Z = 0.f;
	if (!Face.IsNearlyZero()) O->SetActorRotation(FRotator(0.f, Face.Rotation().Yaw, 0.f));
	AttackDuration = 0.5f;
	AttackTime = 0.5f;
	AttackTarget = Best;
	bAttackHit = true;   // урон наносится сразу
	bQueuedAttack = false;
	Combo = 2;
	ChainTime = 1.f;
	InvulnTime = FMath::Max(InvulnTime, 0.35f);
	PlayMontage(CounterMontage);

	GC->ApplyHit(CounterDamage, O, true);
	if (!Best->IsKO())
	{
		GC->OpenTime = 1.8f;
		Dir->Message(TEXT("Контратака! ЛКМ — добить"));
	}
	Dir->RequestSlowMo(0.45f, 0.35f);
}

void UTPCombatComponent::TryDodge(const FVector& Direction)
{
	ACharacter* C = Cast<ACharacter>(GetOwner());
	if (!C || DodgeTime > 0.f || ReactTime > 0.f || !CanActOnGround()) return;
	const UTPCombatDirector* Dir = GetDirector();
	const bool bThreat = Dir && Dir->IsHeroThreatened(C->GetActorLocation(), 450.f);
	if (CombatWeight < 0.3f && !bThreat) return; // вне боя уклонение не нужно

	FVector D = Direction.GetSafeNormal2D();
	if (D.IsNearlyZero()) D = -C->GetActorForwardVector().GetSafeNormal2D();
	InvulnTime = FMath::Max(InvulnTime, DodgeInvuln);
	DodgeTime = 0.4f;
	AttackTime = 0.f;
	bQueuedAttack = false;
	// короткий низкий отскок: в полёте трение не гасит скорость
	C->LaunchCharacter(D * DodgeSpeed + FVector(0.f, 0.f, 180.f), true, true);
	PlayMontage(DodgeMontage);
}

/* ---------------- получение урона ---------------- */

float UTPCombatComponent::ReceiveHit(float Damage, AActor* Attacker, bool bHeavy)
{
	if (IsDefeated() || bRespawning || Damage <= 0.f) return 0.f;
	if (InvulnTime > 0.f) return 0.f; // уклонение

	ATPGuard* G = GetGuardOwner();
	if (!G) return ApplyHit(Damage, Attacker, bHeavy);
	if (G->IsKO()) return 0.f;

	UTPCombatDirector* Dir = GetDirector();
	// защита пробита / после контратаки — добивание
	if (OpenTime > 0.f)
	{
		OpenTime = 0.f;
		if (Dir) Dir->RequestSlowMo(0.25f, 0.35f);
		return ApplyHit(FMath::Max(Damage, FinisherDamage), Attacker, true);
	}

	// блок: вероятность растёт с числом пропущенных ударов подряд
	const bool bChasing = G->State == ETPGuardState::Chase;
	const bool bBlock = BlockTime > 0.f
		|| (bChasing && WindUp <= 0.f && (StaggerTime <= 0.f || HitCount >= 2) && FMath::FRand() < BlockChance + BlockChancePerHit * HitCount);
	if (bBlock)
	{
		BlockTime = 0.45f;
		PlayMontage(BlockMontage);
		if (Attacker)
		{
			if (UTPCombatComponent* AC = Attacker->FindComponentByClass<UTPCombatComponent>())
				AC->ReactTime = FMath::Max(AC->ReactTime, 0.18f);
			FaceTowards(Attacker->GetActorLocation(), 1.f, 100.f);
		}
		if (bHeavy && FMath::FRand() < GuardBreakChance)
		{
			// третий удар серии пробил защиту
			OpenTime = 1.f;
			StaggerTime = 0.6f;
			HitCount = 0;
			if (Dir) Dir->Message(TEXT("Защита пробита! ЛКМ — добить"));
		}
		else if (!Dir || Dir->TryTakeAttackToken(G, true))
		{
			// ответный замах после блока — его можно контратаковать
			WindUp = 0.5f;
			StaggerTime = 0.f;
			AttackTarget = Attacker;
		}
		return 0.f;
	}
	return ApplyHit(Damage, Attacker, bHeavy);
}

float UTPCombatComponent::ApplyHit(float Damage, AActor* Attacker, bool bHeavy)
{
	if (Damage <= 0.f || IsDefeated() || bRespawning) return 0.f;
	ATPGuard* G = GetGuardOwner();
	if (G && G->IsKO()) return 0.f;

	Health = FMath::Max(0.f, Health - Damage);
	HitCount++;
	HitCountTime = 1.6f;
	RegenTimer = RegenDelay;
	OnDamaged.Broadcast(Damage, Attacker);

	if (G)
	{
		UTPCombatDirector* Dir = GetDirector();
		// лёгкий удар не прерывает замах — нужна контратака
		if (bHeavy || WindUp <= 0.05f)
		{
			StaggerTime = bHeavy ? 0.9f : 0.42f;
			WindUp = 0.f;
			Swing = 0.f;
			G->AimTime = 0.f;
			if (Dir) Dir->ReleaseAttackToken(G, -1.f);
			PlayMontage(HitMontage);
			if (Attacker)
			{
				const FVector Push = (G->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal2D();
				if (UCharacterMovementComponent* M = G->GetCharacterMovement()) M->AddImpulse(Push * (bHeavy ? 600.f : 300.f), true);
			}
		}
		G->OnCombatHit(Attacker, bHeavy);
		if (Health <= 0.f)
		{
			OpenTime = 0.f;
			OnDefeated.Broadcast(Attacker);
			G->KnockOut(G->KODuration, Attacker, false);
		}
	}
	else
	{
		ReactTime = FMath::Max(ReactTime, 0.35f);
		PlayMontage(HitMontage);
		if (Health <= 0.f) HeroDefeated(Attacker);
	}
	return Damage;
}

/* ---------------- такт ---------------- */

void UTPCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	const float Dt = DeltaTime;
	auto Dec = [Dt](float& V) { if (V > 0.f) V = FMath::Max(0.f, V - Dt); };
	Dec(InvulnTime);
	Dec(DodgeTime);
	Dec(ReactTime);
	Dec(StaggerTime);
	Dec(BlockTime);
	Dec(OpenTime);
	Dec(Swing);
	if (ChainTime > 0.f) ChainTime -= Dt;
	else Combo = 0;
	if (HitCountTime > 0.f)
	{
		HitCountTime -= Dt;
		if (HitCountTime <= 0.f) HitCount = 0;
	}

	if (GetGuardOwner()) TickGuard(Dt);
	else
	{
		TickHero(Dt);
		TickAttack(Dt);
	}
}

void UTPCombatComponent::TickGuard(float DeltaTime)
{
	ATPGuard* G = GetGuardOwner();
	if (!G) return;
	CombatWeight = G->State == ETPGuardState::Chase ? 1.f : 0.f;
	if (WindUp > 0.f)
	{
		if (G->IsKO()) { WindUp = 0.f; return; }
		if (const AActor* T = AttackTarget.Get()) FaceTowards(T->GetActorLocation(), DeltaTime, 10.f);
		WindUp -= DeltaTime;
		if (WindUp <= 0.f)
		{
			WindUp = 0.f;
			GuardStrike();
		}
	}
}

void UTPCombatComponent::GuardStrike()
{
	ATPGuard* G = GetGuardOwner();
	UTPCombatDirector* Dir = GetDirector();
	Swing = 0.32f;
	if (Dir) Dir->ReleaseAttackToken(G, 0.55f + FMath::FRand() * 0.8f);
	if (AttackMontages.Num() > 0) PlayMontage(AttackMontages[FMath::RandRange(0, AttackMontages.Num() - 1)]);
	AActor* T = AttackTarget.Get();
	if (!G || !T) return;
	const FVector To = T->GetActorLocation() - G->GetActorLocation();
	if (To.Size2D() < StrikeReach && FMath::Abs(To.Z) < 140.f)
	{
		if (UTPCombatComponent* TC = T->FindComponentByClass<UTPCombatComponent>())
		{
			const float Dealt = TC->ReceiveHit(AttackDamage, G, false);
			if (Dealt > 0.f && Dir) Dir->Message(TEXT("Удар! E — контратака, Alt — уклонение"));
		}
	}
}

void UTPCombatComponent::TickHero(float DeltaTime)
{
	ACharacter* C = Cast<ACharacter>(GetOwner());
	if (!C) return;
	UTPCombatDirector* Dir = GetDirector();
	UTPTraversalComponent* T = C->FindComponentByClass<UTPTraversalComponent>();

	// урон от падений — от паркура
	if (T)
	{
		const float Fall = T->ConsumeDamage();
		if (Fall > 0.f && !bRespawning)
		{
			Health = FMath::Max(0.f, Health - Fall);
			RegenTimer = RegenDelay;
			OnDamaged.Broadcast(Fall, nullptr);
		}
	}

	// боевая стойка плавно нарастает, когда рядом преследующая стража
	const FVector Me = C->GetActorLocation();
	const bool bGround = CanActOnGround();
	const bool bNear = Dir && Dir->IsGuardChasingNear(Me, 900.f, 300.f);
	if (bNear && bGround)
	{
		CombatWeight = FMath::Min(1.f, CombatWeight + DeltaTime * 4.f);
		CombatHold = 2.5f;
	}
	else
	{
		CombatHold -= DeltaTime;
		// во время уклонения (короткий отскок) стойку не теряем
		if (CombatHold <= 0.f || (!bGround && DodgeTime <= 0.f)) CombatWeight = FMath::Max(0.f, CombatWeight - DeltaTime * 2.5f);
	}

	// восстановление вне боя
	if (RegenTimer > 0.f) RegenTimer -= DeltaTime;
	else if (CombatWeight < 0.1f && Health > 0.f && Health < MaxHealth) Health = FMath::Min(MaxHealth, Health + RegenRate * DeltaTime);

	// последняя безопасная точка на земле (вне погони)
	SafeTimer -= DeltaTime;
	if (SafeTimer <= 0.f)
	{
		SafeTimer = 0.5f;
		const UCharacterMovementComponent* M = C->GetCharacterMovement();
		if (M && M->IsMovingOnGround() && CombatWeight < 0.05f && Health > 0.f && (!Dir || Dir->CountChasing() == 0))
			SetSafePoint(Me, C->GetActorRotation().Yaw);
	}

	if (Health <= 0.f && !bRespawning) HeroDefeated(nullptr);
}

/* ---------------- поражение и возрождение героя ---------------- */

void UTPCombatComponent::HeroDefeated(AActor* Instigator)
{
	if (bRespawning || GetGuardOwner()) return;
	bRespawning = true;
	Health = 0.f;
	OnDefeated.Broadcast(Instigator);
	if (UTPCombatDirector* Dir = GetDirector()) Dir->Message(TEXT("Вы без сил. Очнулись у последнего безопасного места."));
	Respawn();
}

void UTPCombatComponent::Respawn()
{
	if (GetGuardOwner()) return;
	AActor* O = GetOwner();
	if (!O) return;
	bRespawning = true;

	FVector P = SafePoint;
	float Yaw = SafeYaw;
	if (!bHasSafePoint)
	{
		P = O->GetActorLocation();
		if (UWorld* W = GetWorld())
			for (TActorIterator<ATPCityGenerator> It(W); It; ++It) { P = It->GetPlayerStart() + FVector(0.f, 0.f, 100.f); break; }
	}
	if (UTPTraversalComponent* T = O->FindComponentByClass<UTPTraversalComponent>())
	{
		T->ConsumeDamage();
		T->ResetTo(P, Yaw);
	}
	else
	{
		O->SetActorLocationAndRotation(P, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
		if (ACharacter* C = Cast<ACharacter>(O))
			if (UCharacterMovementComponent* M = C->GetCharacterMovement()) M->StopMovementImmediately();
	}

	Health = MaxHealth;
	AttackTime = 0.f;
	bQueuedAttack = false;
	bAttackHit = true;
	ChainTime = 0.f;
	Combo = 0;
	ReactTime = 0.f;
	DodgeTime = 0.f;
	InvulnTime = 1.5f; // короткая передышка после возрождения
	CombatWeight = 0.f;
	CombatHold = 0.f;
	RegenTimer = 0.f;
	AttackTarget.Reset();
	if (UTPCombatDirector* Dir = GetDirector()) Dir->ResetAlert();
	bRespawning = false;
	OnRespawned.Broadcast();
}
