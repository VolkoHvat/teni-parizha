// Плохие примеры: отражение (UHT) и сборщик мусора.
#pragma once
#include "CoreMinimal.h"
#include "TPBadTypes.generated.h"
#include "GameFramework/Actor.h"
// ОЖИДАЕТСЯ: .generated.h должен быть последним
class UTexture2D;

UENUM(BlueprintType)
enum class ETPBadEnum
{
	A, B
};
// ОЖИДАЕТСЯ: должен быть ": uint8"

USTRUCT(BlueprintType)
struct FTPBadStruct
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) double Precise = 0.0;
	// ОЖИДАЕТСЯ: с типом double
	UPROPERTY() TArray<TArray<int32>> Grid;
	// ОЖИДАЕТСЯ: вложенные контейнеры
	UTexture2D* Loose = nullptr;
	// ОЖИДАЕТСЯ: поле «Loose» (UTexture2D *) хранит UObject без UPROPERTY()
};

UCLASS()
class ATPBadActor : public AActor
{
	GENERATED_BODY()
public:
	float Speed = 0.f;
	TArray<AActor*> Friends;
	// ОЖИДАЕТСЯ: поле «Friends»
	void SetSpeed(float NewSpeed);
	void SetSpeedInline(float Speed) { (void)Speed; }
	// ОЖИДАЕТСЯ: параметр «Speed» в ATPBadActor::SetSpeedInline скрывает поле ATPBadActor::Speed
	void Declared(float Speed);
	// НЕ_ОЖИДАЕТСЯ: ATPBadActor::Declared
	void Handler(float Amount);
	UFUNCTION(BlueprintImplementableEvent) void OnBp();
	virtual void BeginPlay() override;
};
