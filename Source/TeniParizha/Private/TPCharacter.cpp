#include "TPCharacter.h"
#include "TeniParizha.h"
#include "TPTraversalComponent.h"
#include "TPCombatComponent.h"
#include "TPCityGenerator.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "EnhancedPlayerInput.h"
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

	// Шаблон Third Person: в UE 5.4+ — SKM_Manny_Simple + ABP_Unarmed, раньше — SKM_Manny + ABP_Manny
	MeshCandidates = {
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny.SKM_Manny"),
		TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple") };
	AnimClassCandidates = {
		TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Manny.ABP_Manny_C"),
		TEXT("/Game/Characters/Mannequins/Animations/ABP_Quinn.ABP_Quinn_C") };
}

void ATPCharacter::ApplyAppearance()
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
		UE_LOG(LogTeni, Warning, TEXT("Нет меша персонажа: добавьте Third Person через Content Browser → Add → Add Feature or Content Pack"));
}

void ATPCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyAppearance();
	// старт на площади сгенерированного квартала
	for (TActorIterator<ATPCityGenerator> It(GetWorld()); It; ++It)
	{
		const FVector Start = It->GetPlayerStart() + FVector(0.f, 0.f, 100.f);
		Traversal->ResetTo(Start, 0.f);
		break;
	}
}

void ATPCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
			if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				if (!Mapping) BuildInput();
				Sub->ClearAllMappings();
				Sub->AddMappingContext(Mapping, 0);
			}
}

/* Действия и раскладка создаются в коде — не нужны ассеты .uasset */
void ATPCharacter::BuildInput()
{
	Mapping = NewObject<UInputMappingContext>(this, TEXT("TP_Mapping"));
	auto MakeAction = [this](FName Name, EInputActionValueType Type) {
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		Actions.Add(Name, A);
		return A;
	};
	auto Neg = [this]() { return NewObject<UInputModifierNegate>(this); };
	auto Swz = [this]() { UInputModifierSwizzleAxis* S = NewObject<UInputModifierSwizzleAxis>(this); S->Order = EInputAxisSwizzle::YXZ; return S; };
	auto DeadZone = [this]() { UInputModifierDeadZone* D = NewObject<UInputModifierDeadZone>(this); D->LowerThreshold = 0.2f; return D; };

	UInputAction* Move = MakeAction("Move", EInputActionValueType::Axis2D);
	Mapping->MapKey(Move, EKeys::D);
	Mapping->MapKey(Move, EKeys::A).Modifiers.Add(Neg());
	Mapping->MapKey(Move, EKeys::W).Modifiers.Add(Swz());
	{ FEnhancedActionKeyMapping& K = Mapping->MapKey(Move, EKeys::S); K.Modifiers.Add(Swz()); K.Modifiers.Add(Neg()); }
	Mapping->MapKey(Move, EKeys::Gamepad_Left2D).Modifiers.Add(DeadZone());

	UInputAction* Look = MakeAction("Look", EInputActionValueType::Axis2D);
	Mapping->MapKey(Look, EKeys::Mouse2D);
	{ FEnhancedActionKeyMapping& K = Mapping->MapKey(Look, EKeys::Gamepad_Right2D); K.Modifiers.Add(DeadZone()); }

	auto MapBool = [&](FName Name, std::initializer_list<FKey> Keys) {
		UInputAction* A = MakeAction(Name, EInputActionValueType::Boolean);
		for (const FKey& K : Keys) Mapping->MapKey(A, K);
		return A;
	};
	MapBool("Sprint", {EKeys::LeftShift, EKeys::Gamepad_RightTrigger, EKeys::RightMouseButton});
	MapBool("Jump", {EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom});
	MapBool("Crouch", {EKeys::LeftControl, EKeys::C, EKeys::Gamepad_FaceButton_Right});
	MapBool("Drop", {EKeys::Q, EKeys::Gamepad_LeftShoulder});
	MapBool("Turn", {EKeys::R, EKeys::Gamepad_RightThumbstick});
	MapBool("Attack", {EKeys::LeftMouseButton, EKeys::F, EKeys::Gamepad_FaceButton_Left});
	MapBool("Counter", {EKeys::E, EKeys::Gamepad_FaceButton_Top});
	MapBool("Dodge", {EKeys::LeftAlt, EKeys::Gamepad_RightShoulder});
	MapBool("Interact", {EKeys::G, EKeys::Gamepad_DPad_Up});
	MapBool("Album", {EKeys::Tab, EKeys::Gamepad_Special_Left});
	MapBool("Pause", {EKeys::Escape, EKeys::Gamepad_Special_Right});
	MapBool("Help", {EKeys::F1});
}

void ATPCharacter::SetupPlayerInputComponent(UInputComponent* PIC)
{
	Super::SetupPlayerInputComponent(PIC);
	if (!Mapping) BuildInput();
	UEnhancedInputComponent* EI = Cast<UEnhancedInputComponent>(PIC);
	if (!EI) { UE_LOG(LogTeni, Error, TEXT("Нужен EnhancedInputComponent (Config/DefaultEngine.ini)")); return; }
	EI->BindAction(Actions["Move"], ETriggerEvent::Triggered, this, &ATPCharacter::OnMove);
	EI->BindAction(Actions["Move"], ETriggerEvent::Completed, this, &ATPCharacter::OnMoveDone);
	EI->BindAction(Actions["Look"], ETriggerEvent::Triggered, this, &ATPCharacter::OnLook);
	EI->BindAction(Actions["Jump"], ETriggerEvent::Started, this, &ATPCharacter::OnJump);
	EI->BindAction(Actions["Drop"], ETriggerEvent::Started, this, &ATPCharacter::OnDrop);
	EI->BindAction(Actions["Turn"], ETriggerEvent::Started, this, &ATPCharacter::OnTurn);
	EI->BindAction(Actions["Attack"], ETriggerEvent::Started, this, &ATPCharacter::OnAttack);
	EI->BindAction(Actions["Counter"], ETriggerEvent::Started, this, &ATPCharacter::OnCounter);
	EI->BindAction(Actions["Dodge"], ETriggerEvent::Started, this, &ATPCharacter::OnDodge);
	EI->BindAction(Actions["Interact"], ETriggerEvent::Started, this, &ATPCharacter::OnInteract);
	EI->BindAction(Actions["Album"], ETriggerEvent::Started, this, &ATPCharacter::OnAlbum);
	EI->BindAction(Actions["Pause"], ETriggerEvent::Started, this, &ATPCharacter::OnPause);
	EI->BindAction(Actions["Help"], ETriggerEvent::Started, this, &ATPCharacter::OnHelp);
}

void ATPCharacter::OnMove(const FInputActionValue& V) { MoveAxis = V.Get<FVector2D>(); }
void ATPCharacter::OnMoveDone(const FInputActionValue& V) { MoveAxis = FVector2D::ZeroVector; }
void ATPCharacter::OnLook(const FInputActionValue& V)
{
	const FVector2D L = V.Get<FVector2D>();
	AddControllerYawInput(L.X);
	AddControllerPitchInput(-L.Y);
}
void ATPCharacter::OnJump(const FInputActionValue& V) { if (!bInputLocked) Pending.bJump = true; }
void ATPCharacter::OnDrop(const FInputActionValue& V) { if (!bInputLocked) Pending.bDrop = true; }
void ATPCharacter::OnTurn(const FInputActionValue& V) { if (!bInputLocked) Pending.bTurn = true; }
void ATPCharacter::OnAttack(const FInputActionValue& V) { if (!bInputLocked) Combat->RequestAction(ETPCombatAction::Attack, GetActorForwardVector()); }
void ATPCharacter::OnCounter(const FInputActionValue& V) { if (!bInputLocked) Combat->RequestAction(ETPCombatAction::Counter, GetActorForwardVector()); }
void ATPCharacter::OnDodge(const FInputActionValue& V)
{
	if (bInputLocked) return;
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
		auto Held = [this](FName N) {
			const APlayerController* PC = Cast<APlayerController>(GetController());
			if (!PC || !PC->GetLocalPlayer()) return false;
			const UEnhancedInputLocalPlayerSubsystem* S = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());
			return S && S->GetPlayerInput() && S->GetPlayerInput()->GetActionValue(Actions[N]).Get<bool>();
		};
		In.bSprint = Held("Sprint");
		In.bCrouch = Held("Crouch");
	}
	else In = FTPMoveInput();
	Traversal->SetInput(In);
	// камера чуть дальше на свободном бегу
	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, In.bSprint ? 430.f : 380.f, Dt, 3.f);
}
