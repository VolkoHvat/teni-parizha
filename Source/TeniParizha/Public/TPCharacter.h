// Тени Парижа — герой: камера от третьего лица, классический ввод UE 4.27 (оси и действия из Config/DefaultInput.ini), паркур и бой.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputCoreTypes.h"
#include "TPTypes.h"
#include "TPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UTPTraversalComponent;
class UTPCombatComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTPSimpleEvent);

UCLASS()
class TENIPARIZHA_API ATPCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATPCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Камера") USpringArmComponent* CameraBoom = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Камера") UCameraComponent* FollowCamera = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Паркур") UTPTraversalComponent* Traversal = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Бой") UTPCombatComponent* Combat = nullptr;

	/** события для систем интерфейса и сюжета */
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnInteractPressed;   // G
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnAlbumPressed;      // Tab
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnPausePressed;      // Esc
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnHelpPressed;       // F1

	/** запасные пути к мешу и анимациям манекена из пакета Third Person UE4 */
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> MeshCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> AnimClassCandidates;

	/** скорость поворота камеры со стика геймпада, градусов в секунду (до множителя InputYawScale) */
	UPROPERTY(EditAnywhere, Category = "Камера") float GamepadTurnRate = 60.f;
	UPROPERTY(EditAnywhere, Category = "Камера") float GamepadLookUpRate = 45.f;

	/** ввод заблокирован (диалог, меню) */
	UPROPERTY(BlueprintReadWrite) bool bInputLocked = false;

protected:
	// оси (BindAxis)
	void OnMoveForward(float Value) { MoveAxis.Y = Value; }
	void OnMoveRight(float Value) { MoveAxis.X = Value; }
	void OnTurn(float Value);
	void OnLookUp(float Value);
	void OnTurnRate(float Value);
	void OnLookUpRate(float Value);
	// действия (BindAction, IE_Pressed)
	void OnJump();
	void OnDrop();
	void OnTurnBack();
	void OnAttack();
	void OnCounter();
	void OnDodge();
	void OnInteract() { if (!bInputLocked) OnInteractPressed.Broadcast(); }
	void OnAlbum() { OnAlbumPressed.Broadcast(); }
	void OnPause() { OnPausePressed.Broadcast(); }
	void OnHelp() { OnHelpPressed.Broadcast(); }

	void ApplyAppearance();
	/** клавиши действия из настроек ввода (для опроса «зажато») */
	void CacheHeldKeys();
	bool IsActionHeld(const TArray<FKey>& Keys) const;
	FVector CameraRelative(const FVector2D& Axis) const;

	/** клавиши «Sprint» и «Crouch» из DefaultInput.ini: их состояние опрашивается каждый кадр */
	TArray<FKey> SprintKeys;
	TArray<FKey> CrouchKeys;

	FVector2D MoveAxis = FVector2D::ZeroVector;
	FTPMoveInput Pending;
};
