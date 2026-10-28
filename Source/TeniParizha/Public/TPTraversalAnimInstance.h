// Тени Парижа — базовый класс анимации героя: переменные паркура для AnimBP (родитель для ABP героя).
// Владелец модуля: агент «паркур».
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TPTypes.h"
#include "TPTraversalAnimInstance.generated.h"

class ACharacter;
class UTPTraversalComponent;

/**
 * Каждый кадр забирает состояние из UTPTraversalComponent и CharacterMovement.
 * В AnimBP (Class Settings → Parent Class = TPTraversalAnimInstance) переменные доступны только для чтения:
 * машина состояний — по State/bHanging/bClimbing/bInAir, IK рук — Two Bone IK к LeftHandIK/RightHandIK (мир) с весом HandIKAlpha.
 */
UCLASS(Blueprintable)
class TENIPARIZHA_API UTPTraversalAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** горизонтальная скорость, см/с */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") float Speed = 0.f;
	/** вертикальная скорость, см/с */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") float VerticalSpeed = 0.f;
	/** в полёте (прыжок, падение) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") bool bInAir = false;
	/** состояние паркура */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") ETPTraversal State = ETPTraversal::Ground;
	/** вис на уступе (в т. ч. перескок и спуск в вис) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") bool bHanging = false;
	/** лазание по стене или забег на стену */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") bool bClimbing = false;
	/** крадётся */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") bool bCrouching = false;
	/** свободный бег (спринт) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") bool bFreeRunning = false;
	/** скорость проигрывания цикла лазания (0 — висит неподвижно) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") float ClimbSpeed = 0.f;
	/** -1 влево, 0 на месте, +1 вправо */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") float ShimmyDir = 0.f;
	/** мировые точки ладоней на кромке/стене */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур|IK") FVector LeftHandIK = FVector::ZeroVector;
	UPROPERTY(BlueprintReadOnly, Category = "Паркур|IK") FVector RightHandIK = FVector::ZeroVector;
	/** вес IK рук 0..1 (сглажен) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур|IK") float HandIKAlpha = 0.f;
	/** наклон корпуса в повороте на бегу, градусы (+ вправо) */
	UPROPERTY(BlueprintReadOnly, Category = "Паркур") float LeanAngle = 0.f;

protected:
	UPROPERTY(Transient) ACharacter* OwnerCharacter = nullptr;
	UPROPERTY(Transient) UTPTraversalComponent* Traversal = nullptr;

	void ResolveOwner();
};
