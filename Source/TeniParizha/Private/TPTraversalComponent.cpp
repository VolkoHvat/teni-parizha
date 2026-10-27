// Тени Парижа — свободный бег (паркур) в духе Assassin's Creed.
// Перенос автомата состояний из legacy/lutece/game/lu_traversal.c (метры, Y вверх → сантиметры, Z вверх, вперёд +X).
// На земле и в полёте физику ведёт CharacterMovement (компонент задаёт скорость, ввод и толчки);
// в висе, лазании и кинематических действиях (перемах, подтягивание, перескок, спуск) — MOVE_Flying
// с нулевой скоростью, положение и поворот ставит сам компонент.
#include "TPTraversalComponent.h"
#include "TeniParizha.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

namespace
{
	/** сглаживание 0..1 (как u*u*(3-2u) в lu_traversal.c) */
	float TrSmooth01(float U)
	{
		U = FMath::Clamp(U, 0.f, 1.f);
		return U * U * (3.f - 2.f * U);
	}

	/** экспоненциальное приближение (lu_damp) */
	float TrDamp(float A, float B, float K, float Dt)
	{
		return FMath::Lerp(A, B, 1.f - FMath::Exp(-K * Dt));
	}

	bool TrHasFloor(float Z)
	{
		return Z > FTPTraversalSensors::NoFloor * 0.5f;
	}

	float TrYawOf(const FVector& Dir)
	{
		return FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Dir.Y), static_cast<float>(Dir.X)));
	}

	FQuat TrYawQuat(const FVector& Dir)
	{
		return FRotator(0.f, TrYawOf(Dir), 0.f).Quaternion();
	}

	FVector TrFlat(const FVector& V)
	{
		return FVector(V.X, V.Y, 0.f);
	}

	/** подъём с переносом: Z доходит до цели к доле ZEnd, XY начинает двигаться с доли XStart; Bump — дуга вверх */
	FVector TrStagedLerp(const FVector& From, const FVector& To, float U, float ZEnd, float XStart, float Bump)
	{
		const float Zu = TrSmooth01(U / FMath::Max(ZEnd, 0.05f));
		const float Xu = TrSmooth01((U - XStart) / FMath::Max(1.f - XStart, 0.05f));
		FVector P;
		P.X = FMath::Lerp(From.X, To.X, static_cast<double>(Xu));
		P.Y = FMath::Lerp(From.Y, To.Y, static_cast<double>(Xu));
		P.Z = FMath::Lerp(From.Z, To.Z, static_cast<double>(Zu)) + static_cast<double>(Bump * FMath::Sin(UE_PI * U));
		return P;
	}

	/** запас: капсула героя (38) толще, чем в Lutèce (30) — дальности до стен увеличены на разницу */
	constexpr float TrLegacyRadius = 30.f;
	/** отложенная хватка после «отпустить руки» (как st_t = -0.3 в оригинале) */
	constexpr float TrReleaseGrabDelay = 0.3f;
	/** скорость начала бега по стене и её убывание, см/с и см/с² */
	constexpr float TrWallRunStartVz = 520.f;
	constexpr float TrWallRunDecel = 900.f;
}

UTPTraversalComponent::UTPTraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTPTraversalComponent::BeginPlay()
{
	Super::BeginPlay();
	Owner = Cast<ACharacter>(GetOwner());
	if (!Owner)
	{
		UE_LOG(LogTeni, Warning, TEXT("UTPTraversalComponent: владелец не ACharacter — паркур отключён"));
		SetComponentTickEnabled(false);
		return;
	}
	// порядок кадра: персонаж (SetInput) → паркур → CharacterMovement
	AddTickPrerequisiteActor(Owner.Get());
	if (UCharacterMovementComponent* M = GetMove())
	{
		M->AddTickPrerequisiteComponent(this);
	}
	if (const UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		CapsuleRadius = Cap->GetScaledCapsuleRadius();
		CapsuleHalfHeight = Cap->GetScaledCapsuleHalfHeight();
	}
	AirTopZ = static_cast<float>(GetFeet().Z);
	State = ETPTraversal::Ground;
	StateTime = 0.f;
}

/* ================= общий интерфейс ================= */

void UTPTraversalComponent::SetInput(const FTPMoveInput& In)
{
	// однократные флаги «защёлкиваются» до обработки в TickComponent (порядок тиков не важен)
	const bool bJump = Input.bJump || In.bJump;
	const bool bDrop = Input.bDrop || In.bDrop;
	const bool bTurn = Input.bTurn || In.bTurn;
	Input = In;
	Input.Move.Z = 0.f;
	Input.bJump = bJump;
	Input.bDrop = bDrop;
	Input.bTurn = bTurn;
}

float UTPTraversalComponent::ConsumeDamage()
{
	const float D = PendingDamage;
	PendingDamage = 0.f;
	return D;
}

void UTPTraversalComponent::ResetTo(const FVector& Location, float Yaw)
{
	if (!Owner) Owner = Cast<ACharacter>(GetOwner());
	if (!Owner) return;
	StopLoops(0.f);
	const FRotator Rot(0.f, Yaw, 0.f);
	Owner->SetActorLocationAndRotation(Location, Rot, false, nullptr, ETeleportType::TeleportPhysics);
	if (AController* C = Owner->GetController())
	{
		C->SetControlRotation(Rot);
	}
	if (UCharacterMovementComponent* M = GetMove())
	{
		M->StopMovementImmediately();
		// если точка в воздухе — CharacterMovement сам перейдёт в падение, а паркур — в полёт
		M->SetMovementMode(MOVE_Walking);
	}
	Input = FTPMoveInput();
	Ledge = FTPLedge();
	NextLedge = FTPLedge();
	IgnoreTopZ = FTPTraversalSensors::NoFloor;
	AirTopZ = static_cast<float>(GetFeet().Z);
	bLeftGround = false;
	ClimbAnimRate = ShimmyDir = LeanAngle = 0.f;
	SetState(ETPTraversal::Ground);
}

void UTPTraversalComponent::SetState(ETPTraversal NewState)
{
	const ETPTraversal Old = State;
	State = NewState;
	StateTime = 0.f;
	if (Old != NewState)
	{
		OnStateChanged.Broadcast(Old, NewState);
	}
}

/* ================= кадр ================= */

void UTPTraversalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Owner || !GetMove()) return;

	const float Dt = FMath::Min(DeltaTime, 0.1f);
	StateTime += Dt;
	if (const UCapsuleComponent* Cap = Owner->GetCapsuleComponent())
	{
		CapsuleRadius = Cap->GetScaledCapsuleRadius();
		CapsuleHalfHeight = Cap->GetScaledCapsuleHalfHeight();
	}
	Sensors.Setup(GetWorld(), Owner.Get(), bDebugDraw);

	switch (State)
	{
	case ETPTraversal::Ground:  UpdateGround(Dt); break;
	case ETPTraversal::Jump:
	case ETPTraversal::Air:     UpdateAir(Dt); break;
	case ETPTraversal::Land:    UpdateLand(Dt); break;
	case ETPTraversal::Roll:    UpdateRoll(Dt); break;
	case ETPTraversal::Turn:    UpdateTurn(Dt); break;
	case ETPTraversal::Climb:   UpdateClimb(Dt); break;
	case ETPTraversal::Hang:    UpdateHang(Dt); break;
	case ETPTraversal::WallRun: UpdateWallRun(Dt); break;
	case ETPTraversal::ClimbUp: UpdateClimbUp(Dt); break;
	case ETPTraversal::Vault:   UpdateVault(Dt); break;
	case ETPTraversal::Drop:    UpdateDrop(Dt); break;
	case ETPTraversal::Hop:     UpdateHop(Dt); break;
	default: break;
	}

	// однократные нажатия обработаны (или проигнорированы) в этом кадре
	Input.bJump = false;
	Input.bDrop = false;
	Input.bTurn = false;

	// наклон гаснет вне бега
	if (State != ETPTraversal::Ground) LeanAngle = TrDamp(LeanAngle, 0.f, 6.f, Dt);
	if (State != ETPTraversal::Climb && State != ETPTraversal::Hop) ClimbAnimRate = 0.f;
	if (State != ETPTraversal::Hang && State != ETPTraversal::Climb) ShimmyDir = 0.f;

	if (bDebugDraw)
	{
		if (Ledge.bValid)
		{
			DrawDebugSphere(GetWorld(), Ledge.Edge, 6.f, 8, FColor::Yellow, false, -1.f, 0, 1.f);
			DrawDebugLine(GetWorld(), Ledge.Edge - Ledge.EdgeDir * 40.f, Ledge.Edge + Ledge.EdgeDir * 40.f, FColor::Yellow, false, -1.f, 0, 1.f);
			DrawDebugLine(GetWorld(), Ledge.Edge, Ledge.Edge + Ledge.WallNormal * 40.f, FColor::Blue, false, -1.f, 0, 1.f);
		}
		FVector L, R;
		if (GetHandIK(L, R) > 0.f)
		{
			DrawDebugSphere(GetWorld(), L, 4.f, 6, FColor::Magenta, false, -1.f, 0, 1.f);
			DrawDebugSphere(GetWorld(), R, 4.f, 6, FColor::Magenta, false, -1.f, 0, 1.f);
		}
	}
}

/* ================= земля ================= */

void UTPTraversalComponent::UpdateGround(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (!M->IsMovingOnGround())
	{
		if (M->IsFalling()) { EnterAir(M->Velocity); return; }
		M->SetMovementMode(MOVE_Walking);
	}

	const FVector Feet = GetFeet();
	const FVector Fwd = GetFacing();
	const FVector Move2D = TrFlat(Input.Move);
	const float MoveLen = static_cast<float>(Move2D.Size());
	const float Mag = FMath::Min(1.f, MoveLen);
	const FVector Dir = Mag > 0.05f ? Move2D / MoveLen : Fwd;
	const float HS = static_cast<float>(M->Velocity.Size2D());
	const float RE = FMath::Max(0.f, CapsuleRadius - TrLegacyRadius);

	// скорость по режиму: крадётся / свободный бег / шаг–бег по силе отклонения стика
	float MaxSpeed;
	if (Input.bCrouch) MaxSpeed = CrouchSpeed;
	else if (Input.bSprint) MaxSpeed = SprintSpeed;
	else if (Mag < 0.55f) MaxSpeed = WalkSpeed / 0.55f;
	else MaxSpeed = FMath::Lerp(WalkSpeed, RunSpeed, (Mag - 0.55f) / 0.45f) / FMath::Max(Mag, 0.05f);
	M->MaxWalkSpeed = MaxSpeed;
	M->RotationRate = FRotator(0.f, Input.bSprint ? 420.f : 720.f, 0.f);
	// крадучись не сходим с края случайно (для спуска в вис)
	M->bCanWalkOffLedges = !Input.bCrouch;

	// резкий разворот
	if (Input.bTurn && HS > 200.f)
	{
		TurnFromYaw = static_cast<float>(Owner->GetActorRotation().Yaw);
		FVector V = M->Velocity;
		V.X *= 0.3f; V.Y *= 0.3f;
		M->Velocity = V;
		SetState(ETPTraversal::Turn);
		StateDuration = ActionDuration(MontageTurn, TurnTime);
		return;
	}

	// прыжок / паркур по кнопке
	if (Input.bJump)
	{
		FTPLedge L;
		if (bAutoVault && TryStartVault(Fwd, 90.f + RE + HS * 0.12f)) return;
		if (Sensors.FindLedge(Feet, Fwd, 130.f, 245.f, 90.f + RE, L) && TryHangOrMantle(L)) return;
		FVector WN;
		bool bNoClimb = false;
		if (Sensors.ProbeWall(Feet, Fwd, 120.f, 80.f + RE, &WN, &bNoClimb) >= 0.f && !bNoClimb)
		{
			EnterClimb(WN);
			return;
		}
		const float Vz = HS > RunSpeed * 0.9f ? LeapVelocity : JumpVelocity;
		EnterJump(FVector(0.f, 0.f, Vz), false);
		return;
	}

	// свободный бег: автоматические действия
	if (Input.bSprint && Mag > 0.3f)
	{
		FVector WN;
		bool bNoClimb = false;
		const float WD = Sensors.ProbeWall(Feet, Fwd, 50.f, 55.f + RE + HS * 0.1f, &WN, &bNoClimb);
		if (WD >= 0.f)
		{
			FTPLedge L;
			if (bAutoVault && TryStartVault(Fwd, 120.f + RE)) return;
			if (bAutoClimb && Sensors.FindLedge(Feet, Fwd, 130.f, 245.f, 100.f + RE, L) && TryHangOrMantle(L)) return;
			if (bAutoClimb && !bNoClimb && WD < 45.f + RE)
			{
				// забег на стену
				SetKinematic();
				const FVector Into = -WN;
				const FVector C = Owner->GetActorLocation() + Into * (WD - HangWallOffset);
				PlaceActor(C, TrYawQuat(Into));
				ClimbWallNormal = WN;
				WallRunVz = TrWallRunStartVz;
				SetState(ETPTraversal::WallRun);
				PlayMontage(MontageWallRun);
				return;
			}
		}
		// край крыши впереди → дальний прыжок
		if (bAutoLeap && HS > 300.f)
		{
			const FVector Ahead = Feet + Fwd * (55.f + RE);
			const float FY = Sensors.FloorAt(Ahead, 50.f, 300.f);
			if (FY < Feet.Z - 120.f)
			{
				const float H = FMath::Max(HS, SprintSpeed * 0.95f);
				M->MaxWalkSpeed = FMath::Max(M->MaxWalkSpeed, H);
				EnterJump(FVector(Fwd.X * H, Fwd.Y * H, LeapVelocity), true);
				return;
			}
		}
	}

	// спуститься с края в вис (присед + движение к краю)
	if (Input.bCrouch && Mag > 0.3f)
	{
		const FVector Ahead = Feet + Dir * 50.f;
		const float FY = Sensors.FloorAt(Ahead, 50.f, 300.f);
		if (FY < Feet.Z - 160.f)
		{
			const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;   // ноги ниже кромки в висе
			const FVector Feet2 = Feet + Dir * 110.f - FVector(0.f, 0.f, HFE);
			FTPLedge L;
			if (Sensors.FindLedge(Feet2, -Dir, HFE - 47.f, HFE + 43.f, 140.f, L))
			{
				Ledge = L;
				ActFrom = Owner->GetActorLocation();
				ActFromRot = Owner->GetActorQuat();
				HangTransform(L, ActTo, ActToRot);
				SetKinematic();
				SetState(ETPTraversal::Drop);
				ActDur = ActionDuration(MontageDropToHang, DropTime);
				return;
			}
		}
	}

	// обычное движение — CharacterMovement
	if (Mag > 0.05f)
	{
		Owner->AddMovementInput(Move2D);
	}

	// наклон корпуса в повороте на бегу
	float SideRad = 0.f;
	if (Mag > 0.05f)
	{
		SideRad = FMath::DegreesToRadians(FMath::FindDeltaAngleDegrees(TrYawOf(Fwd), TrYawOf(Dir)));
	}
	const float LeanRad = FMath::Clamp(SideRad * HS * 0.0005f, -0.35f, 0.35f);
	LeanAngle = TrDamp(LeanAngle, FMath::RadiansToDegrees(LeanRad), 6.f, Dt);
}

/* ================= полёт ================= */

void UTPTraversalComponent::UpdateAir(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->MovementMode == MOVE_Flying || M->MovementMode == MOVE_None)
	{
		M->SetMovementMode(MOVE_Falling);
	}
	const FVector Feet = GetFeet();
	const FVector Fwd = GetFacing();
	const float RE = FMath::Max(0.f, CapsuleRadius - TrLegacyRadius);
	AirTopZ = FMath::Max(AirTopZ, static_cast<float>(Feet.Z));
	if (M->IsFalling()) bLeftGround = true;

	// управление в воздухе (AirControl персонажа)
	const FVector Move2D = TrFlat(Input.Move);
	if (Move2D.SizeSquared() > 0.0025)
	{
		Owner->AddMovementInput(Move2D);
	}

	if (State == ETPTraversal::Jump && StateTime > 0.25f)
	{
		// время полёта не сбрасываем, иначе хватка выключится ещё на 0.12 с
		const float T = StateTime;
		SetState(ETPTraversal::Air);
		StateTime = T;
	}

	// хватка за уступ
	if (bLedgeGrab && bLeftGround && M->Velocity.Z < 150.0 && StateTime > 0.12f && !Input.bDrop)
	{
		const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;
		FTPLedge L;
		if (Sensors.FindGrab(Feet + FVector(0.f, 0.f, HFE), Fwd, HFE, 75.f + RE, L))
		{
			EnterHang(L);
			return;
		}
		// влетел в стену на свободном бегу — зацепиться и лезть
		FVector WN;
		bool bNoClimb = false;
		if (Input.bSprint && M->Velocity.Z < 0.0 &&
			Sensors.ProbeWall(Feet, Fwd, 120.f, CapsuleRadius + 12.f, &WN, &bNoClimb) >= 0.f && !bNoClimb &&
			FVector::DotProduct(WN, Fwd) < -0.6)
		{
			EnterClimb(WN);
			return;
		}
	}

	// приземление
	if ((bLeftGround || StateTime > 0.3f) && M->IsMovingOnGround())
	{
		HandleLanding();
	}
}

void UTPTraversalComponent::HandleLanding()
{
	UCharacterMovementComponent* M = GetMove();
	const FVector Feet = GetFeet();
	const float Fall = FMath::Max(0.f, AirTopZ - static_cast<float>(Feet.Z));
	LastFallHeight = Fall;

	bool bSoft = false;
	if (M->CurrentFloor.HitResult.bBlockingHit)
	{
		bSoft = FTPTraversalSensors::HasSurfaceTag(M->CurrentFloor.HitResult, FTPTraversalSensors::TagSoft);
	}
	if (!bSoft)
	{
		bSoft = Sensors.IsSoftAt(Feet, CapsuleRadius + 20.f);
	}

	const float HS = static_cast<float>(M->Velocity.Size2D());
	const bool bCanRoll = bRoll && HS > 150.f;
	if (!bSoft && Fall > HurtHeight && !bCanRoll)
	{
		PendingDamage += (Fall - HurtHeight) * FallDamagePerCm;
	}
	OnLanded.Broadcast(Fall, bSoft);

	if (bSoft)
	{
		// стог сена: мягко и без урона
		M->StopMovementImmediately();
		SetState(ETPTraversal::Land);
		StateDuration = LandSoftTime;
	}
	else if (Fall > RollHeight && bCanRoll)
	{
		if (Fall > HurtHeight * 1.6f)
		{
			PendingDamage += (Fall - HurtHeight * 1.6f) * FallDamagePerCm * (10.f / 12.f);
		}
		SetState(ETPTraversal::Roll);
		StateDuration = ActionDuration(MontageRoll, RollTime);
	}
	else if (Fall > 220.f)
	{
		M->Velocity *= 0.2f;
		SetState(ETPTraversal::Land);
		StateDuration = ActionDuration(MontageHardLand, LandHardTime);
	}
	else if (Fall > 60.f)
	{
		SetState(ETPTraversal::Land);
		StateDuration = LandSoftTime;
	}
	else
	{
		SetState(ETPTraversal::Ground);
	}
}

/* ================= приземление, кувырок, разворот ================= */

void UTPTraversalComponent::UpdateLand(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->IsFalling()) { EnterAir(M->Velocity); return; }
	// ввод не подаём — CharacterMovement тормозит сам
	const bool bMoving = TrFlat(Input.Move).SizeSquared() > 0.04;
	if (StateTime > StateDuration * (bMoving ? 0.55f : 0.95f))
	{
		EnterGround();
	}
}

void UTPTraversalComponent::UpdateRoll(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->IsFalling()) { EnterAir(M->Velocity); return; }
	const FVector Fwd = GetFacing();
	M->MaxWalkSpeed = RunSpeed;
	Owner->AddMovementInput(Fwd, 1.f);
	if (StateTime >= StateDuration)
	{
		EnterGround();
		const FVector V = Fwd * (RunSpeed * 0.8f);
		M->Velocity = FVector(V.X, V.Y, M->Velocity.Z);
	}
}

void UTPTraversalComponent::UpdateTurn(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->IsFalling()) { EnterAir(M->Velocity); return; }
	const float U = FMath::Clamp(StateTime / FMath::Max(StateDuration, 0.05f), 0.f, 1.f);
	if (!MontageTurn)
	{
		// без анимации — плавно поворачиваем тело
		Owner->SetActorRotation(FRotator(0.f, TurnFromYaw + 180.f * TrSmooth01(U), 0.f));
	}
	if (U >= 1.f)
	{
		Owner->SetActorRotation(FRotator(0.f, TurnFromYaw + 180.f, 0.f));
		EnterGround();
	}
}

/* ================= лазание по стене ================= */

void UTPTraversalComponent::UpdateClimb(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->MovementMode != MOVE_Flying) SetKinematic();

	const FVector Feet0 = GetFeet();
	FVector N;
	bool bNoClimb = false;
	const float WD = Sensors.ProbeWall(Feet0, GetFacing(), 120.f, 120.f, &N, &bNoClimb);
	if (WD < 0.f || bNoClimb)
	{
		EnterAir(FVector::ZeroVector);
		return;
	}
	ClimbWallNormal = N;
	const FVector Fwd = -N;
	const FVector Side = FVector::CrossProduct(N, FVector::UpVector);   // вправо, если смотреть на стену
	const FVector Move2D = TrFlat(Input.Move);
	const float Up = static_cast<float>(FVector::DotProduct(Move2D, Fwd));
	const float Sd = static_cast<float>(FVector::DotProduct(Move2D, Side));

	// прижаться к стене
	FVector C = Owner->GetActorLocation() + Fwd * (WD - HangWallOffset);
	const float Vz = Up * ClimbSpeed * (Input.bSprint ? 1.6f : 1.f);
	const float Vs = Sd * ShimmySpeed;
	const FVector NewC = C + Side * (Vs * Dt) + FVector(0.f, 0.f, Vz * Dt);
	const FVector NewFeet = NewC - FVector(0.f, 0.f, CapsuleHalfHeight);
	// тело отодвинуто от стены: выступы до ~35 см (карнизы, подоконники) не мешают лезть
	const FVector Body = NewFeet - Fwd * 20.f + FVector(0.f, 0.f, 92.5f);
	if (Sensors.ProbeWall(NewFeet, Fwd, 120.f, 120.f) >= 0.f && !Sensors.Blocked(Body, 17.f, 69.5f))
	{
		C = NewC;
	}
	PlaceActor(C, TrYawQuat(Fwd));

	const float Moving = FMath::Min(1.f, FMath::Abs(Up) + FMath::Abs(Sd));
	ClimbAnimRate = Moving > 0.1f ? Moving * (Input.bSprint ? 1.6f : 1.f) : 0.f;
	ShimmyDir = FMath::Abs(Sd) > 0.1f ? FMath::Sign(Sd) : 0.f;
	PlayLoop(MontageClimbLoop);
	if (MontageClimbLoop && Owner->GetMesh())
	{
		if (UAnimInstance* AI = Owner->GetMesh()->GetAnimInstance())
		{
			AI->Montage_SetPlayRate(MontageClimbLoop, ClimbAnimRate);
		}
	}

	const FVector Feet = GetFeet();
	const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;
	// пройденный карниз снова доступен, когда руки ушли выше него
	if (TrHasFloor(IgnoreTopZ) && Feet.Z + HFE > IgnoreTopZ + 120.f)
	{
		IgnoreTopZ = FTPTraversalSensors::NoFloor;
	}
	// кромка над головой → вис
	FTPLedge L;
	if (Up > 0.1f && Sensors.FindLedge(Feet, Fwd, HFE - 37.f, HFE + 33.f, 100.f, L) && L.Edge.Z > IgnoreTopZ + 30.f)
	{
		EnterHang(L);
		return;
	}
	// спустились до земли
	if (Up < -0.1f)
	{
		const float FY = Sensors.FloorAt(Feet, 30.f, 50.f);
		if (TrHasFloor(FY) && Feet.Z - FY < 25.f)
		{
			PlaceActor(FVector(C.X, C.Y, FY + CapsuleHalfHeight + 1.f), TrYawQuat(Fwd));
			EnterGround();
			return;
		}
	}
	if (Input.bDrop)
	{
		EnterAir(N * 100.f, TrReleaseGrabDelay);
		return;
	}
	if (Input.bJump)
	{
		// отскок от стены назад
		Owner->SetActorRotation(FRotator(0.f, TrYawOf(N), 0.f));
		EnterJump(N * 450.f + FVector(0.f, 0.f, 500.f), true);
		return;
	}
}

/* ================= вис на уступе ================= */

void UTPTraversalComponent::UpdateHang(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->MovementMode != MOVE_Flying) SetKinematic();

	const FVector Fwd = -Ledge.WallNormal;
	const FVector Side = Ledge.EdgeDir;
	const FVector Move2D = TrFlat(Input.Move);
	const float Up = static_cast<float>(FVector::DotProduct(Move2D, Fwd));
	const float Sd = static_cast<float>(FVector::DotProduct(Move2D, Side));
	const FVector Feet = GetFeet();
	const bool bBraced = Sensors.ProbeWall(Feet, Fwd, 60.f, 80.f) >= 0.f;   // ноги упираются в стену
	ShimmyDir = 0.f;
	if (StateTime < 0.15f) return;

	// прыжок назад от стены (прыжок + стик от стены)
	if (Input.bJump && Up < -0.5f)
	{
		const FVector Away = Ledge.WallNormal;
		Owner->SetActorRotation(FRotator(0.f, TrYawOf(Away), 0.f));
		EnterJump(Away * 450.f + FVector(0.f, 0.f, 500.f), true);
		return;
	}
	// прыжок вбок к соседнему уступу (прыжок + стик вдоль стены)
	if (Input.bJump && FMath::Abs(Sd) > 0.5f && FMath::Abs(Sd) > Up && TryLateralHop(Sd > 0.f ? 1.f : -1.f))
	{
		return;
	}
	// подтянуться, если наверху можно встать
	if ((Up > 0.5f || (Input.bSprint && Up > 0.1f) || Input.bJump) && Ledge.bCanStand)
	{
		StartClimbUp(Ledge, true);
		return;
	}
	if ((Input.bJump || Up > 0.5f) && !Ledge.bCanStand)
	{
		// выше — уступ (скат мансарды, следующий карниз)? перескочить к нему
		const FVector Into = -Ledge.WallNormal;
		const FVector FeetTop = Ledge.Edge - Into * 5.f;
		FTPLedge L2;
		if (Sensors.FindLedge(FeetTop, Into, 50.f, 260.f, 130.f, L2) ||
			(Input.bJump && Sensors.FindLedge(Feet, Fwd, 260.f, 420.f, 100.f, L2)))
		{
			StartHop(L2, HopTime);
			return;
		}
		// сплошная стена выше — лезть дальше, мимо этого карниза
		FVector WN2;
		bool bNoClimb = false;
		if (Sensors.ProbeWall(Feet, Fwd, 260.f, 100.f, &WN2, &bNoClimb) >= 0.f && !bNoClimb)
		{
			IgnoreTopZ = static_cast<float>(Ledge.Edge.Z);
			EnterClimb(WN2);
			return;
		}
	}
	// отпустить руки
	if (Input.bDrop || (Up < -0.6f && !bBraced))
	{
		EnterAir(Ledge.WallNormal * 80.f, TrReleaseGrabDelay);
		return;
	}
	// слезть по стене вниз
	if (Up < -0.6f && bBraced)
	{
		EnterClimb(Ledge.WallNormal);
		return;
	}
	// перебежка по карнизу
	if (FMath::Abs(Sd) > 0.3f)
	{
		const float S = Sd > 0.f ? 1.f : -1.f;
		FTPLedge L2;
		if (Sensors.LedgeContinues(Ledge, S, 35.f, L2))
		{
			const FVector P = Owner->GetActorLocation() + Side * (S * ShimmySpeed * Dt);
			const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;
			FTPLedge L3;
			if (Sensors.FindLedge(P - FVector(0.f, 0.f, CapsuleHalfHeight), Fwd, HFE - 27.f, HFE + 28.f, 100.f, L3))
			{
				AttachLedge(L3);
			}
			else
			{
				PlaceActor(P, Owner->GetActorQuat());
			}
			ShimmyDir = S;
			PlayLoop(S > 0.f ? MontageShimmyRight : MontageShimmyLeft);
			return;
		}
	}
	PlayLoop(MontageHangIdle);
}

/* ================= бег по стене ================= */

void UTPTraversalComponent::UpdateWallRun(float Dt)
{
	UCharacterMovementComponent* M = GetMove();
	if (M->MovementMode != MOVE_Flying) SetKinematic();
	const FVector Fwd = GetFacing();
	const float RE = FMath::Max(0.f, CapsuleRadius - TrLegacyRadius);

	WallRunVz -= TrWallRunDecel * Dt;
	const FVector C = Owner->GetActorLocation();
	const FVector NewC = C + FVector(0.f, 0.f, WallRunVz * Dt);
	// голова упёрлась (балкон, навес) — забег окончен
	if (WallRunVz > 0.f && Sensors.Blocked(NewC + FVector(0.f, 0.f, 4.f), FMath::Max(CapsuleRadius - 6.f, 10.f), CapsuleHalfHeight))
	{
		WallRunVz = 0.f;
	}
	else
	{
		PlaceActor(NewC, Owner->GetActorQuat());
	}

	const FVector Feet = GetFeet();
	const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;
	FTPLedge L;
	if (Sensors.FindLedge(Feet, Fwd, HFE - 47.f, HFE + 33.f, 90.f + RE, L))
	{
		EnterHang(L);
		return;
	}
	if (WallRunVz < 100.f || StateTime > 0.75f)
	{
		FVector WN;
		bool bNoClimb = false;
		if (Sensors.ProbeWall(Feet, Fwd, 120.f, 90.f + RE, &WN, &bNoClimb) >= 0.f && !bNoClimb)
		{
			EnterClimb(WN);
		}
		else
		{
			EnterAir(FVector(0.f, 0.f, FMath::Max(WallRunVz, 0.f)));
		}
	}
}

/* ================= кинематические действия ================= */

void UTPTraversalComponent::UpdateClimbUp(float Dt)
{
	const float U = FMath::Clamp(StateTime / FMath::Max(ActDur, 0.05f), 0.f, 1.f);
	// сначала тело поднимается, затем переносится вперёд на крышу
	const FVector P = TrStagedLerp(ActFrom, ActTo, U, ActZEnd, ActXStart, 10.f);
	PlaceActor(P, FQuat::Slerp(ActFromRot, ActToRot, TrSmooth01(U / 0.25f)));
	if (U >= 1.f)
	{
		EnterGround();
		if (!EnsureCapsuleFree())
		{
			if (bActFromHang)
			{
				// встать не удалось — обратно в вис, больше не пытаться подтянуться на этот уступ
				EnterHang(Ledge);
				Ledge.bCanStand = false;
			}
			else
			{
				// с земли — вернуться на место
				PlaceActor(ActFrom, ActToRot);
			}
		}
	}
}

void UTPTraversalComponent::UpdateVault(float Dt)
{
	const float U = FMath::Clamp(StateTime / FMath::Max(ActDur, 0.05f), 0.f, 1.f);
	FVector P;
	if (bActMount)
	{
		// запрыгивание: сначала вверх, затем вперёд — ноги не проходят сквозь препятствие
		P = TrStagedLerp(ActFrom, ActTo, U, ActZEnd, ActXStart, 12.f);
	}
	else
	{
		// перемах: плавно вперёд по дуге над препятствием
		P = FMath::Lerp(ActFrom, ActTo, static_cast<double>(TrSmooth01(U)));
		P.Z += ActArc * FMath::Sin(UE_PI * U);
	}
	PlaceActor(P, FQuat::Slerp(ActFromRot, ActToRot, TrSmooth01(U / 0.3f)));
	if (U >= 1.f)
	{
		EnterGround();
		if (!EnsureCapsuleFree())
		{
			PlaceActor(ActFrom, ActToRot);
		}
		else if (bActKeepSpeed)
		{
			UCharacterMovementComponent* M = GetMove();
			const float Spd = (Input.bSprint ? SprintSpeed : RunSpeed) * 0.8f;
			const FVector V = GetFacing() * Spd;
			M->MaxWalkSpeed = FMath::Max(M->MaxWalkSpeed, Spd);
			M->Velocity = FVector(V.X, V.Y, 0.f);
		}
	}
}

void UTPTraversalComponent::UpdateDrop(float Dt)
{
	const float U = FMath::Clamp(StateTime / FMath::Max(ActDur, 0.05f), 0.f, 1.f);
	// сначала шаг за край, затем вниз в вис
	const float Xu = TrSmooth01(U / 0.5f);
	const float Zu = TrSmooth01((U - 0.25f) / 0.75f);
	FVector P;
	P.X = FMath::Lerp(ActFrom.X, ActTo.X, static_cast<double>(Xu));
	P.Y = FMath::Lerp(ActFrom.Y, ActTo.Y, static_cast<double>(Xu));
	P.Z = FMath::Lerp(ActFrom.Z, ActTo.Z, static_cast<double>(Zu));
	PlaceActor(P, FQuat::Slerp(ActFromRot, ActToRot, TrSmooth01(U / 0.6f)));
	if (U >= 1.f)
	{
		EnterHang(Ledge);
	}
}

void UTPTraversalComponent::UpdateHop(float Dt)
{
	const float U = FMath::Clamp(StateTime / FMath::Max(ActDur, 0.05f), 0.f, 1.f);
	const float E = TrSmooth01(U);
	FVector P = FMath::Lerp(ActFrom, ActTo, static_cast<double>(E));
	P.Z += 25.0 * FMath::Sin(UE_PI * U);
	PlaceActor(P, FQuat::Slerp(ActFromRot, ActToRot, E));
	ClimbAnimRate = 1.4f;
	if (U >= 1.f)
	{
		EnterHang(NextLedge);
	}
}

/* ================= переходы ================= */

void UTPTraversalComponent::EnterGround()
{
	UCharacterMovementComponent* M = GetMove();
	StopLoops();
	if (M && !M->IsMovingOnGround())
	{
		M->SetMovementMode(MOVE_Walking);
	}
	SetState(ETPTraversal::Ground);
}

void UTPTraversalComponent::EnterAir(const FVector& Velocity, float GrabDelay)
{
	UCharacterMovementComponent* M = GetMove();
	StopLoops();
	if (M)
	{
		M->SetMovementMode(MOVE_Falling);
		M->Velocity = Velocity;
	}
	AirTopZ = static_cast<float>(GetFeet().Z);
	bLeftGround = true;
	SetState(ETPTraversal::Air);
	StateTime = -GrabDelay;     // не хвататься сразу за только что отпущенный уступ
}

void UTPTraversalComponent::EnterJump(const FVector& LaunchVelocity, bool bOverrideXY)
{
	UCharacterMovementComponent* M = GetMove();
	StopLoops();
	bLeftGround = false;
	if (M && !M->IsMovingOnGround())
	{
		// со стены/из виса: сразу в падение с нужной скоростью
		M->SetMovementMode(MOVE_Falling);
		M->Velocity = bOverrideXY ? LaunchVelocity : FVector(M->Velocity.X, M->Velocity.Y, LaunchVelocity.Z);
		bLeftGround = true;
	}
	// толчок; CharacterMovement применит его в своём тике (после паркура в этом же кадре)
	Owner->LaunchCharacter(LaunchVelocity, bOverrideXY, true);
	AirTopZ = static_cast<float>(GetFeet().Z);
	SetState(ETPTraversal::Jump);
}

void UTPTraversalComponent::EnterHang(const FTPLedge& L)
{
	AttachLedge(L);
	SetState(ETPTraversal::Hang);
	PlayLoop(MontageHangIdle);
}

void UTPTraversalComponent::EnterClimb(const FVector& WallNormal)
{
	SetKinematic();
	const FVector N = TrFlat(WallNormal).GetSafeNormal();
	const FVector Into = N.IsNearlyZero() ? GetFacing() : -N;
	FVector C = Owner->GetActorLocation();
	const float WD = Sensors.ProbeWall(GetFeet(), Into, 120.f, 150.f);
	if (WD >= 0.f)
	{
		C += Into * (WD - HangWallOffset);
	}
	PlaceActor(C, TrYawQuat(Into));
	ClimbWallNormal = -Into;
	SetState(ETPTraversal::Climb);
	PlayLoop(MontageClimbLoop);
}

void UTPTraversalComponent::StartHop(const FTPLedge& L, float Duration)
{
	NextLedge = L;
	ActFrom = Owner->GetActorLocation();
	ActFromRot = Owner->GetActorQuat();
	HangTransform(L, ActTo, ActToRot);
	ActDur = FMath::Max(Duration, 0.1f);
	SetKinematic();
	StopLoops(0.15f);
	SetState(ETPTraversal::Hop);
}

bool UTPTraversalComponent::TryStartVault(const FVector& Dir, float Reach)
{
	const FVector Feet = GetFeet();
	FTPLedge L;
	float Depth = 0.f;
	if (!Sensors.FindVault(Feet, Dir, Reach, L, Depth)) return false;

	const FVector Into = -L.WallNormal;
	const float TopZ = static_cast<float>(L.Edge.Z);
	FVector ToFeet;
	if (Depth > 0.f)
	{
		// перемах: приземлиться за препятствием
		ToFeet = FVector(L.Edge.X, L.Edge.Y, Feet.Z) + Into * (Depth + 75.f);
		const float FY = Sensors.FloorAt(ToFeet, 100.f, 200.f);
		if (TrHasFloor(FY)) ToFeet.Z = FY;
		bActKeepSpeed = true;
		bActMount = false;
	}
	else
	{
		// широкое препятствие — запрыгнуть на него
		ToFeet = L.Edge + Into * 50.f;
		const float FY = Sensors.FloorAt(ToFeet, 60.f, 60.f);
		ToFeet.Z = TrHasFloor(FY) ? FY : TopZ;
		bActKeepSpeed = TrFlat(Input.Move).SizeSquared() > 0.09;
		bActMount = true;
		ActZEnd = 0.6f;
		ActXStart = 0.3f;
	}
	Ledge = L;
	ActFrom = Owner->GetActorLocation();
	ActTo = ToFeet + FVector(0.f, 0.f, CapsuleHalfHeight + 2.f);
	ActFromRot = Owner->GetActorQuat();
	ActToRot = TrYawQuat(Into);
	ActArc = FMath::Max(0.f, TopZ + 25.f - static_cast<float>(FMath::Max(Feet.Z, ToFeet.Z)));
	SetKinematic();
	SetState(ETPTraversal::Vault);
	ActDur = ActionDuration(MontageVault, VaultTime);
	return true;
}

bool UTPTraversalComponent::TryHangOrMantle(const FTPLedge& L)
{
	if (!L.bValid) return false;
	const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;   // высота кромки над ногами в висе
	// кромка достаточно высоко — повиснуть (ноги не уходят в землю больше чем на 25 см)
	if (L.Height >= HFE - 25.f)
	{
		StartHop(L, 0.35f);
		return true;
	}
	// низкий уступ, на который можно встать — сразу забраться
	if (L.bCanStand)
	{
		StartClimbUp(L, false);
		return true;
	}
	return false;
}

void UTPTraversalComponent::StartClimbUp(const FTPLedge& L, bool bFromHang)
{
	const FTPLedge Target = L;   // копия: L может быть самим Ledge
	const FVector Into = -Target.WallNormal;
	const FVector SP = Target.Edge + Into * 50.f;
	const float TopZ = static_cast<float>(Target.Edge.Z);
	float FY = Sensors.FloorAt(FVector(SP.X, SP.Y, TopZ + 80.f), 0.f, 140.f);
	if (!TrHasFloor(FY) || FY < TopZ - 60.f || FY > TopZ + 70.f) FY = TopZ;

	Ledge = Target;
	ActFrom = Owner->GetActorLocation();
	ActTo = FVector(SP.X, SP.Y, FY + CapsuleHalfHeight + 2.f);
	ActFromRot = Owner->GetActorQuat();
	ActToRot = TrYawQuat(Into);
	bActFromHang = bFromHang;
	// из виса: долгий подъём, затем перенос; с земли: подъём и перенос почти одновременно
	ActZEnd = bFromHang ? 0.65f : 0.6f;
	ActXStart = bFromHang ? 0.45f : 0.3f;
	SetKinematic();
	StopLoops(0.1f);
	SetState(ETPTraversal::ClimbUp);
	ActDur = bFromHang ? ActionDuration(MontageClimbUp, ClimbUpTime) : ActionDuration(MontageMantle, MantleTime);
}

bool UTPTraversalComponent::TryLateralHop(float Side)
{
	const float HFE = HangCenterBelowEdge + CapsuleHalfHeight;
	const FVector WN = Ledge.WallNormal;
	static const float Dists[3] = { 90.f, 140.f, 190.f };
	for (const float D : Dists)
	{
		const FVector P = Ledge.Edge + Ledge.EdgeDir * (Side * D);
		const FVector Feet = FVector(P.X, P.Y, Ledge.Edge.Z - HFE) + WN * 50.f;
		FTPLedge L2;
		if (Sensors.FindLedge(Feet, -WN, HFE - 70.f, HFE + 70.f, 100.f, L2) &&
			FVector::DotProduct(L2.WallNormal, WN) > 0.5)
		{
			StartHop(L2, HopTime * 0.8f);
			return true;
		}
	}
	return false;
}

/* ================= вспомогательное ================= */

UCharacterMovementComponent* UTPTraversalComponent::GetMove() const
{
	return Owner ? Owner->GetCharacterMovement() : nullptr;
}

FVector UTPTraversalComponent::GetFeet() const
{
	if (!Owner) return FVector::ZeroVector;
	return Owner->GetActorLocation() - FVector(0.f, 0.f, CapsuleHalfHeight);
}

FVector UTPTraversalComponent::GetFacing() const
{
	if (!Owner) return FVector::ForwardVector;
	const FVector F = TrFlat(Owner->GetActorForwardVector()).GetSafeNormal();
	return F.IsNearlyZero() ? FVector::ForwardVector : F;
}

void UTPTraversalComponent::HangTransform(const FTPLedge& L, FVector& OutCenter, FQuat& OutRot) const
{
	OutCenter = L.Edge + L.WallNormal * HangWallOffset;
	OutCenter.Z = L.Edge.Z - HangCenterBelowEdge;
	OutRot = TrYawQuat(-L.WallNormal);
}

void UTPTraversalComponent::AttachLedge(const FTPLedge& L)
{
	Ledge = L;
	// в висе ноги всегда на одной высоте под кромкой — для перебежки (LedgeContinues)
	Ledge.Height = HangCenterBelowEdge + CapsuleHalfHeight;
	FVector C;
	FQuat R;
	HangTransform(L, C, R);
	SetKinematic();
	PlaceActor(C, R);
}

void UTPTraversalComponent::SetKinematic()
{
	UCharacterMovementComponent* M = GetMove();
	if (!M) return;
	M->StopMovementImmediately();
	if (M->MovementMode != MOVE_Flying)
	{
		M->SetMovementMode(MOVE_Flying);
	}
}

void UTPTraversalComponent::PlaceActor(const FVector& Center, const FQuat& Rot)
{
	if (Owner)
	{
		Owner->SetActorLocationAndRotation(Center, Rot, false, nullptr, ETeleportType::None);
	}
}

bool UTPTraversalComponent::EnsureCapsuleFree()
{
	const FVector C = Owner->GetActorLocation();
	const float R = FMath::Max(CapsuleRadius - 2.f, 5.f);
	const float H = FMath::Max(CapsuleHalfHeight - 2.f, R + 1.f);
	if (!Sensors.Blocked(C, R, H)) return true;
	// немного приподнять
	static const float Lifts[3] = { 10.f, 20.f, 35.f };
	for (const float Dz : Lifts)
	{
		const FVector P = C + FVector(0.f, 0.f, Dz);
		if (!Sensors.Blocked(P, R, H))
		{
			Owner->SetActorLocation(P);
			return true;
		}
	}
	// поиск ближайшего свободного места движком
	FVector Spot = C;
	if (UWorld* W = GetWorld())
	{
		if (W->FindTeleportSpot(Owner.Get(), Spot, Owner->GetActorRotation()))
		{
			Owner->SetActorLocation(Spot);
			return true;
		}
	}
	return false;
}

float UTPTraversalComponent::PlayMontage(UAnimMontage* Montage, float Rate)
{
	if (!Montage || !Owner || !Owner->GetMesh()) return 0.f;
	UAnimInstance* AI = Owner->GetMesh()->GetAnimInstance();
	if (!AI) return 0.f;
	return AI->Montage_Play(Montage, Rate, EMontagePlayReturnType::Duration);
}

void UTPTraversalComponent::PlayLoop(UAnimMontage* Montage)
{
	if (!Montage || !Owner || !Owner->GetMesh()) return;
	UAnimInstance* AI = Owner->GetMesh()->GetAnimInstance();
	if (AI && !AI->Montage_IsPlaying(Montage))
	{
		AI->Montage_Play(Montage, 1.f);
	}
}

void UTPTraversalComponent::StopLoops(float BlendOut)
{
	if (!Owner || !Owner->GetMesh()) return;
	UAnimInstance* AI = Owner->GetMesh()->GetAnimInstance();
	if (!AI) return;
	UAnimMontage* Loops[] = { MontageHangIdle.Get(), MontageClimbLoop.Get(), MontageShimmyLeft.Get(), MontageShimmyRight.Get(), MontageWallRun.Get() };
	for (UAnimMontage* Mtg : Loops)
	{
		if (Mtg && AI->Montage_IsPlaying(Mtg))
		{
			AI->Montage_Stop(BlendOut, Mtg);
		}
	}
}

float UTPTraversalComponent::ActionDuration(UAnimMontage* Montage, float Default)
{
	const float Len = PlayMontage(Montage);
	return Len > 0.05f ? FMath::Clamp(Len, 0.2f, 3.f) : Default;
}

float UTPTraversalComponent::GetHandIK(FVector& OutLeft, FVector& OutRight) const
{
	OutLeft = OutRight = FVector::ZeroVector;
	if (!Owner) return 0.f;

	auto OnEdge = [this, &OutLeft, &OutRight](const FTPLedge& L)
	{
		// ладони чуть за кромкой, на верхней грани
		const FVector Base = L.Edge - L.WallNormal * 4.f + FVector(0.f, 0.f, 2.f);
		OutLeft = Base - L.EdgeDir * HandSpread;
		OutRight = Base + L.EdgeDir * HandSpread;
	};
	const float U = FMath::Clamp(StateTime / FMath::Max(ActDur, 0.05f), 0.f, 1.f);

	switch (State)
	{
	case ETPTraversal::Hang:
		OnEdge(Ledge);
		return 1.f;
	case ETPTraversal::ClimbUp:
		OnEdge(Ledge);
		return 1.f - FMath::Clamp((U - 0.3f) / 0.3f, 0.f, 1.f);
	case ETPTraversal::Drop:
		OnEdge(Ledge);
		return FMath::Clamp((U - 0.6f) / 0.4f, 0.f, 1.f);
	case ETPTraversal::Hop:
		OnEdge(NextLedge);
		return FMath::Clamp((U - 0.7f) / 0.3f, 0.f, 1.f);
	case ETPTraversal::Climb:
	{
		// ладони на стене над головой
		const FVector N = ClimbWallNormal.IsNearlyZero() ? -GetFacing() : ClimbWallNormal;
		const FVector Right = FVector::CrossProduct(N, FVector::UpVector);
		const FVector Base = Owner->GetActorLocation() - N * HangWallOffset + FVector(0.f, 0.f, HangCenterBelowEdge - 10.f);
		OutLeft = Base - Right * HandSpread;
		OutRight = Base + Right * HandSpread;
		return 0.8f;
	}
	default:
		return 0.f;
	}
}
