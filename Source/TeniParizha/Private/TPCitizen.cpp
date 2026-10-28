// Тени Парижа — житель квартала.
#include "TPCitizen.h"
#include "TeniParizha.h"
#include "TPIdentitySubsystem.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

// Подсистему боя пишет другой агент: подключаем, только если заголовок уже есть
#if __has_include("TPCombatDirector.h")
#include "TPCombatDirector.h"
#define TP_HAS_COMBAT_DIRECTOR 1
#else
#define TP_HAS_COMBAT_DIRECTOR 0
#endif

namespace TPCitizenText
{
	static const TCHAR* Brush[6] = {TEXT("Pardon!"), TEXT("Осторожнее, месье!"), TEXT("Простите…"), TEXT("Куда так спешите?"), TEXT("Ох!"), TEXT("Полегче!")};
	static const TCHAR* BumpLines[6] = {TEXT("Эй, смотри, куда бежишь!"), TEXT("Ай! Вот наглец!"), TEXT("Ну и манеры!"), TEXT("Совсем ошалел?!"), TEXT("Под ноги смотри!"), TEXT("Сумасшедший!")};
	static const TCHAR* Down[6] = {TEXT("Ой-ой-ой!"), TEXT("Помогите, сбили с ног!"), TEXT("Негодяй!"), TEXT("Стража! Стража!"), TEXT("Моя спина!"), TEXT("Да что ж такое!")};
	static const TCHAR* See[3] = {TEXT("Что творится!"), TEXT("Вы видели?!"), TEXT("Безобразие!")};
	static const TCHAR* GetUp[4] = {TEXT("Ох, мои кости…"), TEXT("Носятся тут всякие…"), TEXT("Куда катится Париж!"), TEXT("Хоть бы извинился!")};
	static const TCHAR* Heavy[4] = {TEXT("Ха! Меня так просто не свалишь."), TEXT("Полегче, курьер!"), TEXT("Береги силы для гонки!"), TEXT("Эй, эй!")};
	static const TCHAR* Scream[5] = {TEXT("Драка! Бегите!"), TEXT("Убивают!"), TEXT("Мама родная!"), TEXT("Спасайся кто может!"), TEXT("Стреляют!")};
	static const TCHAR* Calm[3] = {TEXT("Опять стража лютует…"), TEXT("Не моё это дело."), TEXT("Ну и времена!")};
}

ATPCitizen::ATPCitizen()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	AIControllerClass = AAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 360.f, 0.f);
	Move->MaxWalkSpeed = WalkSpeed;
	Move->BrakingDecelerationWalking = 600.f;
	Move->bUseRVOAvoidance = true;
	Move->AvoidanceConsiderationRadius = 220.f;
	Move->SetWalkableFloorAngle(50.f);
	Move->MaxStepHeight = 40.f;

	GetMesh()->SetRelativeLocation(MeshRelLoc);
	GetMesh()->SetRelativeRotation(MeshRelRot);
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));

	// манекен пакета Third Person UE4 (женского меша в 4.27 нет — у всех один, рост и цвет различают)
	MaleMeshCandidates = {
		TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"),
		TEXT("/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin") };
	FemaleMeshCandidates = {
		TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin_Female.SK_Mannequin_Female"),
		TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"),
		TEXT("/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin") };
	AnimClassCandidates = {
		TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C") };
}

void ATPCitizen::InitCitizen(int32 InIdentityIndex, const TArray<FVector>& InWalkPoints, bool bInStationary)
{
	IdentityIndex = InIdentityIndex;
	WalkPoints = InWalkPoints;
	bStationary = bInStationary;
	Rng.Initialize(InIdentityIndex * 7919 + 17);
	if (HasActorBegunPlay()) ApplyAppearance();
}

void ATPCitizen::BeginPlay()
{
	Super::BeginPlay();
	HomeLocation = GetActorLocation();
	HomeYaw = GetActorRotation().Yaw;
	ApplyAppearance();
	if (bStationary) { Mode = ETPCitizenMode::Stand; ModeTime = 0.f; }
	else EnterIdle(0.5f, 4.f);
#if TP_HAS_COMBAT_DIRECTOR
	if (UWorld* W = GetWorld())
		if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
			Dir->OnPanic.AddUniqueDynamic(this, &ATPCitizen::HandlePanic);
#endif
}

void ATPCitizen::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if TP_HAS_COMBAT_DIRECTOR
	if (UWorld* W = GetWorld())
		if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
			Dir->OnPanic.RemoveDynamic(this, &ATPCitizen::HandlePanic);
#endif
	Super::EndPlay(EndPlayReason);
}

/* ---------------- паника ---------------- */

void ATPCitizen::HandlePanic(FVector Location, float Radius)
{
	if (Mode == ETPCitizenMode::Down || Mode == ETPCitizenMode::Racing) return;
	if (FVector::Dist(Location, GetActorLocation()) > Radius) return;
	Panic(Location);
}

void ATPCitizen::Panic(const FVector& From)
{
	LookAtFor(From, 1.5f);
	if (bStationary)
	{
		// бегуны Лиги и художник не из пугливых
		if (BarkTime <= 0.f && Rng.FRand() < 0.3f) Say(TPCitizenText::Calm[Rng.RandRange(0, 2)]);
		return;
	}
	if (bFleeing && Mode == ETPCitizenMode::Walk) return;   // уже бежит
	if (BarkTime <= 0.f && Rng.FRand() < 0.45f) Say(TPCitizenText::Scream[Rng.RandRange(0, 4)], 2.f);
	if (Mode == ETPCitizenMode::Talk || Mode == ETPCitizenMode::Sidestep || Mode == ETPCitizenMode::Walk) StopWalking();
	ChatPartner = nullptr;   // собеседник получит то же событие и убежит сам

	// точка прогулки подальше от шума, по возможности в сторону «от него»
	const FVector Here = GetActorLocation();
	FVector Away = Here - From; Away.Z = 0.f;
	if (!Away.Normalize()) Away = FRotator(0.f, Rng.FRandRange(-180.f, 180.f), 0.f).Vector();
	FVector Best = Here + Away * 1500.f;
	float BestScore = -1e9f;
	for (const FVector& P : WalkPoints)
	{
		const FVector D = P - Here;
		const float Dist = D.Size2D();
		if (Dist < 600.f || Dist > 4500.f) continue;
		const float Score = FVector::DotProduct(D.GetSafeNormal2D(), Away) * 2.f + FVector::Dist2D(P, From) / 2000.f + Rng.FRand() * 0.3f;
		if (Score > BestScore) { BestScore = Score; Best = P; }
	}
	Target = Best;
	bFleeing = true;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed * 2.6f;
	IssueMove();
	ModeDuration = 10.f;
}

/* ---------------- внешность ---------------- */

UStaticMeshComponent* ATPCitizen::AddPart(UStaticMesh* InMesh, const FVector& RelLoc, const FVector& Scale, const FColor& Color)
{
	if (!InMesh) return nullptr;
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetStaticMesh(InMesh);
	C->SetupAttachment(GetCapsuleComponent());
	C->SetRelativeLocation(RelLoc);
	C->SetRelativeScale3D(Scale);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetGenerateOverlapEvents(false);
	C->SetCanEverAffectNavigation(false);
	C->RegisterComponent();
	if (UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet))
	{
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(Basic, this);
		M->SetVectorParameterValue(TEXT("Color"), FLinearColor(Color));
		C->SetMaterial(0, M);
		Mids.Add(M);
	}
	Accessories.Add(C);
	return C;
}

void ATPCitizen::SetAccessoriesVisible(bool bVisible)
{
	for (UStaticMeshComponent* C : Accessories)
		if (C) C->SetVisibility(bVisible);
}

void ATPCitizen::ApplyAppearance()
{
	if (bAppearanceDone || IdentityIndex < 0) return;
	const UGameInstance* GI = GetGameInstance();
	const UTPIdentitySubsystem* Ids = GI ? GI->GetSubsystem<UTPIdentitySubsystem>() : nullptr;
	if (!Ids) return;
	bAppearanceDone = true;
	const FTPIdentity& Id = Ids->Get(IdentityIndex);
	bFemale = Id.bFemale;
	bHeavy = Id.Role == ETPCitizenRole::Runner || Id.Role == ETPCitizenRole::Painter;

	// меш и анимации шаблона Third Person (если пакета нет — простая фигура из примитивов)
	USkeletalMeshComponent* M = GetMesh();
	for (const FString& P : (bFemale ? FemaleMeshCandidates : MaleMeshCandidates))
		if (USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetSkeletalMesh(SM); break; }
	for (const FString& P : AnimClassCandidates)
		if (UClass* C = LoadClass<UAnimInstance>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetAnimInstanceClass(C); break; }
	const bool bHasMesh = M->SkeletalMesh != nullptr;

	// рост — масштабом, цвета — динамическими материалами
	SetActorScale3D(FVector(FMath::Clamp(Id.Height, 0.85f, 1.1f)));
	if (bHasMesh)
	{
		const int32 NumMats = M->GetNumMaterials();
		for (int32 I = 0; I < NumMats; ++I)
		{
			UMaterialInstanceDynamic* D = M->CreateDynamicMaterialInstance(I);
			if (!D) continue;
			const FLinearColor C = FLinearColor(I == 0 ? Id.Coat : Id.Pants);
			// разные версии манекена называют параметр цвета по-разному
			D->SetVectorParameterValue(TEXT("BodyColor"), C);   // M_UE4Man_Body манекена 4.27
			D->SetVectorParameterValue(TEXT("Tint"), C);
			D->SetVectorParameterValue(TEXT("Paint Tint"), C);
			D->SetVectorParameterValue(TEXT("BaseColor"), C);
			D->SetVectorParameterValue(TEXT("Base Color"), C);
			D->SetVectorParameterValue(TEXT("Color"), C);
			Mids.Add(D);
		}
	}

	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* Cone = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	if (!bHasMesh)
	{
		// запасная фигура: туловище, голова
		AddPart(Cylinder, FVector(0.f, 0.f, -18.f), FVector(0.5f, 0.42f, 1.42f), Id.Coat);
		AddPart(Sphere, FVector(0.f, 0.f, 68.f), FVector(0.3f), Id.Skin);
	}

	// одежда и головные уборы по сословию (крепятся к капсуле: работают с любым скелетом)
	const FColor Dark(30, 26, 28);
	const float HeadZ = bFemale ? 72.f : 79.f;
	if (bFemale && Id.Class != ETPSocialClass::Soldier)
		AddPart(Cone, FVector(0.f, 0.f, -46.f), FVector(0.62f, 0.62f, 0.86f), Id.Coat);   // юбка
	switch (Id.Role)
	{
	case ETPCitizenRole::Painter:
		AddPart(Sphere, FVector(0.f, -3.f, HeadZ + 6.f), FVector(0.3f, 0.3f, 0.1f), FColor(70, 30, 60));    // берет
		break;
	case ETPCitizenRole::Runner:
		AddPart(Cylinder, FVector(0.f, 0.f, 2.f), FVector(0.4f, 0.4f, 0.08f), Id.Trim);                    // красный кушак Лиги
		break;
	default:
		switch (Id.Class)
		{
		case ETPSocialClass::Noble:
			if (bFemale) AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 6.f), FVector(0.55f, 0.55f, 0.03f), Id.Trim);
			else { AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 2.f), FVector(0.44f, 0.44f, 0.03f), Dark); AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 9.f), FVector(0.24f, 0.24f, 0.13f), Dark); }
			break;
		case ETPSocialClass::Clergy:
			if (bFemale) AddPart(Sphere, FVector(0.f, 0.f, HeadZ + 2.f), FVector(0.3f, 0.3f, 0.24f), FColor(250, 248, 248));
			else AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 3.f), FVector(0.4f, 0.4f, 0.03f), Dark);
			break;
		case ETPSocialClass::Labor:
		case ETPSocialClass::Sailor:
			if (bFemale) AddPart(Sphere, FVector(0.f, 0.f, HeadZ + 3.f), FVector(0.27f, 0.27f, 0.17f), Id.Trim);
			else AddPart(Sphere, FVector(0.f, 0.f, HeadZ + 4.f), FVector(0.25f, 0.25f, 0.12f), Id.Class == ETPSocialClass::Sailor ? FColor(170, 40, 40) : Id.Pants);
			break;
		case ETPSocialClass::Vagrant:
			if (!bFemale && (IdentityIndex & 1)) AddPart(Sphere, FVector(0.f, 0.f, HeadZ + 3.f), FVector(0.26f, 0.26f, 0.13f), FColor(86, 76, 66));
			break;
		default:
			if (bFemale) AddPart(Sphere, FVector(0.f, 0.f, HeadZ + 3.f), FVector(0.27f, 0.27f, 0.17f), Id.Trim);
			else { AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 2.f), FVector(0.42f, 0.42f, 0.03f), Dark); AddPart(Cylinder, FVector(0.f, 0.f, HeadZ + 8.f), FVector(0.23f, 0.23f, 0.12f), Dark); }
			break;
		}
		break;
	}

	// старики и знать ходят медленнее, молодёжь быстрее
	WalkSpeed = FMath::Clamp(175.f - (Id.Age - 20) * 1.1f, 105.f, 175.f) * Rng.FRandRange(0.92f, 1.08f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

/* ---------------- реплики и взгляд ---------------- */

void ATPCitizen::Say(const FString& Text, float Seconds)
{
	BarkText = Text;
	BarkTime = Seconds;
}

float ATPCitizen::GetBarkAlpha() const
{
	return BarkTime > 0.f ? FMath::Clamp(BarkTime / 0.4f, 0.f, 1.f) : 0.f;
}

void ATPCitizen::LookAtFor(const FVector& Where, float Seconds)
{
	LookTarget = Where;
	LookTime = FMath::Max(LookTime, Seconds);
}

void ATPCitizen::FaceYaw(float Yaw, float Dt, float DegPerSec)
{
	const FRotator R = GetActorRotation();
	SetActorRotation(FRotator(0.f, FMath::FixedTurn(R.Yaw, Yaw, DegPerSec * Dt), 0.f));
}

void ATPCitizen::OnTalkedTo(AActor* Talker)
{
	if (!Talker || Mode == ETPCitizenMode::Down || Mode == ETPCitizenMode::Racing) return;
	if (Mode == ETPCitizenMode::Walk || Mode == ETPCitizenMode::Sidestep) StopWalking();
	LookAtFor(Talker->GetActorLocation(), 5.f);
	if (!bStationary) { Mode = ETPCitizenMode::Idle; ModeTime = 0.f; ModeDuration = 6.f; }
}

/* ---------------- прогулки ---------------- */

void ATPCitizen::EnterIdle(float MinTime, float MaxTime)
{
	Mode = bStationary ? ETPCitizenMode::Stand : ETPCitizenMode::Idle;
	ChatPartner = nullptr;
	if (bFleeing)
	{
		bFleeing = false;
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
	ModeTime = 0.f;
	ModeDuration = Rng.FRandRange(MinTime, MaxTime);
}

void ATPCitizen::StopWalking()
{
	if (AAIController* AI = Cast<AAIController>(GetController())) AI->StopMovement();
	GetCharacterMovement()->StopMovementImmediately();
}

void ATPCitizen::PickNewTarget()
{
	const FVector Here = GetActorLocation();
	FVector Best = Here;
	bool bFound = false;
	if (WalkPoints.Num() > 0)
	{
		// точка недалеко от дома, но не та, где стоим
		for (int32 Try = 0; Try < 14 && !bFound; ++Try)
		{
			const FVector& P = WalkPoints[Rng.RandRange(0, WalkPoints.Num() - 1)];
			const float FromHome = FVector::Dist2D(P, HomeLocation), FromHere = FVector::Dist2D(P, Here);
			if (FromHome < 4500.f && FromHere > 400.f && FromHere < 3500.f) { Best = P; bFound = true; }
		}
		if (!bFound) { Best = WalkPoints[Rng.RandRange(0, WalkPoints.Num() - 1)]; bFound = FVector::Dist2D(Best, Here) > 200.f; }
	}
	if (!bFound)
	{
		const float A = Rng.FRandRange(0.f, 2.f * PI), R = Rng.FRandRange(300.f, 1200.f);
		Best = HomeLocation + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
	}
	Target = Best;
	IssueMove();
}

void ATPCitizen::IssueMove()
{
	bManualMove = true;
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		const EPathFollowingRequestResult::Type R = AI->MoveToLocation(Target, 80.f, true, true, true);
		bManualMove = (R == EPathFollowingRequestResult::Failed);
	}
	Mode = ETPCitizenMode::Walk;
	ModeTime = 0.f;
	ModeDuration = 45.f;
	LastProgressPos = GetActorLocation();
	ProgressTimer = 0.f;
}

void ATPCitizen::UpdateWalk(float Dt)
{
	const FVector Here = GetActorLocation();
	FVector To = Target - Here; To.Z = 0.f;
	const float D = To.Size();
	if (D < 120.f)
	{
		StopWalking();
		EnterIdle(2.f, 7.f);
		if (Rng.FRand() < 0.4f) TryStartChat();
		return;
	}
	AAIController* AI = Cast<AAIController>(GetController());
	// навигация не построена или путь закончился раньше цели — идём напрямик
	if (!bManualMove && (!AI || (ModeTime > 0.6f && AI->GetMoveStatus() == EPathFollowingStatus::Idle))) bManualMove = true;
	if (bManualMove) AddMovementInput(To / D, 1.f);

	ProgressTimer += Dt;
	if (ProgressTimer > 2.5f)
	{
		if (FVector::Dist2D(Here, LastProgressPos) < 60.f)
		{
			// упёрлись: сначала пробуем напрямик, потом выбираем другую цель
			if (!bManualMove) { if (AI) AI->StopMovement(); bManualMove = true; }
			else { StopWalking(); EnterIdle(0.5f, 2.f); return; }
		}
		LastProgressPos = Here;
		ProgressTimer = 0.f;
	}
	if (ModeTime > ModeDuration) { StopWalking(); EnterIdle(1.f, 3.f); }
}

void ATPCitizen::TryStartChat()
{
	ATPCitizen* Best = nullptr;
	float BestD = 420.f;
	for (TActorIterator<ATPCitizen> It(GetWorld()); It; ++It)
	{
		ATPCitizen* O = *It;
		if (O == this || O->bStationary || O->ChatPartner.IsValid()) continue;
		if (O->Mode != ETPCitizenMode::Idle && O->Mode != ETPCitizenMode::Walk) continue;
		const float D = FVector::Dist(O->GetActorLocation(), GetActorLocation());
		if (D < BestD) { BestD = D; Best = O; }
	}
	if (!Best) return;
	const float Len = Rng.FRandRange(6.f, 12.f);
	Best->StopWalking();
	ChatPartner = Best; Best->ChatPartner = this;
	Mode = ETPCitizenMode::Talk; ModeTime = 0.f; ModeDuration = Len; ChatTimer = Rng.FRandRange(0.5f, 2.f);
	Best->Mode = ETPCitizenMode::Talk; Best->ModeTime = 0.f; Best->ModeDuration = Len; Best->ChatTimer = Rng.FRandRange(2.5f, 4.f);
}

/* ---------------- герой рядом ---------------- */

void ATPCitizen::CheckHero(float Dt)
{
	ACharacter* Hero = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!Hero) return;
	const FVector HL = Hero->GetActorLocation();
	FVector D = GetActorLocation() - HL;
	const float DZ = FMath::Abs(D.Z);
	D.Z = 0.f;
	const float Dist = D.Size();
	FVector HV = Hero->GetVelocity(); HV.Z = 0.f;
	const float HS = HV.Size();
	if (Dist > 1500.f) return;

	// оглядывается на того, кто носится по крышам или мчится мимо
	const UCharacterMovementComponent* HM = Hero->GetCharacterMovement();
	const bool bSpectacle = HS > 520.f || (HM && HM->MovementMode != MOVE_Walking && HM->MovementMode != MOVE_NavWalking);
	if (bSpectacle && Dist < 900.f && Mode != ETPCitizenMode::Walk) LookAtFor(HL, 1.2f);

	// расступается перед бегущим героем
	if (!bHeavy && HS > 380.f && Dist > 90.f && Dist < 380.f && DZ < 150.f &&
		(Mode == ETPCitizenMode::Walk || Mode == ETPCitizenMode::Idle || Mode == ETPCitizenMode::Talk))
	{
		const FVector Dir = HV / HS;
		const float Along = FVector::DotProduct(D, Dir);
		const FVector Lat = D - Dir * Along;
		if (Along > 0.f && Lat.Size() < 110.f)
		{
			SideDir = Lat.Size() > 5.f ? Lat.GetSafeNormal() : FVector(Dir.Y, -Dir.X, 0.f);
			ModeBeforeSidestep = Mode == ETPCitizenMode::Walk ? ETPCitizenMode::Walk : ETPCitizenMode::Idle;
			if (Mode == ETPCitizenMode::Walk) StopWalking();
			Mode = ETPCitizenMode::Sidestep; ModeTime = 0.f; ModeDuration = 0.7f;
			LookAtFor(HL, 1.5f);
		}
	}

	// столкновение
	if (ContactCooldown > 0.f) return;
	const float Reach = 38.f + GetCapsuleComponent()->GetScaledCapsuleRadius() + 14.f;
	if (Dist < Reach && DZ < 130.f)
	{
		const FVector N = Dist > 1.f ? D / Dist : Hero->GetActorForwardVector();
		const float Approach = FVector::DotProduct(Hero->GetVelocity(), N);
		if (Approach > 560.f && !bHeavy) Knockdown(N, Hero);
		else if (Approach > 260.f) Bump(N, Hero);
		else
		{
			ContactCooldown = 1.f;
			LookAtFor(HL, 1.f);
			if (Rng.FRand() < 0.25f) Say(TPCitizenText::Brush[Rng.RandRange(0, 5)]);
		}
	}
}

void ATPCitizen::Bump(const FVector& PushDir, ACharacter* Hero)
{
	ContactCooldown = 1.2f;
	LookAtFor(Hero->GetActorLocation(), 2.f);
	if (bHeavy) { Say(TPCitizenText::Heavy[Rng.RandRange(0, 3)]); return; }
	Say(TPCitizenText::BumpLines[Rng.RandRange(0, 5)]);
	if (Mode == ETPCitizenMode::Walk) StopWalking();
	LaunchCharacter(PushDir * 260.f + FVector(0.f, 0.f, 60.f), true, false);
	if (UCharacterMovementComponent* HM = Hero->GetCharacterMovement()) { HM->Velocity.X *= 0.7f; HM->Velocity.Y *= 0.7f; }
	Witness(0.25f);
	if (Mode != ETPCitizenMode::Talk) EnterIdle(1.5f, 3.f);
}

void ATPCitizen::Knockdown(const FVector& PushDir, ACharacter* Hero)
{
	ContactCooldown = 3.f;
	Say(TPCitizenText::Down[Rng.RandRange(0, 5)], 3.f);
	Witness(0.85f);
	StopWalking();
	if (ATPCitizen* P = ChatPartner.Get()) { P->ChatPartner = nullptr; P->EnterIdle(1.f, 3.f); }
	ChatPartner = nullptr;
	Mode = ETPCitizenMode::Down; ModeTime = 0.f; ModeDuration = Rng.FRandRange(2.2f, 3.4f);

	// герой спотыкается
	if (UCharacterMovementComponent* HM = Hero->GetCharacterMovement()) { HM->Velocity.X *= 0.45f; HM->Velocity.Y *= 0.45f; }

	// соседи оборачиваются
	for (TActorIterator<ATPCitizen> It(GetWorld()); It; ++It)
	{
		ATPCitizen* O = *It;
		if (O == this || O->IsDown() || O->IsRacing()) continue;
		if (FVector::Dist(O->GetActorLocation(), GetActorLocation()) > 700.f) continue;
		O->LookAtFor(GetActorLocation(), 1.6f);
		if (O->BarkTime <= 0.f && O->Rng.FRand() < 0.33f) O->Say(TPCitizenText::See[O->Rng.RandRange(0, 2)]);
	}

	const FVector Push = PushDir * 420.f + FVector(0.f, 0.f, 180.f);
	USkeletalMeshComponent* M = GetMesh();
	SetAccessoriesVisible(false);
	if (M->SkeletalMesh && M->GetPhysicsAsset())
	{
		// короткий рэгдолл
		bRagdoll = true;
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		M->SetCollisionProfileName(TEXT("Ragdoll"));
		M->SetAllBodiesSimulatePhysics(true);
		M->SetSimulatePhysics(true);
		M->WakeAllRigidBodies();
		M->SetAllPhysicsLinearVelocity(Push);
	}
	else
	{
		// без физического ассета: толчок и «падение» модели (см. Tick)
		bRagdoll = false;
		LaunchCharacter(Push, true, true);
	}
}

void ATPCitizen::Recover()
{
	USkeletalMeshComponent* M = GetMesh();
	if (bRagdoll)
	{
		const FVector Body = M->GetSocketLocation(TEXT("pelvis"));
		M->SetSimulatePhysics(false);
		M->SetAllBodiesSimulatePhysics(false);
		M->SetCollisionProfileName(TEXT("CharacterMesh"));
		M->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		M->SetRelativeLocationAndRotation(MeshRelLoc, MeshRelRot);
		// ставим капсулу туда, где лежит тело
		FVector NewLoc = GetActorLocation();
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(TPCitizenGetUp), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Body + FVector(0.f, 0.f, 80.f), Body - FVector(0.f, 0.f, 400.f), ECC_Visibility, Q))
			NewLoc = Hit.ImpactPoint + FVector(0.f, 0.f, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 4.f);
		SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		bRagdoll = false;
	}
	else M->SetRelativeRotation(MeshRelRot);
	SetAccessoriesVisible(true);
	if (Rng.FRand() < 0.6f) Say(TPCitizenText::GetUp[Rng.RandRange(0, 3)]);
	EnterIdle(1.5f, 3.f);
}

void ATPCitizen::Witness(float Amount) const
{
#if TP_HAS_COMBAT_DIRECTOR
	if (UWorld* W = GetWorld())
		if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
			Dir->Witness(GetActorLocation(), Amount);
#else
	(void)Amount;
#endif
}

/* ---------------- гонки ---------------- */

void ATPCitizen::SetRacing(bool bOn)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	UCapsuleComponent* Cap = GetCapsuleComponent();
	if (bOn)
	{
		if (Mode == ETPCitizenMode::Down) Recover();
		StopWalking();
		if (ATPCitizen* P = ChatPartner.Get()) { P->ChatPartner = nullptr; P->EnterIdle(1.f, 3.f); }
		ChatPartner = nullptr;
		if (bFleeing) { bFleeing = false; Move->MaxWalkSpeed = WalkSpeed; }
		Mode = ETPCitizenMode::Racing;
		Move->SetMovementMode(MOVE_Flying);
		Move->bOrientRotationToMovement = false;   // в полёте разворот «по движению» наклонял бы фигуру
		Move->MaxAcceleration = 6000.f;
		Move->BrakingDecelerationFlying = 6000.f;
		Cap->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Ignore);
		Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}
	else
	{
		Cap->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Cap->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		Cap->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Move->StopMovementImmediately();
		Move->bOrientRotationToMovement = true;
		Move->MaxAcceleration = 2048.f;
		Move->SetMovementMode(MOVE_Walking);
		EnterIdle(1.f, 2.f);
	}
}

void ATPCitizen::RaceStep(const FVector& RaceTarget, float Speed)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->MaxFlySpeed = Speed;
	const FVector To = RaceTarget - GetActorLocation();
	if (!To.IsNearlyZero()) AddMovementInput(To.GetSafeNormal(), 1.f);
	FVector Flat = To; Flat.Z = 0.f;
	if (Flat.SizeSquared() > 100.f) FaceYaw(Flat.Rotation().Yaw, GetWorld()->GetDeltaSeconds(), 720.f);
}

void ATPCitizen::ReturnHome()
{
	SetActorLocation(HomeLocation + FVector(0.f, 0.f, 20.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(FRotator(0.f, HomeYaw, 0.f));
}

/* ---------------- кадр ---------------- */

void ATPCitizen::Tick(float Dt)
{
	Super::Tick(Dt);
	if (!bAppearanceDone) ApplyAppearance();
	ModeTime += Dt;
	BarkTime -= Dt;
	ContactCooldown -= Dt;
	LookTime -= Dt;

	if (Mode == ETPCitizenMode::Racing) return;   // ведёт сюжет
	if (Mode == ETPCitizenMode::Down)
	{
		if (!bRagdoll)
		{
			// «падение» модели без физики: заваливается и лежит
			const float T = FMath::Clamp(ModeTime / 0.35f, 0.f, 1.f);
			GetMesh()->SetRelativeRotation(FRotator(0.f, MeshRelRot.Yaw, MeshRelRot.Roll + 82.f * T));
		}
		if (ModeTime > ModeDuration) Recover();
		return;
	}

	CheckHero(Dt);

	switch (Mode)
	{
	case ETPCitizenMode::Stand:
		if (LookTime <= 0.f) FaceYaw(HomeYaw, Dt, 120.f);
		// стоящие персонажи (бегуны Лиги) возвращаются на место, если их сдвинули
		if (FVector::Dist2D(GetActorLocation(), HomeLocation) > 150.f)
		{
			FVector To = HomeLocation - GetActorLocation(); To.Z = 0.f;
			AddMovementInput(To.GetSafeNormal(), 0.6f);
		}
		break;
	case ETPCitizenMode::Idle:
		if (ModeTime > ModeDuration) PickNewTarget();
		break;
	case ETPCitizenMode::Talk:
	{
		ATPCitizen* P = ChatPartner.Get();
		if (!P || P->Mode != ETPCitizenMode::Talk || ModeTime > ModeDuration)
		{
			ChatPartner = nullptr;
			EnterIdle(0.5f, 2.f);
			break;
		}
		if (LookTime <= 0.f)
		{
			FVector To = P->GetActorLocation() - GetActorLocation(); To.Z = 0.f;
			if (!To.IsNearlyZero()) FaceYaw(To.Rotation().Yaw, Dt, 240.f);
		}
		ChatTimer -= Dt;
		if (ChatTimer <= 0.f)
		{
			ChatTimer = Rng.FRandRange(3.f, 5.5f);
			const ACharacter* Hero = UGameplayStatics::GetPlayerCharacter(this, 0);
			const UGameInstance* GI = GetGameInstance();
			const UTPIdentitySubsystem* Ids = GI ? GI->GetSubsystem<UTPIdentitySubsystem>() : nullptr;
			if (Hero && Ids && FVector::Dist(Hero->GetActorLocation(), GetActorLocation()) < 1500.f)
				Say(Ids->GetRumor(IdentityIndex, Rng.RandRange(0, 9999)), 2.8f);
		}
		break;
	}
	case ETPCitizenMode::Sidestep:
		AddMovementInput(SideDir, 1.f);
		if (ModeTime > ModeDuration)
		{
			if (ModeBeforeSidestep == ETPCitizenMode::Walk) IssueMove();
			else EnterIdle(1.f, 3.f);
		}
		break;
	case ETPCitizenMode::Walk:
		UpdateWalk(Dt);
		break;
	default:
		break;
	}

	// оглянуться на героя или упавшего соседа
	if (LookTime > 0.f && Mode != ETPCitizenMode::Walk && Mode != ETPCitizenMode::Sidestep)
	{
		FVector To = LookTarget - GetActorLocation(); To.Z = 0.f;
		if (!To.IsNearlyZero()) FaceYaw(To.Rotation().Yaw, Dt, 300.f);
	}
}
