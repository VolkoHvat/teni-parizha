// Эталон: правильный код UE 4.27 (широкий охват API) — стенд должен пропускать его без ошибок.
#include "TPGoodActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/InputComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "CollisionQueryParams.h"
#include "WorldCollision.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "ProceduralMeshComponent.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogTPGood, Log, All);

static TAutoConsoleVariable<int32> CVarTPGood(TEXT("tp.Good"), 1, TEXT("Эталонная переменная"), ECVF_Scalability);

UTPGoodComponent::UTPGoodComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UTPGoodComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ATPGoodActor* Owner = GetOwner<ATPGoodActor>())
	{
		OnDamaged.AddDynamic(Owner, &ATPGoodActor::HandleDamaged);
	}
}

void UTPGoodComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Health = FMath::Clamp(Health + DeltaTime, 0.f, 100.f);
}

void UTPGoodComponent::Hit(float Amount, AActor* InInstigator)
{
	Health -= Amount;
	OnDamaged.Broadcast(Amount, InInstigator);
}

ATPGoodActor::ATPGoodActor()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(34.f, 88.f);
	GetCharacterMovement()->MaxWalkSpeed = 450.f;
	GetCharacterMovement()->SetWalkableFloorAngle(50.f);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);
	bUseControllerRotationYaw = false;
	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);
	Combat = CreateDefaultSubobject<UTPGoodComponent>(TEXT("Combat"));
	Lamp = CreateDefaultSubobject<UPointLightComponent>(TEXT("Lamp"));
	Lamp->SetupAttachment(GetRootComponent());
	Lamp->SetIntensity(800.f);
	Lamp->SetAttenuationRadius(600.f);
	Lamp->SetCastShadows(false);
	Lamp->SetIntensityUnits(ELightUnits::Candelas);
	Lamp->SetLightColor(FLinearColor(1.f, 0.7f, 0.4f));
	USpringArmComponent* Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(GetRootComponent());
	Arm->TargetArmLength = 350.f;
	Arm->bUsePawnControlRotation = true;
	UCameraComponent* Cam = CreateDefaultSubobject<UCameraComponent>(TEXT("Cam"));
	Cam->SetupAttachment(Arm, USpringArmComponent::SocketName);
	if (USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin")))
	{
		GetMesh()->SetSkeletalMesh(SM);
		GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	}
}

void ATPGoodActor::BeginPlay()
{
	Super::BeginPlay();
	UWorld* W = GetWorld();
	if (!W) return;
	W->GetTimerManager().SetTimer(Timer, this, &ATPGoodActor::OnTimer, 0.5f, true);
	FTimerHandle Once;
	W->GetTimerManager().SetTimer(Once, FTimerDelegate::CreateUObject(this, &ATPGoodActor::OnTimer), 1.f, false);
	W->GetTimerManager().SetTimer(Once, [this]() { Accum = 0.f; }, 2.f, false);

	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
	{
		Mid = UMaterialInstanceDynamic::Create(Base, this);
		Mid->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.8f, 0.6f, 0.4f));
		Mid->SetScalarParameterValue(TEXT("Roughness"), 0.7f);
		Body->SetMaterial(0, Mid);
	}
	UTexture2D* Tex = UTexture2D::CreateTransient(64, 64, PF_B8G8R8A8);
	if (Tex && Tex->PlatformData && Tex->PlatformData->Mips.Num() > 0)
	{
		FTexture2DMipMap& Mip = Tex->PlatformData->Mips[0];
		uint8* Data = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		FMemory::Memset(Data, 128, 64 * 64 * 4);
		Mip.BulkData.Unlock();
		Tex->SRGB = true;
		Tex->CompressionSettings = TC_Default;
		Tex->Filter = TF_Bilinear;
		Tex->UpdateResource();
		Textures.Add(Tex);
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TPGood), false, this);
	Params.AddIgnoredActor(this);
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector::UpVector * 500.f;
	if (W->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		AActor* HitActor = Hit.GetActor();
		UE_LOG(LogTPGood, Log, TEXT("Пол: %s на %.1f см"), *GetNameSafe(HitActor), Hit.Distance);
		DrawDebugLine(W, Start, Hit.ImpactPoint, FColor::Green, false, 2.f);
	}
	TArray<FOverlapResult> Overlaps;
	W->OverlapMultiByChannel(Overlaps, Start, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(300.f), Params);
	for (const FOverlapResult& O : Overlaps)
	{
		if (AActor* A = O.GetActor()) Named.Add(A->GetFName(), A);
	}
	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_WorldStatic);
	W->SweepSingleByObjectType(Hit, Start, End, FQuat::Identity, Obj, FCollisionShape::MakeCapsule(30.f, 80.f), Params);

	for (TActorIterator<ACharacter> It(W); It; ++It)
	{
		ACharacter* C = *It;
		if (C != this && FVector::DistSquared(C->GetActorLocation(), Start) < FMath::Square(1000.f)) LastTarget = C;
	}
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, ACharacter::StaticClass(), Found);
	Found.Sort([](const AActor& A, const AActor& B) { return A.GetActorLocation().Z < B.GetActorLocation().Z; });
	Found.RemoveAll([](const AActor* A) { return A == nullptr; });

	FActorSpawnParameters SP;
	SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SP.Owner = this;
	AActor* Spawned = W->SpawnActor<AActor>(AActor::StaticClass(), GetActorTransform(), SP);
	if (Spawned) Spawned->SetLifeSpan(5.f);

	UHierarchicalInstancedStaticMeshComponent* Hism = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, TEXT("Lamps"));
	Hism->SetupAttachment(GetRootComponent());
	Hism->RegisterComponent();
	Hism->SetCullDistances(6000, 8000);
	Hism->SetCastShadow(false);
	Hism->AddInstance(FTransform(FRotator::ZeroRotator, FVector(100.f, 0.f, 0.f)));
	Hism->AddInstanceWorldSpace(FTransform(FVector(0.f, 100.f, 0.f)));

	UProceduralMeshComponent* Proc = NewObject<UProceduralMeshComponent>(this);
	TArray<FVector> V = { FVector(0, 0, 0), FVector(100, 0, 0), FVector(0, 100, 0) };
	TArray<int32> T = { 0, 1, 2 };
	Proc->CreateMeshSection_LinearColor(0, V, T, TArray<FVector>(), TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);

	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		FVector2D Screen;
		PC->ProjectWorldLocationToScreen(Start, Screen);
		if (PC->WasInputKeyJustPressed(EKeys::E)) Accum += 1.f;
	}
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, FString::Printf(TEXT("Здоровье %.0f"), Combat->Health));
	if (UAnimInstance* Anim = GetMesh()->GetAnimInstance())
	{
		if (UAnimMontage* M = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Anim/AM_Roll.AM_Roll"))) Anim->Montage_Play(M, 1.2f);
	}
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	const float Ang = FMath::RadiansToDegrees(FMath::Atan2(Start.Y, Start.X));
	const FRotator R = FRotationMatrix::MakeFromX(FVector::ForwardVector).Rotator();
	const FQuat Q = FQuat(FVector::UpVector, FMath::DegreesToRadians(Ang)) * R.Quaternion();
	SetActorRotation(Q);
	TMap<FName, int32> Counts;
	Counts.FindOrAdd(TEXT("Стражи"))++;
	for (const TPair<FName, int32>& P : Counts) { Accum += P.Value; }
	TSet<int32> Ids; Ids.Add(3);
	FString Name = FString::Printf(TEXT("%s_%d"), *GetName(), Ids.Num());
	FText Label = FText::FromString(Name);
	if (Label.IsEmpty() || Name.StartsWith(TEXT("TP"))) Accum += 1.f;
	const int32 Level = CVarTPGood.GetValueOnGameThread();
	Accum += Level;
	Info.Speed = GetVelocity().Size2D();
	const AActor* Self = this;
	TMap<AActor*, int32> Score; Score.Add(this, 1);
	if (Found.Contains(Self) && Score.Find(Self)) Accum += 1.f;
	GetCapsuleComponent()->CanCharacterStepUpOn = ECB_No;
	const FString SavePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TP"), TEXT("log.txt"));
	FFileHelper::SaveStringToFile(Name, *SavePath, FFileHelper::EEncodingOptions::ForceUTF8);
	FSimpleDelegate Simple = FSimpleDelegate::CreateUObject(this, &ATPGoodActor::OnTimer);
	Simple.ExecuteIfBound();
}

void ATPGoodActor::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ATPGoodActor::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ATPGoodActor::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ATPGoodActor::JumpPressed);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
}

void ATPGoodActor::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
	}
}

void ATPGoodActor::MoveRight(float Value)
{
	if (Value != 0.f) AddMovementInput(GetActorRightVector(), Value);
}

void ATPGoodActor::JumpPressed() { Jump(); }

void ATPGoodActor::OnTimer()
{
	Accum += 0.5f;
	if (LastTarget.IsValid() && GetCharacterMovement()->IsFalling()) LastTarget->GetActorLocation();
}

void ATPGoodActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const FVector Vel = GetCharacterMovement()->Velocity;
	const float Lean = FMath::FInterpTo(0.f, FMath::Clamp(Vel.Y * 0.01f, -15.f, 15.f), DeltaSeconds, 6.f);
	State = Lean > 1.f ? ETPGoodState::Run : ETPGoodState::Idle;
}

void ATPGoodActor::HandleDamaged(float Amount, AActor* InInstigator)
{
	if (AAIController* AI = Cast<AAIController>(GetController()))
	{
		AI->MoveToLocation(InInstigator ? InInstigator->GetActorLocation() : GetActorLocation(), 50.f);
	}
	if (UNavigationSystemV1* Nav = UNavigationSystemV1::GetCurrent<UNavigationSystemV1>(GetWorld()))
	{
		FNavLocation Out;
		Nav->ProjectPointToNavigation(GetActorLocation(), Out);
	}
}
