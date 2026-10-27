// Тени Парижа — герой: камера от третьего лица, ввод (клавиатура/мышь/геймпад), паркур и бой.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TPTypes.h"
#include "TPCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
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
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Камера") TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Камера") TObjectPtr<UCameraComponent> FollowCamera;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Паркур") TObjectPtr<UTPTraversalComponent> Traversal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Бой") TObjectPtr<UTPCombatComponent> Combat;

	/** события для систем интерфейса и сюжета */
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnInteractPressed;   // E
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnAlbumPressed;      // Tab
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnPausePressed;      // Esc
	UPROPERTY(BlueprintAssignable) FTPSimpleEvent OnHelpPressed;       // F1

	/** запасные пути к мешу и анимациям из шаблона Third Person (Add Feature or Content Pack) */
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> MeshCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> AnimClassCandidates;

	/** ввод заблокирован (диалог, меню) */
	UPROPERTY(BlueprintReadWrite) bool bInputLocked = false;

protected:
	void BuildInput();
	void OnMove(const FInputActionValue& V);
	void OnMoveDone(const FInputActionValue& V);
	void OnLook(const FInputActionValue& V);
	void OnJump(const FInputActionValue& V);
	void OnDrop(const FInputActionValue& V);
	void OnTurn(const FInputActionValue& V);
	void OnAttack(const FInputActionValue& V);
	void OnCounter(const FInputActionValue& V);
	void OnDodge(const FInputActionValue& V);
	void OnInteract(const FInputActionValue& V) { if (!bInputLocked) OnInteractPressed.Broadcast(); }
	void OnAlbum(const FInputActionValue& V) { OnAlbumPressed.Broadcast(); }
	void OnPause(const FInputActionValue& V) { OnPausePressed.Broadcast(); }
	void OnHelp(const FInputActionValue& V) { OnHelpPressed.Broadcast(); }
	void ApplyAppearance();
	FVector CameraRelative(const FVector2D& Axis) const;

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> Mapping;
	UPROPERTY(Transient) TMap<FName, TObjectPtr<UInputAction>> Actions;

	FVector2D MoveAxis = FVector2D::ZeroVector;
	FTPMoveInput Pending;
};
