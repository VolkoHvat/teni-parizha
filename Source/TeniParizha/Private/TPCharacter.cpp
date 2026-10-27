#include "TPCharacter.h"
#include "TeniParizha.h"
#include "TPTraversalComponent.h"
#include "TPCombatComponent.h"
#include "TPCityGenerator.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"
#include "Engine/World.h"
#include "EngineUtils.h"

ATPCharacter::ATPCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(38.f, 92.f);

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 720.f, 0.f);
	Move->MaxWalkSpeed = 460.f;
	Move->JumpZVelocity = 620.f;
	Move->AirControl = 0.25f;
	Move->GravityScale = 2.0f;
	Move->MaxStepHeight = 45.f;
	Move->SetWalkableFloorAngle(50.f);
	Move->BrakingDecelerationWalking = 1600.f;
	Move->NavAgentProps.bCanCrouch = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->ProbeSize = 14.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	FollowCamera->SetFieldOfView(80.f);

	Traversal = CreateDefaultSubobject<UTPTraversalComponent>(TEXT("Traversal"));
	Combat = CreateDefaultSubobject<UTPCombatComponent>(TEXT("Combat"));

	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -92.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

	// Пакет Third Person UE4 (Add New → Add Feature or Content Pack → Third Person), запасной — Animation Starter Pack
	MeshCandidates = {
		TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"),
		TEXT("/Game/AnimStarterPack/UE4_Mannequin/Mesh/SK_Mannequin.SK_Mannequin") };
	AnimClassCandidates = {
		TEXT("/Game/Mannequin/Animations/ThirdPerson_AnimBP.ThirdPerson_AnimBP_C") };
}

void ATPCharacter::ApplyAppearance()
{
	USkeletalMeshComponent* M = GetMesh();
	if (!M->SkeletalMesh)
	{
		for (const FString& P : MeshCandidates)
			if (USkeletalMesh* SM = LoadObject<USkeletalMesh>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetSkeletalMesh(SM); break; }
	}
	if (!M->GetAnimInstance())
	{
		for (const FString& P : AnimClassCandidates)
			if (UClass* C = LoadClass<UAnimInstance>(nullptr, *P, nullptr, LOAD_NoWarn | LOAD_Quiet)) { M->SetAnimInstanceClass(C); break; }
	}
	if (!M->SkeletalMesh)
		UE_LOG(LogTeni, Warning, TEXT("Нет меша персонажа: добавьте Third Person через Content Browser → Add → Add Feature or Content Pack → Third Person"));
}

void ATPCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyAppearance();
	// старт на площади сгенерированного квартала
	for (TActorIterator<ATPCityGenerator> It(GetWorld()); It; ++It)
	{
		const FVector Start = It->GetPlayerStart() + FVector(0.f, 0.f, 100.f);
		if (Traversal) Traversal->ResetTo(Start, 0.f);
		break;
	}
}

/* Классический ввод UE 4.27: оси и действия описаны в Config/DefaultInput.ini — ассеты не нужны */
void ATPCharacter::SetupPlayerInputComponent(UInputComponent* PIC)
{
	Super::SetupPlayerInputComponent(PIC);
	check(PIC);
	PIC->BindAxis(TEXT("MoveForward"), this, &ATPCharacter::OnMoveForward);
	PIC->BindAxis(TEXT("MoveRight"), this, &ATPCharacter::OnMoveRight);
	PIC->BindAxis(TEXT("Turn"), this, &ATPCharacter::OnTurn);
	PIC->BindAxis(TEXT("LookUp"), this, &ATPCharacter::OnLookUp);
	PIC->BindAxis(TEXT("TurnRate"), this, &ATPCharacter::OnTurnRate);
	PIC->BindAxis(TEXT("LookUpRate"), this, &ATPCharacter::OnLookUpRate);

	PIC->BindAction(TEXT("Jump"), IE_Pressed, this, &ATPCharacter::OnJump);
	PIC->BindAction(TEXT("Drop"), IE_Pressed, this, &ATPCharacter::OnDrop);
	PIC->BindAction(TEXT("TurnBack"), IE_Pressed, this, &ATPCharacter::OnTurnBack);
	PIC->BindAction(TEXT("Attack"), IE_Pressed, this, &ATPCharacter::OnAttack);
	PIC->BindAction(TEXT("Counter"), IE_Pressed, this, &ATPCharacter::OnCounter);
	PIC->BindAction(TEXT("Dodge"), IE_Pressed, this, &ATPCharacter::OnDodge);
	PIC->BindAction(TEXT("Interact"), IE_Pressed, this, &ATPCharacter::OnInteract);
	PIC->BindAction(TEXT("Album"), IE_Pressed, this, &ATPCharacter::OnAlbum);
	// на паузе действия не срабатывают: меню паузы, альбом и справку закрывает сам интерфейс (опрос клавиш в ATPHUD)
	PIC->BindAction(TEXT("Pause"), IE_Pressed, this, &ATPCharacter::OnPause);
	PIC->BindAction(TEXT("Help"), IE_Pressed, this, &ATPCharacter::OnHelp);
	CacheHeldKeys();
}

void ATPCharacter::CacheHeldKeys()
{
	SprintKeys.Reset();
	CrouchKeys.Reset();
	const UInputSettings* IS = UInputSettings::GetInputSettings();
	if (!IS) return;
	TArray<FInputActionKeyMapping> Maps;
	IS->GetActionMappingByName(TEXT("Sprint"), Maps);
	for (const FInputActionKeyMapping& KM : Maps) SprintKeys.AddUnique(KM.Key);
	Maps.Reset();
	IS->GetActionMappingByName(TEXT("Crouch"), Maps);
	for (const FInputActionKeyMapping& KM : Maps) CrouchKeys.AddUnique(KM.Key);
	if (SprintKeys.Num() == 0 || CrouchKeys.Num() == 0)
		UE_LOG(LogTeni, Warning, TEXT("В Config/DefaultInput.ini нет действий Sprint/Crouch — бег и присед недоступны"));
}

bool ATPCharacter::IsActionHeld(const TArray<FKey>& Keys) const
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return false;
	for (const FKey& K : Keys)
		if (PC->IsInputKeyDown(K)) return true;
	return false;
}

void ATPCharacter::OnTurn(float Value) { if (Value != 0.f) AddControllerYawInput(Value); }
void ATPCharacter::OnLookUp(float Value) { if (Value != 0.f) AddControllerPitchInput(Value); }
void ATPCharacter::OnTurnRate(float Value)
{
	if (Value != 0.f && GetWorld()) AddControllerYawInput(Value * GamepadTurnRate * GetWorld()->GetDeltaSeconds());
}
void ATPCharacter::OnLookUpRate(float Value)
{
	if (Value != 0.f && GetWorld()) AddControllerPitchInput(Value * GamepadLookUpRate * GetWorld()->GetDeltaSeconds());
}
void ATPCharacter::OnJump() { if (!bInputLocked) Pending.bJump = true; }
void ATPCharacter::OnDrop() { if (!bInputLocked) Pending.bDrop = true; }
void ATPCharacter::OnTurnBack() { if (!bInputLocked) Pending.bTurn = true; }
void ATPCharacter::OnAttack() { if (!bInputLocked && Combat) Combat->RequestAction(ETPCombatAction::Attack, GetActorForwardVector()); }
void ATPCharacter::OnCounter() { if (!bInputLocked && Combat) Combat->RequestAction(ETPCombatAction::Counter, GetActorForwardVector()); }
void ATPCharacter::OnDodge()
{
	if (bInputLocked || !Combat) return;
	const FVector D = CameraRelative(MoveAxis);
	Combat->RequestAction(ETPCombatAction::Dodge, D.IsNearlyZero() ? -GetActorForwardVector() : D);
}

FVector ATPCharacter::CameraRelative(const FVector2D& Axis) const
{
	const FRotator Yaw(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Fwd = FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Right = FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y);
	FVector M = Fwd * Axis.Y + Right * Axis.X;
	return M.Size() > 1.f ? M.GetSafeNormal() : M;
}

void ATPCharacter::Tick(float Dt)
{
	Super::Tick(Dt);
	const bool bPC = Controller != nullptr && !bInputLocked;
	FTPMoveInput In = Pending;
	Pending = FTPMoveInput();
	if (bPC)
	{
		In.Move = CameraRelative(MoveAxis);
		In.bSprint = IsActionHeld(SprintKeys);
		In.bCrouch = IsActionHeld(CrouchKeys);
	}
	else In = FTPMoveInput();
	if (Traversal) Traversal->SetInput(In);
	// камера чуть дальше на свободном бегу
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, In.bSprint ? 430.f : 380.f, Dt, 3.f);
}
