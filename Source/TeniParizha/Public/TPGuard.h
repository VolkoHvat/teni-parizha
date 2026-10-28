// Тени Парижа — стражник: патруль по улицам, зрение (конус и проверка видимости), тревога,
// погоня, поиск, ближний бой по жетону (UTPCombatDirector), мушкетёры, оглушение с рэгдоллом.
// Владелец модуля: агент «бой и стража». Логика перенесена из legacy/teni_v1/game2/game.c (npcs_update, ветка «стражник»).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPGuard.generated.h"

class UTPCombatComponent;
class UStaticMeshComponent;
class ATPGuardController;
class ATPEnvironment;

UENUM(BlueprintType)
enum class ETPGuardState : uint8
{
	Patrol      UMETA(DisplayName = "Патруль"),
	Suspicious  UMETA(DisplayName = "Подозрение"),
	Chase       UMETA(DisplayName = "Погоня"),
	Search      UMETA(DisplayName = "Поиск"),
	KO          UMETA(DisplayName = "Повержен")
};

UCLASS()
class TENIPARIZHA_API ATPGuard : public ACharacter
{
	GENERATED_BODY()

public:
	ATPGuard();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Бой") UTPCombatComponent* Combat = nullptr;
	/** треуголка: поля (сплющенный цилиндр) и тулья (низкий конус), на сокете head */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Внешность") UStaticMeshComponent* HatBrim = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Внешность") UStaticMeshComponent* HatCrown = nullptr;
	/** мушкет за спиной (только у мушкетёров) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Внешность") UStaticMeshComponent* Musket = nullptr;

	/* ---- для интерфейса ---- */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Стража") ETPGuardState State = ETPGuardState::Patrol;
	/** уровень тревоги 0..1 (1 — погоня) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Стража") float Alert = 0.f;
	/** прицеливание мушкетёра, с (выстрел при AimDuration) */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Стража") float AimTime = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Стража") float Reload = 0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Стража") FVector LastSeen = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража") bool bMusketeer = false;
	/** маршрут патруля в мире (2–4 точки); задаётся режиссёром при расстановке */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража") TArray<FVector> PatrolRoute;

	/* ---- баланс ---- */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float SightRange = 2600.f;
	/** половина угла обзора, градусы (конус ~110°) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float SightHalfAngle = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float PatrolSpeed = 170.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float SearchSpeed = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float ChaseSpeed = 520.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float MeleeRange = 340.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float CircleRadius = 210.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float AimDuration = 1.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float ReloadDuration = 5.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float MusketDamage = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float MusketMinRange = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float MusketMaxRange = 3000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float KODuration = 45.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float TakedownKODuration = 30.f;
	/** герой выше на столько см и недосягаем дольше UnreachableTime — стража бросает погоню */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float UnreachableHeight = 400.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Стража|Баланс") float UnreachableTime = 6.f;

	/* ---- внешность ---- */
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> MeshCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> AnimClassCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") FName HatSocket = TEXT("head");
	/** положение шляпы в осях кости head (у манекена ось X кости направлена вверх по шее) */
	UPROPERTY(EditAnywhere, Category = "Внешность") FVector HatOffset = FVector(18.f, 2.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Внешность") FRotator HatRotation = FRotator(-90.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, Category = "Внешность") FLinearColor CoatColor = FLinearColor(0.07f, 0.12f, 0.38f);
	UPROPERTY(EditAnywhere, Category = "Внешность") FLinearColor HatColor = FLinearColor(0.03f, 0.03f, 0.04f);

	/* ---- для других систем ---- */
	UFUNCTION(BlueprintPure, Category = "Стража") bool IsKO() const { return State == ETPGuardState::KO; }
	UFUNCTION(BlueprintPure, Category = "Стража") float GetAimProgress() const { return AimDuration > 0.f ? FMath::Clamp(AimTime / AimDuration, 0.f, 1.f) : 0.f; }
	/** оглушить (рэгдолл); через Duration с поднимется и начнёт поиск */
	UFUNCTION(BlueprintCallable, Category = "Стража") void KnockOut(float Duration, AActor* By, bool bSilent);
	/** видит ли стражник точку (конус, дальность, трассировка видимости) */
	UFUNCTION(BlueprintPure, Category = "Стража") bool CanSeePoint(FVector Point, float Range, const AActor* IgnoreActor) const;
	/** пойти в погоню за героем, замеченным в точке Seen */
	void EnterChase(const FVector& Seen);
	/** насторожиться (свидетель происшествия) */
	void AddSuspicion(float Amount, const FVector& Seen);
	/** вернуться к патрулю (сброс тревоги) */
	void ResetToPatrol();
	/** компонент боя сообщает: по стражнику попали */
	void OnCombatHit(AActor* Attacker, bool bHeavy);

protected:
	void ApplyAppearance();
	void GetUp();
	void EnterSearch(float WaitTime);
	void TickChase(float Dt, ACharacter* Hero, bool bSee, float& OutMoveSpeed, bool& bOutMove, bool& bOutManualFacing);
	void FireMusket(ACharacter* Hero);
	void SpawnMuzzleFlash();
	void FaceDirection(const FVector& Dir, float Dt, float Speed);
	float ComputeSightRange(const ACharacter* Hero) const;
	bool IsNight() const;
	ATPGuardController* GetGuardController() const;

	float KOTimer = 0.f;
	bool bRagdoll = false;
	float Wait = 0.f;
	float Lost = 0.f;
	float Unreachable = 0.f;
	float SearchTime = 0.f;
	float StrafeSign = 0.f;
	int32 RouteIndex = 0;
	FVector MeshOffset = FVector(0.f, 0.f, -92.f);
	FRotator MeshRotation = FRotator(0.f, -90.f, 0.f);
	mutable TWeakObjectPtr<ATPEnvironment> Environment;
	mutable float EnvSearchTimer = 0.f;
};
