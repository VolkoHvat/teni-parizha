// Тени Парижа — житель квартала: гуляет по улицам, болтает с соседями, оглядывается на героя,
// расступается и падает, если его сбить на бегу. Личность берётся из UTPIdentitySubsystem.
// Владелец модуля: агент «жители, сюжет и интерфейс».
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TPCitizen.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** Что сейчас делает житель (внутреннее состояние, без рефлексии). */
enum class ETPCitizenMode : uint8
{
	Idle,      // постоял и пошёл дальше
	Stand,     // стоит на своём месте (бегуны Лиги, художник)
	Walk,      // идёт к точке прогулки
	Talk,      // разговаривает с соседом
	Sidestep,  // уступает дорогу герою
	Down,      // сбит с ног
	Racing     // бежит в гонке Лиги (управляет сюжет)
};

UCLASS()
class TENIPARIZHA_API ATPCitizen : public ACharacter
{
	GENERATED_BODY()

public:
	ATPCitizen();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	/** шум драки или выстрела (событие UTPCombatDirector::OnPanic): разбежаться */
	UFUNCTION() void HandlePanic(FVector Location, float Radius);
	/** испугаться и убежать от точки From */
	void Panic(const FVector& From);
	bool IsFleeing() const { return bFleeing; }

	/** задать личность и точки прогулок; вызывать до FinishSpawning (или сразу после спавна) */
	void InitCitizen(int32 InIdentityIndex, const TArray<FVector>& InWalkPoints, bool bInStationary);

	UFUNCTION(BlueprintPure, Category = "Житель") int32 GetIdentityIndex() const { return IdentityIndex; }
	/** реплика над головой */
	UFUNCTION(BlueprintCallable, Category = "Житель") void Say(const FString& Text, float Seconds = 2.6f);
	const FString& GetBark() const { return BarkText; }
	/** 0..1 — видимость реплики (для интерфейса) */
	float GetBarkAlpha() const;

	bool IsDown() const { return Mode == ETPCitizenMode::Down; }
	bool IsRacing() const { return Mode == ETPCitizenMode::Racing; }
	bool IsStationary() const { return bStationary; }

	/** герой заговорил с жителем: остановиться и повернуться к нему */
	void OnTalkedTo(AActor* Talker);
	/** посмотреть на точку некоторое время (оглянуться) */
	void LookAtFor(const FVector& Where, float Seconds);

	/* гонки: житель-соперник бежит по маршруту «призраком», не задевая стен */
	void SetRacing(bool bOn);
	void RaceStep(const FVector& Target, float Speed);
	/** вернуться на своё место (после гонки) */
	void ReturnHome();

	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> MaleMeshCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> FemaleMeshCandidates;
	UPROPERTY(EditAnywhere, Category = "Внешность") TArray<FString> AnimClassCandidates;

	FVector HomeLocation = FVector::ZeroVector;
	float HomeYaw = 0.f;

protected:
	void ApplyAppearance();
	UStaticMeshComponent* AddPart(UStaticMesh* InMesh, const FVector& RelLoc, const FVector& Scale, const FColor& Color);
	void SetAccessoriesVisible(bool bVisible);
	void PickNewTarget();
	void IssueMove();
	void UpdateWalk(float Dt);
	void CheckHero(float Dt);
	void Knockdown(const FVector& PushDir, ACharacter* Hero);
	void Bump(const FVector& PushDir, ACharacter* Hero);
	void Recover();
	void Witness(float Amount) const;
	void TryStartChat();
	void StopWalking();
	void FaceYaw(float Yaw, float Dt, float DegPerSec);
	void EnterIdle(float MinTime, float MaxTime);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Житель") int32 IdentityIndex = -1;
	UPROPERTY(Transient) TArray<UMaterialInstanceDynamic*> Mids;
	UPROPERTY(Transient) TArray<UStaticMeshComponent*> Accessories;

	TWeakObjectPtr<ATPCitizen> ChatPartner;
	TArray<FVector> WalkPoints;
	bool bStationary = false;
	bool bHeavy = false;
	bool bFemale = false;
	bool bManualMove = false;
	bool bRagdoll = false;
	bool bAppearanceDone = false;
	bool bFleeing = false;

	ETPCitizenMode Mode = ETPCitizenMode::Idle;
	ETPCitizenMode ModeBeforeSidestep = ETPCitizenMode::Idle;
	float ModeTime = 0.f;
	float ModeDuration = 1.f;
	FVector Target = FVector::ZeroVector;
	FVector LastProgressPos = FVector::ZeroVector;
	float ProgressTimer = 0.f;
	FVector SideDir = FVector::ZeroVector;

	FString BarkText;
	float BarkTime = 0.f;
	float ContactCooldown = 0.f;
	float LookTime = 0.f;
	FVector LookTarget = FVector::ZeroVector;
	float ChatTimer = 0.f;
	float WalkSpeed = 140.f;

	FVector MeshRelLoc = FVector(0.f, 0.f, -90.f);
	FRotator MeshRelRot = FRotator(0.f, -90.f, 0.f);
	FRandomStream Rng;
};
