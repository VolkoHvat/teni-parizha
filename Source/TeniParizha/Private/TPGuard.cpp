#include "TPGuard.h"
#include "TeniParizha.h"
#include "TPGuardController.h"
#include "TPCombatComponent.h"
#include "TPCombatDirector.h"
#include "TPTraversalComponent.h"
#include "TPEnvironment.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"

namespace TPGuardImpl
{
	UTPCombatDirector* DirectorOf(const UObject* Ctx)
	{
		UWorld* W = Ctx ? Ctx->GetWorld() : nullptr;
		return W ? W->GetSubsystem<UTPCombatDirector>() : nullptr;
	}

	/** герой крадётся: присел или идёт шагом (в первой версии — клавиша «красться») */
	bool IsHeroSneaking(const ACharacter* Hero)
	{
		if (!Hero) return false;
		if (Hero->bIsCrouched) return true;
		const UTPTraversalComponent* T = Hero->FindComponentByClass<UTPTraversalComponent>();
		if (T && T->IsCrouching()) return true; // присед героя идёт через паркур (Ctrl/C)
		const UCharacterMovementComponent* M = Hero->GetCharacterMovement();
		if (!M) return false;
		if (M->IsCrouching()) return true;
		if (!M->IsMovingOnGround()) return false;
		const float Walk = T ? T->WalkSpeed : 160.f;
		const float V = Hero->GetVelocity().Size2D();
		return V > 5.f && V < Walk * 1.2f;
	}
}

ATPGuard::ATPGuard()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(36.f, 92.f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	AIControllerClass = ATPGuardController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->bUseControllerDesiredRotation = false;
	Move->RotationRate = FRotator(0.f, 540.f, 0.f);
	Move->MaxWalkSpeed = PatrolSpeed;
	Move->BrakingDecelerationWalking = 1400.f;
	Move->MaxStepHeight = 45.f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 250.f;

	GetMesh()->SetRelativeLocation(MeshOffset);
	GetMesh()->SetRelativeRotation(MeshRotation);

	Combat = CreateDefaultSubobject<UTPCombatComponent>(TEXT("Combat"));
	Combat->AttackDamage = 15.f;

	auto MakeProp = [this](const TCHAR* Name) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false);
		return C;
	};
	HatBrim = MakeProp(TEXT("HatBrim"));
	HatBrim->SetupAttachment(GetMesh(), HatSocket);
	HatCrown = MakeProp(TEXT("HatCrown"));
	HatCrown->SetupAttachment(GetMesh(), HatSocket);
	Musket = MakeProp(TEXT("Musket"));
	Musket->SetupAttachment(GetCapsuleComponent());
	Musket->SetRelativeLocation(FVector(-24.f, 0.f, 20.f));
	Musket->SetRelativeRotation(FRotator(0.f, 0.f, 35.f));
	Musket->SetRelativeScale3D(FVector(0.035f, 0.035f, 1.3f));

	MeshCandidates = {
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny") };
	AnimClassCandidates = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C") };
}

void ATPGuard::BeginPlay()
{
	Super::BeginPlay();
	if (bMusketeer && Combat)
	{
		Combat->MaxHealth = 75.f;
		Combat->Health = 75.f;
	}
	ApplyAppearance();
	if (!GetController()) SpawnDefaultController();
	if (PatrolRoute.Num() == 0) PatrolRoute.Add(GetActorLocation());
	RouteIndex = PatrolRoute.Num() > 1 ? 1 : 0;
	LastSeen = GetActorLocation();
	StrafeSign = FMath::RandBool() ? 1.f : -1.f;
	if (UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this)) Dir->RegisterGuard(this);
}

void ATPGuard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this)) Dir->UnregisterGuard(this);
	Super::EndPlay(EndPlayReason);
}

ATPGuardController* ATPGuard::GetGuardController() const
{
	return Cast<ATPGuardController>(GetController());
}

void ATPGuard::ApplyAppearance()
{
	USkeletalMeshComponent* M = GetMesh();
	if (!M->GetSkeletalMeshAsset())
	{
		for (const FString& P : MeshCandidates)
			if (USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetSkeletalMesh(SM); break; }
	}
	if (!M->GetAnimInstance())
	{
		for (const FString& P : AnimClassCandidates)
			if (UClass* C = LoadClass<UAnimInstance>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetAnimInstanceClass(C); break; }
	}
	if (!M->GetSkeletalMeshAsset())
		UE_LOG(LogTeni, Warning, TEXT("Стражник без меша: добавьте Third Person через Content Browser → Add → Add Feature or Content Pack"));

	// мундир: синий у пехоты, темнее с красным отливом у мушкетёров; параметров может не быть — не страшно
	const FLinearColor Coat = bMusketeer ? CoatColor * 0.7f + FLinearColor(0.14f, 0.02f, 0.02f, 0.f) : CoatColor;
	static const FName ColorParams[] = { FName(TEXT("Tint")), FName(TEXT("Paint Tint")), FName(TEXT("BaseColor")), FName(TEXT("Base Color")), FName(TEXT("Color")) };
	for (int32 i = 0; i < M->GetNumMaterials(); ++i)
	{
		if (UMaterialInstanceDynamic* MID = M->CreateAndSetMaterialInstanceDynamic(i))
			for (const FName& Param : ColorParams) MID->SetVectorParameterValue(Param, Coat);
	}

	// треуголка и мушкет из простых фигур движка
	UStaticMesh* Cyl = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* ShapeMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	auto Paint = [this, ShapeMat](UStaticMeshComponent* C, const FLinearColor& Col) {
		if (!ShapeMat || !C) return;
		if (UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(ShapeMat, this))
		{
			MID->SetVectorParameterValue(TEXT("Color"), Col);
			C->SetMaterial(0, MID);
		}
	};

	const bool bHead = M->GetSkeletalMeshAsset() && M->DoesSocketExist(HatSocket);
	USceneComponent* HatParent = bHead ? static_cast<USceneComponent*>(M) : static_cast<USceneComponent*>(GetCapsuleComponent());
	const FName Sock = bHead ? HatSocket : NAME_None;
	// без кости head — шляпа просто над капсулой, ось конуса вверх
	const FVector BaseLoc = bHead ? HatOffset : FVector(0.f, 0.f, 95.f);
	const FRotator BaseRot = bHead ? HatRotation : FRotator::ZeroRotator;
	if (Cyl && HatBrim)
	{
		HatBrim->SetStaticMesh(Cyl);
		HatBrim->AttachToComponent(HatParent, FAttachmentTransformRules::KeepRelativeTransform, Sock);
		HatBrim->SetRelativeLocationAndRotation(BaseLoc, BaseRot);
		HatBrim->SetRelativeScale3D(FVector(0.46f, 0.46f, 0.035f));
		Paint(HatBrim, HatColor);
	}
	if (Cone && HatCrown)
	{
		HatCrown->SetStaticMesh(Cone);
		HatCrown->AttachToComponent(HatParent, FAttachmentTransformRules::KeepRelativeTransform, Sock);
		HatCrown->SetRelativeLocationAndRotation(BaseLoc + BaseRot.RotateVector(FVector(0.f, 0.f, 8.f)), BaseRot);
		HatCrown->SetRelativeScale3D(FVector(0.3f, 0.3f, 0.16f));
		Paint(HatCrown, bMusketeer ? FLinearColor(0.22f, 0.03f, 0.03f) : HatColor);
	}
	if (Musket)
	{
		if (Cyl && bMusketeer)
		{
			Musket->SetStaticMesh(Cyl);
			Paint(Musket, FLinearColor(0.18f, 0.1f, 0.05f));
		}
		Musket->SetVisibility(bMusketeer && Cyl != nullptr);
	}
}

/* ---------------- зрение ---------------- */

bool ATPGuard::IsNight() const
{
	if (!Environment.IsValid())
	{
		EnvSearchTimer -= 0.25f;
		if (EnvSearchTimer > 0.f) return false;
		EnvSearchTimer = 2.f;
		if (UWorld* W = GetWorld())
			for (TActorIterator<ATPEnvironment> It(W); It; ++It) { Environment = *It; break; }
	}
	return Environment.IsValid() && Environment->IsNight();
}

float ATPGuard::ComputeSightRange(const ACharacter* Hero) const
{
	float R = SightRange;
	if (const UTPTraversalComponent* T = Hero ? Hero->FindComponentByClass<UTPTraversalComponent>() : nullptr)
		if (T->IsFreeRunning()) R *= 1.25f;
	if (TPGuardImpl::IsHeroSneaking(Hero)) R *= 0.55f;
	if (IsNight()) R *= 0.75f;
	if (const UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this)) R *= Dir->GetHeroConcealment();
	return R;
}

bool ATPGuard::CanSeePoint(FVector Point, float Range, const AActor* IgnoreActor) const
{
	if (State == ETPGuardState::KO) return false;
	const FVector Me = GetActorLocation();
	FVector To = Point - Me;
	To.Z = 0.f;
	const float D = To.Size();
	if (D > Range) return false;
	if (D > 300.f)
	{
		const FVector Fwd = GetActorForwardVector().GetSafeNormal2D();
		if (FVector::DotProduct(Fwd, To / D) < FMath::Cos(FMath::DegreesToRadians(SightHalfAngle))) return false;
	}
	const FVector Eye = Me + FVector(0.f, 0.f, 70.f);
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TPGuardSight), false, this);
	if (IgnoreActor) Q.AddIgnoredActor(IgnoreActor);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Point, ECC_Visibility, Q);
}

/* ---------------- состояния ---------------- */

void ATPGuard::FaceDirection(const FVector& Dir, float Dt, float Speed)
{
	FVector F = Dir;
	F.Z = 0.f;
	if (F.IsNearlyZero()) return;
	const FRotator Want(0.f, F.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Want, Dt, Speed));
}

void ATPGuard::EnterChase(const FVector& Seen)
{
	if (State == ETPGuardState::KO) return;
	State = ETPGuardState::Chase;
	Alert = 1.f;
	Lost = 0.f;
	Unreachable = 0.f;
	SearchTime = 0.f;
	LastSeen = Seen;
}

void ATPGuard::EnterSearch(float WaitTime)
{
	State = ETPGuardState::Search;
	Wait = WaitTime;
	SearchTime = 0.f;
	AimTime = 0.f;
	if (Combat) Combat->WindUp = 0.f;
	if (UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this)) Dir->ReleaseAttackToken(this, -1.f);
}

void ATPGuard::ResetToPatrol()
{
	if (State == ETPGuardState::KO) return;
	State = ETPGuardState::Patrol;
	Alert = 0.f;
	Lost = 0.f;
	Wait = 0.f;
	AimTime = 0.f;
	Unreachable = 0.f;
	if (Combat) Combat->WindUp = 0.f;
	// ближайшая точка маршрута
	float Best = TNumericLimits<float>::Max();
	for (int32 i = 0; i < PatrolRoute.Num(); ++i)
	{
		const float D = FVector::DistSquared2D(PatrolRoute[i], GetActorLocation());
		if (D < Best) { Best = D; RouteIndex = i; }
	}
}

void ATPGuard::AddSuspicion(float Amount, const FVector& Seen)
{
	if (State == ETPGuardState::KO) return;
	Alert = FMath::Clamp(Alert + Amount, 0.f, 1.f);
	LastSeen = Seen;
	if (Alert >= 1.f && (State == ETPGuardState::Patrol || State == ETPGuardState::Suspicious))
	{
		EnterChase(Seen);
		if (UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this)) Dir->Message(TEXT("Стража! Вас заметили!"));
	}
	else if (State == ETPGuardState::Patrol) State = ETPGuardState::Suspicious;
}

void ATPGuard::OnCombatHit(AActor* Attacker, bool bHeavy)
{
	if (State == ETPGuardState::KO) return;
	UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this);
	const FVector From = Attacker ? Attacker->GetActorLocation() : GetActorLocation();
	if (State != ETPGuardState::Chase)
	{
		EnterChase(From);
		if (Dir) Dir->AlertAll(GetActorLocation(), 1600.f);
	}
	Alert = 1.f;
	LastSeen = From;
	Lost = 0.f;
	AimTime = 0.f;
	if (Dir) Dir->Panic(GetActorLocation(), 1100.f);
}

void ATPGuard::KnockOut(float Duration, AActor* By, bool bSilent)
{
	if (State == ETPGuardState::KO) return;
	UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this);
	State = ETPGuardState::KO;
	KOTimer = Duration;
	Alert = 0.f;
	AimTime = 0.f;
	Wait = 0.f;
	if (Combat)
	{
		Combat->WindUp = 0.f;
		Combat->Swing = 0.f;
		Combat->StaggerTime = 0.f;
		Combat->OpenTime = 0.f;
		Combat->BlockTime = 0.f;
	}
	if (Dir) Dir->ReleaseAttackToken(this, -1.f);
	if (ATPGuardController* AI = GetGuardController()) AI->StopGuard();

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->StopMovementImmediately();
	Move->DisableMovement();
	UCapsuleComponent* Cap = GetCapsuleComponent();
	Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Cap->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	if (Musket) Musket->SetVisibility(false);

	USkeletalMeshComponent* M = GetMesh();
	if (M->GetSkeletalMeshAsset() && M->GetPhysicsAsset())
	{
		M->SetCollisionProfileName(TEXT("Ragdoll"));
		M->SetSimulatePhysics(true);
		bRagdoll = true;
		M->WakeAllRigidBodies();
		if (By)
		{
			const FVector Push = (GetActorLocation() - By->GetActorLocation()).GetSafeNormal2D() * (bSilent ? 60.f : 260.f)
				+ FVector(0.f, 0.f, bSilent ? 0.f : 80.f);
			M->SetAllPhysicsLinearVelocity(Push, true);
		}
	}
	else
	{
		// без физического ассета — просто положить меш на землю
		M->SetRelativeRotation(MeshRotation + FRotator(0.f, 0.f, 90.f));
		M->SetRelativeLocation(MeshOffset + FVector(0.f, 0.f, 15.f));
	}

	if (!bSilent && Dir)
	{
		Dir->Panic(GetActorLocation(), 1100.f);
		Dir->Message(TEXT("Стражник повержен"));
	}
}

void ATPGuard::GetUp()
{
	USkeletalMeshComponent* M = GetMesh();
	FVector Base = GetActorLocation() - FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	if (bRagdoll)
	{
		Base = M->GetSocketLocation(TEXT("pelvis"));
		M->SetSimulatePhysics(false);
		bRagdoll = false;
	}
	M->SetCollisionProfileName(TEXT("CharacterMesh"));
	M->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	M->SetRelativeLocationAndRotation(MeshOffset, MeshRotation);

	// капсулу — на землю под тазом
	const float Half = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector NewLoc = Base + FVector(0.f, 0.f, Half);
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TPGuardGetUp), false, this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Base + FVector(0.f, 0.f, 60.f), Base - FVector(0.f, 0.f, 400.f), ECC_Visibility, Q))
		NewLoc = Hit.ImpactPoint + FVector(0.f, 0.f, Half + 2.f);
	SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));

	UCapsuleComponent* Cap = GetCapsuleComponent();
	Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Cap->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (Musket) Musket->SetVisibility(bMusketeer && Musket->GetStaticMesh() != nullptr);

	if (Combat)
	{
		Combat->Health = FMath::Min(60.f, Combat->MaxHealth);
		Combat->HitCount = 0;
		Combat->WindUp = 0.f;
		Combat->Swing = 0.f;
		Combat->StaggerTime = 0.f;
	}
	// очнулся — ищет обидчика вокруг себя
	State = ETPGuardState::Search;
	Wait = 5.f;
	Alert = 0.5f;
	Lost = 0.f;
	SearchTime = 0.f;
	LastSeen = NewLoc;
}

/* ---------------- мушкет ---------------- */

void ATPGuard::SpawnMuzzleFlash()
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
	if (!L) return;
	L->SetupAttachment(GetRootComponent());
	L->SetRelativeLocation(FVector(70.f, 0.f, 45.f));
	L->SetMobility(EComponentMobility::Movable);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(6000.f);
	L->SetAttenuationRadius(1200.f);
	L->SetLightColor(FLinearColor(1.f, 0.62f, 0.25f));
	L->SetCastShadows(false);
	L->RegisterComponent();
	TWeakObjectPtr<UPointLightComponent> Weak(L);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateLambda([Weak]()
	{
		if (Weak.IsValid()) Weak->DestroyComponent();
	}), 0.09f, false);
}

void ATPGuard::FireMusket(ACharacter* Hero)
{
	UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this);
	AimTime = 0.f;
	Reload = ReloadDuration;
	SpawnMuzzleFlash();
	if (Dir) Dir->Panic(GetActorLocation(), 1600.f);
	float Dealt = 0.f;
	if (Hero)
	{
		UTPCombatComponent* HC = Hero->FindComponentByClass<UTPCombatComponent>();
		// укрытие спасает: нужна прямая видимость в момент выстрела
		if (HC && CanSeePoint(Hero->GetActorLocation() + FVector(0.f, 0.f, 30.f), MusketMaxRange + 200.f, Hero))
			Dealt = HC->ReceiveHit(MusketDamage, this, true);
	}
	if (Dir) Dir->Message(Dealt > 0.f ? TEXT("Ранение! Alt — уклониться, укрытие спасёт") : TEXT("Пуля просвистела мимо"));
}

/* ---------------- погоня и бой ---------------- */

void ATPGuard::TickChase(float Dt, ACharacter* Hero, bool bSee, float& OutMoveSpeed, bool& bOutMove, bool& bOutManualFacing)
{
	UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this);
	ATPGuardController* AI = GetGuardController();
	const FVector Me = GetActorLocation();
	const FVector HL = Hero->GetActorLocation();
	FVector To = HL - Me;
	const float Dz = To.Z;
	To.Z = 0.f;
	const float D = To.Size();

	bOutMove = true;
	OutMoveSpeed = ChaseSpeed;
	if (Combat->StaggerTime > 0.f || Combat->Swing > 0.f || Combat->WindUp > 0.f || Combat->BlockTime > 0.f) bOutMove = false;

	// мушкетёр целится издалека, если видит героя
	if (bMusketeer && bSee && D > MusketMinRange && D < MusketMaxRange && Reload <= 0.f && Combat->StaggerTime <= 0.f && Combat->WindUp <= 0.f)
	{
		bOutMove = false;
		bOutManualFacing = true;
		FaceDirection(To, Dt, 8.f);
		AimTime += Dt;
		if (AimTime > AimDuration) FireMusket(Hero);
	}
	else AimTime = FMath::Max(0.f, AimTime - Dt * 2.f);

	// ближний бой: кружат вокруг героя, нападают по одному (жетон), с замахом — его можно контратаковать
	bool bCircle = false;
	if (D < MeleeRange && FMath::Abs(Dz) < 150.f && AimTime <= 0.f)
	{
		bOutMove = false;
		bOutManualFacing = true;
		FaceDirection(To, Dt, 10.f);
		if (StrafeSign == 0.f) StrafeSign = FMath::RandBool() ? 1.f : -1.f;
		bCircle = Combat->StaggerTime <= 0.f && Combat->WindUp <= 0.f && Combat->Swing <= 0.f;

		const UCharacterMovementComponent* HM = Hero->GetCharacterMovement();
		const bool bHeroGround = HM && HM->IsMovingOnGround();
		const bool bReady = Combat->WindUp <= 0.f && Combat->StaggerTime <= 0.f && Combat->OpenTime <= 0.f
			&& Combat->BlockTime <= 0.f && Combat->Swing <= 0.f;
		if (bHeroGround && bReady && (!Dir || Dir->CanTakeAttackToken(this)))
			Combat->RequestAction(ETPCombatAction::Attack, To);
	}

	if (!bOutMove && AI) AI->StopGuard();
	if (bCircle && Combat->WindUp <= 0.f)
	{
		const FVector Fwd = To.GetSafeNormal();
		const FVector Side(-Fwd.Y, Fwd.X, 0.f);
		const FVector Mv = Fwd * ((D - CircleRadius) * 1.5f) + Side * (StrafeSign * 110.f);
		const float Sp = Mv.Size();
		if (Sp > 5.f)
		{
			GetCharacterMovement()->MaxWalkSpeed = FMath::Clamp(Sp, 60.f, 220.f);
			AddMovementInput(Mv / Sp, 1.f);
		}
		if (FMath::FRand() < Dt * 0.35f) StrafeSign = -StrafeSign;
	}

	// герой высоко на крыше и недосягаем — бросить погоню
	if (HL.Z - Me.Z > UnreachableHeight && D < 2200.f)
	{
		Unreachable += Dt;
		if (Unreachable > UnreachableTime)
		{
			Unreachable = 0.f;
			EnterSearch(6.f);
			Alert = 0.4f;
			return;
		}
	}
	else Unreachable = 0.f;

	if (!bSee && Lost > 8.f) EnterSearch(7.f);
}

void ATPGuard::Tick(float Dt)
{
	Super::Tick(Dt);

	if (State == ETPGuardState::KO)
	{
		KOTimer -= Dt;
		if (KOTimer <= 0.f) GetUp();
		return;
	}
	if (!Combat) return;

	UTPCombatDirector* Dir = TPGuardImpl::DirectorOf(this);
	ATPGuardController* AI = GetGuardController();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	ACharacter* Hero = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (Reload > 0.f) Reload -= Dt;
	if (!Hero)
	{
		if (AI) AI->StopGuard();
		return;
	}

	const FVector Me = GetActorLocation();
	const FVector HL = Hero->GetActorLocation();
	const float D = FVector::Dist2D(HL, Me);
	const UTPCombatComponent* HeroCombat = Hero->FindComponentByClass<UTPCombatComponent>();
	const bool bHeroDown = HeroCombat && HeroCombat->IsDefeated();
	const float R = ComputeSightRange(Hero);
	const bool bSee = !bHeroDown && R > 1.f && D < R && CanSeePoint(HL + FVector(0.f, 0.f, 30.f), R, Hero);

	// тревога растёт, пока герой на виду (тем быстрее, чем он ближе), и медленно спадает
	if (bSee)
	{
		Alert += (0.7f + (1.f - D / R) * 2.4f) * Dt;
		LastSeen = HL;
		Lost = 0.f;
		if (Alert >= 1.f && (State == ETPGuardState::Patrol || State == ETPGuardState::Suspicious))
		{
			EnterChase(HL);
			if (Dir)
			{
				Dir->Message(TEXT("Вас заметили!"));
				Dir->AlertAll(Me, 2200.f);
			}
		}
		else if (Alert > 0.3f && State == ETPGuardState::Patrol) State = ETPGuardState::Suspicious;
	}
	else
	{
		Alert = FMath::Max(0.f, Alert - (State == ETPGuardState::Chase ? 0.04f : 0.22f) * Dt);
		Lost += Dt;
	}
	Alert = FMath::Min(Alert, 1.f);

	bool bMove = false;
	bool bManual = false;
	float Speed = PatrolSpeed;
	FVector Goal = Me;
	float Acceptance = 100.f;

	switch (State)
	{
	case ETPGuardState::Patrol:
		if (PatrolRoute.Num() >= 2)
		{
			RouteIndex = RouteIndex % PatrolRoute.Num();
			Goal = PatrolRoute[RouteIndex];
			if (Wait > 0.f) Wait -= Dt;
			else if (FVector::Dist2D(Goal, Me) < 130.f)
			{
				Wait = FMath::FRandRange(2.f, 4.f);
				RouteIndex = (RouteIndex + 1) % PatrolRoute.Num();
			}
			else bMove = true;
		}
		else
		{
			// пост: медленно осматривается
			bManual = true;
			AddActorWorldRotation(FRotator(0.f, 20.f * Dt, 0.f));
		}
		break;

	case ETPGuardState::Suspicious:
		bManual = true;
		FaceDirection(LastSeen - Me, Dt, 4.f);
		if (!bSee && Alert < 0.1f) State = ETPGuardState::Patrol;
		break;

	case ETPGuardState::Chase:
		TickChase(Dt, Hero, bSee, Speed, bMove, bManual);
		if (State == ETPGuardState::Chase) Goal = bSee ? HL : LastSeen;
		else bMove = false;
		break;

	case ETPGuardState::Search:
		SearchTime += Dt;
		if (FVector::Dist2D(LastSeen, Me) < 150.f || SearchTime > 25.f)
		{
			bManual = true;
			AddActorWorldRotation(FRotator(0.f, 45.f * Dt, 0.f));
			Wait -= Dt;
			if (Wait <= 0.f) ResetToPatrol();
		}
		else
		{
			bMove = true;
			Goal = LastSeen;
			Speed = SearchSpeed;
		}
		if (bSee && Alert > 0.5f) State = ETPGuardState::Suspicious;
		break;

	default:
		break;
	}

	if (bMove && AI)
	{
		Move->MaxWalkSpeed = Speed;
		AI->MoveGuardTo(Goal, Acceptance, Dt);
	}
	else if (State != ETPGuardState::Chase && AI)
	{
		AI->StopGuard();
	}
	Move->bOrientRotationToMovement = !bManual;
}
