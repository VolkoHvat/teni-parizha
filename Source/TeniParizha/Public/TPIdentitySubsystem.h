// Тени Парижа — 128 жителей квартала: имена, возраст, сословие, ремесло, черты характера, биографии.
// Порт генератора первой версии игры (legacy/teni_v1/game2/ident.c). Генерация детерминирована.
// Владелец модуля: агент «жители, сюжет и интерфейс».
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TPTypes.h"
#include "TPIdentitySubsystem.generated.h"

/** Особая роль жителя в сюжете. */
UENUM(BlueprintType)
enum class ETPCitizenRole : uint8
{
	None    UMETA(DisplayName = "Житель"),
	Runner  UMETA(DisplayName = "Бегун Лиги крыш"),
	Painter UMETA(DisplayName = "Художник"),
	Thief   UMETA(DisplayName = "Карманник")
};

namespace TPCitizens
{
	/** жителей в квартале */
	constexpr int32 Num = 128;
	/** бегунов Лиги крыш */
	constexpr int32 NumRunners = 5;
}

/** Личность жителя квартала. */
USTRUCT(BlueprintType)
struct FTPIdentity
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Name;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") int32 Age = 30;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") bool bFemale = false;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Job;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") ETPSocialClass Class = ETPSocialClass::Labor;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Trait1;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Trait2;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Bio;
	UPROPERTY(BlueprintReadOnly, Category = "Житель") ETPCitizenRole Role = ETPCitizenRole::None;
	/** номер бегуна Лиги (0..4) или -1 */
	UPROPERTY(BlueprintReadOnly, Category = "Житель") int32 RunnerIndex = -1;
	/** прозвище (у бегунов) */
	UPROPERTY(BlueprintReadOnly, Category = "Житель") FString Nick;

	/* облик: цвета в sRGB */
	UPROPERTY(BlueprintReadOnly, Category = "Облик") FColor Skin = FColor(230, 190, 160);
	UPROPERTY(BlueprintReadOnly, Category = "Облик") FColor Hair = FColor(60, 44, 36);
	UPROPERTY(BlueprintReadOnly, Category = "Облик") FColor Coat = FColor(100, 90, 70);
	UPROPERTY(BlueprintReadOnly, Category = "Облик") FColor Pants = FColor(70, 64, 60);
	UPROPERTY(BlueprintReadOnly, Category = "Облик") FColor Trim = FColor(240, 236, 226);
	/** множитель роста (масштаб модели) */
	UPROPERTY(BlueprintReadOnly, Category = "Облик") float Height = 1.f;

	int32 Trait1Index = 0;
	int32 Trait2Index = 0;
};

/** Подсистема личностей: создаётся вместе с игрой и хранит всех 128 жителей. */
UCLASS()
class TENIPARIZHA_API UTPIdentitySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "Жители") int32 Num() const { return Citizens.Num(); }
	UFUNCTION(BlueprintPure, Category = "Жители") FTPIdentity GetIdentity(int32 Index) const;
	/** быстрый доступ без копии (индекс должен быть корректным — иначе вернётся пустая личность) */
	const FTPIdentity& Get(int32 Index) const;

	UFUNCTION(BlueprintPure, Category = "Жители") int32 GetPainter() const { return PainterId; }
	UFUNCTION(BlueprintPure, Category = "Жители") int32 GetThief() const { return ThiefId; }
	UFUNCTION(BlueprintPure, Category = "Жители") int32 GetRunner(int32 RunnerIndex) const { return RunnerIds.IsValidIndex(RunnerIndex) ? RunnerIds[RunnerIndex] : -1; }
	/** слух или присказка жителя (Salt — чтобы реплики менялись) */
	UFUNCTION(BlueprintPure, Category = "Жители") FString GetRumor(int32 Index, int32 Salt) const;

	static FString ClassName(ETPSocialClass Class);
	static FLinearColor ClassColor(ETPSocialClass Class);
	static FString RunnerNick(int32 RunnerIndex);
	/** детерминированный хеш первой версии игры */
	static uint32 Hash(int32 A, int32 B);

protected:
	void Generate();
	void AssignRoles();
	void FinishIdentities();
	void MakeLook(FTPIdentity& D, int32 K) const;

	UPROPERTY() TArray<FTPIdentity> Citizens;
	int32 PainterId = -1;
	int32 ThiefId = -1;
	TArray<int32> RunnerIds;
};
