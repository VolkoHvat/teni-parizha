// Тени Парижа — контроллер стражника: движение по навмешу (MoveToLocation), а если навигации нет или путь
// не найден — напрямую (AddMovementInput) с простым обходом стен. Решения принимает сам ATPGuard.
// Владелец модуля: агент «бой и стража».
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TPGuardController.generated.h"

UCLASS()
class TENIPARIZHA_API ATPGuardController : public AAIController
{
	GENERATED_BODY()

public:
	ATPGuardController();

	/** идти к цели; возвращает true, если уже на месте (в пределах Acceptance, по горизонтали) */
	bool MoveGuardTo(const FVector& Goal, float Acceptance, float DeltaTime);
	/** остановиться (прервать путь) */
	void StopGuard();
	/** идёт ли сейчас прямое движение без навмеша */
	bool IsMovingDirect() const { return bDirect; }

protected:
	bool IsBlocked(const FVector& From, const FVector& Dir) const;

	FVector LastGoal = FVector(1e9f);
	float RepathTimer = 0.f;
	bool bDirect = false;
	float DirectTimer = 0.f;
	float StuckTime = 0.f;
	float AvoidYaw = 0.f;
	float AvoidTime = 0.f;
	int32 ShortPaths = 0;
};
