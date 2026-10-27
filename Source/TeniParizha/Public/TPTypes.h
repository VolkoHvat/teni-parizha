// Тени Парижа — общие типы. Единицы Unreal: сантиметры, ось Z вверх, персонаж смотрит вдоль +X.
#pragma once

#include "CoreMinimal.h"
#include "TPTypes.generated.h"

/** Состояние свободного бега (паркура). */
UENUM(BlueprintType)
enum class ETPTraversal : uint8
{
	Ground   UMETA(DisplayName = "Земля"),
	Jump     UMETA(DisplayName = "Толчок"),
	Air      UMETA(DisplayName = "Полёт"),
	Land     UMETA(DisplayName = "Приземление"),
	Roll     UMETA(DisplayName = "Кувырок"),
	Climb    UMETA(DisplayName = "Лазание"),
	Hang     UMETA(DisplayName = "Вис"),
	ClimbUp  UMETA(DisplayName = "Подтягивание"),
	Drop     UMETA(DisplayName = "Спуск в вис"),
	Vault    UMETA(DisplayName = "Перемах"),
	WallRun  UMETA(DisplayName = "Бег по стене"),
	Turn     UMETA(DisplayName = "Разворот"),
	Hop      UMETA(DisplayName = "Перескок")
};

/** Уступ, найденный сенсорами паркура. */
USTRUCT(BlueprintType)
struct FTPLedge
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool bValid = false;
	/** точка на стене под кромкой */
	UPROPERTY(BlueprintReadOnly) FVector WallPoint = FVector::ZeroVector;
	/** горизонтальная нормаль стены (наружу) */
	UPROPERTY(BlueprintReadOnly) FVector WallNormal = FVector::ZeroVector;
	/** точка на кромке */
	UPROPERTY(BlueprintReadOnly) FVector Edge = FVector::ZeroVector;
	/** направление вдоль кромки (вправо, если смотреть на стену) */
	UPROPERTY(BlueprintReadOnly) FVector EdgeDir = FVector::ZeroVector;
	/** высота кромки над ногами персонажа, см */
	UPROPERTY(BlueprintReadOnly) float Height = 0.f;
	/** можно ли встать на уступ */
	UPROPERTY(BlueprintReadOnly) bool bCanStand = false;
};

/** Ввод одного кадра для системы паркура (уже переведённый в мировые оси относительно камеры). */
USTRUCT(BlueprintType)
struct FTPMoveInput
{
	GENERATED_BODY()

	/** желаемое направление в мире (XY), длина 0..1 */
	UPROPERTY(BlueprintReadWrite) FVector Move = FVector::ZeroVector;
	UPROPERTY(BlueprintReadWrite) bool bSprint = false;   // свободный бег
	UPROPERTY(BlueprintReadWrite) bool bJump = false;     // нажатие (однократное)
	UPROPERTY(BlueprintReadWrite) bool bDrop = false;     // отпустить руки (однократное)
	UPROPERTY(BlueprintReadWrite) bool bCrouch = false;   // красться / спуск с края
	UPROPERTY(BlueprintReadWrite) bool bTurn = false;     // разворот (однократное)
};

/** Действия боя. */
UENUM(BlueprintType)
enum class ETPCombatAction : uint8
{
	None, Attack, Counter, Dodge, Takedown
};

/** Социальный класс жителя квартала (как в первой версии игры). */
UENUM(BlueprintType)
enum class ETPSocialClass : uint8
{
	Labor, Artisan, Shop, Soldier, Noble, Clergy, Vagrant, Servant, Scholar, Sailor
};

/** Пути к ресурсам, которые создаёт скрипт настройки Content/Python/tp_setup.py */
namespace TPPaths
{
	static const TCHAR* BaseMaterial = TEXT("/Game/TeniParizha/Materials/M_TPBase.M_TPBase");
	static const TCHAR* BaseMaterialMasked = TEXT("/Game/TeniParizha/Materials/M_TPBaseMasked.M_TPBaseMasked");
	static const TCHAR* EmissiveMaterial = TEXT("/Game/TeniParizha/Materials/M_TPEmissive.M_TPEmissive");
}
