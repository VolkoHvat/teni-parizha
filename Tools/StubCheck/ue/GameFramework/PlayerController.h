// Макет GameFramework/PlayerController.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "InputCoreTypes.h"
class AHUD; class APlayerCameraManager; class UPlayerInput; class ULocalPlayer; class UPlayer; class ASpectatorPawn; class UCameraShakeBase; class UForceFeedbackEffect;
enum EViewTargetBlendFunction { VTBlend_Linear, VTBlend_Cubic, VTBlend_EaseIn, VTBlend_EaseOut, VTBlend_EaseInOut, VTBlend_PreBlended, VTBlend_MAX };
namespace EMouseCursor { enum Type { None, Default, TextEditBeam, ResizeLeftRight, ResizeUpDown, ResizeSouthEast, ResizeSouthWest, CardinalCross, Crosshairs, Hand, GrabHand, GrabHandClosed, SlashedCircle, EyeDropper, Custom, TotalCursorCount }; }
enum class ECameraShakePlaySpace : uint8 { CameraLocal, World, UserDefined };
DECLARE_DELEGATE_RetVal(bool, FCanUnpause);
struct FInputModeDataBase { virtual ~FInputModeDataBase(); };
struct FInputModeGameOnly : public FInputModeDataBase { FInputModeGameOnly& SetConsumeCaptureMouseDown(bool InConsumeCaptureMouseDown); };
struct FInputModeUIOnly : public FInputModeDataBase { FInputModeUIOnly& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode); };
struct FInputModeGameAndUI : public FInputModeDataBase { FInputModeGameAndUI& SetLockMouseToViewportBehavior(EMouseLockMode InMouseLockMode); FInputModeGameAndUI& SetHideCursorDuringCapture(bool InHideCursorDuringCapture); };
class APlayerController : public AController
{
public:
	typedef AController Super; typedef APlayerController ThisClass; static UClass* StaticClass();
	APlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UPlayer* Player; APawn* AcknowledgedPawn; AHUD* MyHUD;
	APlayerCameraManager* PlayerCameraManager; TSubclassOf<APlayerCameraManager> PlayerCameraManagerClass;
	UPlayerInput* PlayerInput;
	uint32 bShowMouseCursor : 1; uint32 bEnableClickEvents : 1; uint32 bEnableTouchEvents : 1; uint32 bEnableMouseOverEvents : 1; uint32 bEnableTouchOverEvents : 1;
	uint32 bForceFeedbackEnabled : 1; uint32 bAutoManageActiveCameraTarget : 1; uint32 bCinematicMode : 1;
	TEnumAsByte<EMouseCursor::Type> DefaultMouseCursor; TEnumAsByte<EMouseCursor::Type> CurrentMouseCursor;
	float InputYawScale; float InputPitchScale; float InputRollScale; FRotator RotationInput; float HitResultTraceDistance;
	AHUD* GetHUD() const;
	template<class T> T* GetHUD() const { static_assert(sizeof(T) > 0, "GetHUD<T>: неполный тип"); return nullptr; }
	ULocalPlayer* GetLocalPlayer() const;
	virtual void GetPlayerViewPoint(FVector& Location, FRotator& Rotation) const override;
	virtual void SetViewTargetWithBlend(class AActor* NewViewTarget, float BlendTime = 0, enum EViewTargetBlendFunction BlendFunc = VTBlend_Linear, float BlendExp = 0, bool bLockOutgoing = false);
	virtual void SetViewTarget(class AActor* NewViewTarget);
	virtual AActor* GetViewTarget() const override;
	void GetViewportSize(int32& SizeX, int32& SizeY) const;
	bool ProjectWorldLocationToScreen(FVector WorldLocation, FVector2D& ScreenLocation, bool bPlayerViewportRelative = false) const;
	bool DeprojectScreenPositionToWorld(float ScreenX, float ScreenY, FVector& WorldLocation, FVector& WorldDirection) const;
	bool DeprojectMousePositionToWorld(FVector& WorldLocation, FVector& WorldDirection) const;
	bool GetMousePosition(float& LocationX, float& LocationY) const;
	void GetInputMouseDelta(float& DeltaX, float& DeltaY) const;
	bool IsInputKeyDown(FKey Key) const; bool WasInputKeyJustPressed(FKey Key) const; bool WasInputKeyJustReleased(FKey Key) const;
	float GetInputAnalogKeyState(FKey Key) const; FVector GetInputVectorKeyState(FKey Key) const; float GetInputKeyTimeDown(FKey Key) const;
	bool GetHitResultUnderCursor(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;
	void SetInputMode(const FInputModeDataBase& InData);
	virtual bool SetPause(bool bPause, FCanUnpause CanUnpauseDelegate = FCanUnpause());
	bool IsPaused() const;
	virtual FString ConsoleCommand(const FString& Command, bool bWriteToLog = true);
	virtual void AddYawInput(float Val); virtual void AddPitchInput(float Val); virtual void AddRollInput(float Val);
	virtual bool IsLookInputIgnored() const override; virtual bool IsMoveInputIgnored() const override;
	virtual void PlayerTick(float DeltaTime);
	virtual void FlushPressedKeys();
	void ClientStartCameraShake(TSubclassOf<UCameraShakeBase> Shake, float Scale = 1.f, ECameraShakePlaySpace PlaySpace = ECameraShakePlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	void ClientStopCameraShake(TSubclassOf<UCameraShakeBase> Shake, bool bImmediately = true);
	void ClientPlayForceFeedback(class UForceFeedbackEffect* ForceFeedbackEffect, FName Tag = NAME_None, bool bLooping = false, bool bIgnoreTimeDilation = false, bool bPlayWhilePaused = false);
	void SetAudioListenerOverride(USceneComponent* AttachToComponent, FVector Location, FRotator Rotation); void ClearAudioListenerOverride();
	virtual void ClientMessage(const FString& S, FName Type = NAME_None, float MsgLifeTime = 0.f);
	ASpectatorPawn* GetSpectatorPawn() const; APawn* GetPawnOrSpectator() const;
	virtual void GetAudioListenerPosition(FVector& OutLocation, FVector& OutFrontDir, FVector& OutRightDir) const;
	FVector GetFocalLocation() const;
	virtual void SetCinematicMode(bool bInCinematicMode, bool bHidePlayer, bool bAffectsHUD, bool bAffectsMovement, bool bAffectsTurning);
	virtual void EnableCheats();
protected:
	virtual void SetupInputComponent();
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void ProcessPlayerInput(const float DeltaTime, const bool bGamePaused);
};
