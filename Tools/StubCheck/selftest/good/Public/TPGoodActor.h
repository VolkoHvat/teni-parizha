// Эталон: правильный код UE 4.27 — стенд должен пропускать его без ошибок.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/ActorComponent.h"
#include "TPGoodActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTPGoodDamaged, float, Amount, AActor*, Instigator);

UENUM(BlueprintType)
enum class ETPGoodState : uint8
{
	Idle UMETA(DisplayName = "Покой"),
	Run,
	Climb
};

USTRUCT(BlueprintType)
struct FTPGoodInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Speed = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector Target = FVector::ZeroVector;
	UPROPERTY() UTexture2D* Icon = nullptr;
	UPROPERTY() TArray<AActor*> Seen;
};

UCLASS()
class TENIPARIZHA_API UTPGoodComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTPGoodComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintCallable) void Hit(float Amount, AActor* InInstigator);
	UPROPERTY(BlueprintAssignable) FTPGoodDamaged OnDamaged;
	UPROPERTY(EditAnywhere) float Health = 100.f;
protected:
	virtual void BeginPlay() override;
};

UCLASS()
class TENIPARIZHA_API ATPGoodActor : public ACharacter
{
	GENERATED_BODY()
public:
	ATPGoodActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	UFUNCTION() void HandleDamaged(float Amount, AActor* InInstigator);
	UFUNCTION(BlueprintImplementableEvent) void OnGoodEvent(int32 Code);

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Body = nullptr;
	UPROPERTY(VisibleAnywhere) UTPGoodComponent* Combat = nullptr;
	UPROPERTY(VisibleAnywhere) UPointLightComponent* Lamp = nullptr;
	UPROPERTY(Transient) UMaterialInstanceDynamic* Mid = nullptr;
	UPROPERTY() TArray<UTexture2D*> Textures;
	UPROPERTY() TMap<FName, AActor*> Named;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) ETPGoodState State = ETPGoodState::Idle;
	UPROPERTY(EditAnywhere) FTPGoodInfo Info;
	TWeakObjectPtr<AActor> LastTarget;
protected:
	virtual void BeginPlay() override;
private:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void JumpPressed();
	void OnTimer();
	FTimerHandle Timer;
	float Accum = 0.f;
};
