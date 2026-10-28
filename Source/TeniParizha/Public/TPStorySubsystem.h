// Тени Парижа — сюжет: главы, Лига крыш (12 гонок шести типов, медали, награды-способности),
// альбом «Лица квартала», знакомства с жителями, сохранение. Порт legacy/teni_v1/game2/game.c.
// Владелец модуля: агент «жители, сюжет и интерфейс».
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "TPStorySubsystem.generated.h"

class ATPCharacter;
class ATPCitizen;
class ATPStoryGate;
class ATPHUD;
class ATPEnvironment;
class UTPIdentitySubsystem;
class AActor;

/** Тип гонки Лиги крыш. */
UENUM(BlueprintType)
enum class ETPRaceType : uint8
{
	Sprint UMETA(DisplayName = "Спринт"),
	Roofs  UMETA(DisplayName = "Не касаясь мостовой"),
	Rival  UMETA(DisplayName = "Дуэль"),
	Hunt   UMETA(DisplayName = "Охота за флажками"),
	Tower  UMETA(DisplayName = "Восхождение"),
	Thief  UMETA(DisplayName = "Погоня за вором")
};

UENUM(BlueprintType)
enum class ETPRaceState : uint8
{
	None,
	Countdown,
	Running
};

/** Трасса гонки, построенная по геометрии сгенерированного квартала. */
struct FTPRaceCourse
{
	bool bBuilt = false;
	FVector Start = FVector::ZeroVector;
	float StartYaw = 0.f;
	/** высота мостовой у старта */
	float StreetZ = 0.f;
	/** центры ворот (≈ 110 см над поверхностью) */
	TArray<FVector> Gates;
	/** 'G' — улица, 'R' — крыша, 'T' — вершина */
	TArray<TCHAR> Kinds;
	float Ref = 60.f, Gold = 75.f, Silver = 95.f, Bronze = 130.f, Limit = 130.f;
};

UCLASS()
// В UE 4.27 нет UTickableWorldSubsystem: тик даёт FTickableGameObject (CDO не тикает — см. GetTickableTickType)
class TENIPARIZHA_API UTPStorySubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	static constexpr int32 NumRaces = 12;
	static constexpr int32 NumAbilities = 6;

	/* ---- USubsystem ---- */
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	/* ---- FTickableGameObject ---- */
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }

	/* ---------- сюжет (для интерфейса) ---------- */
	UFUNCTION(BlueprintPure, Category = "Сюжет") int32 GetChapter() const { return Chapter; }
	UFUNCTION(BlueprintPure, Category = "Сюжет") FString GetChapterTitle() const;
	UFUNCTION(BlueprintPure, Category = "Сюжет") FString GetObjective() const;

	/* ---------- альбом «Лица квартала» ---------- */
	/** 0 — незнаком, 1 — знаком, 2 — портрет написан */
	UFUNCTION(BlueprintPure, Category = "Альбом") int32 GetAlbumStatus(int32 Index) const { return Album.IsValidIndex(Index) ? Album[Index] : 0; }
	UFUNCTION(BlueprintPure, Category = "Альбом") int32 CountAlbum(int32 MinStatus) const;

	/* ---------- гонки ---------- */
	UFUNCTION(BlueprintPure, Category = "Гонки") bool IsRaceActive() const { return RaceState != ETPRaceState::None; }
	ETPRaceState GetRaceState() const { return RaceState; }
	float GetCountdown() const { return Countdown; }
	float GetRaceTime() const { return RaceTime; }
	int32 GetMedal(int32 Race) const { return Medals.IsValidIndex(Race) ? Medals[Race] : 0; }
	FString GetRaceTitle() const;
	/** строка состояния гонки: ворота, флажки, соперник */
	FString GetRaceStatus() const;
	/** пороги медалей текущей гонки (false — у гонки нет порогов: дуэль, погоня) */
	bool GetMedalTimes(float& Gold, float& Silver, float& Bronze) const;
	/** главная цель: следующие ворота, ближайший флажок или вор */
	bool GetRaceTarget(FVector& Out) const;
	/** все видимые отметки гонки (для миникарты) */
	void GetRaceMarkers(TArray<FVector>& Out) const;
	/** ввод героя заблокирован (обратный отсчёт) */
	bool WantsInputLock() const { return RaceState == ETPRaceState::Countdown; }
	void BeginRace(int32 Race);
	void AbortRace();

	/* ---------- жители ---------- */
	const TArray<TWeakObjectPtr<ATPCitizen>>& GetCitizens() const { return Citizens; }
	/** ближайший житель, с которым можно заговорить */
	ATPCitizen* FindTalkTarget(const AActor* InHero, float Radius = 250.f) const;
	/** подсказка «G — …» для жителя */
	FString GetTalkPrompt(const ATPCitizen* Citizen) const;
	/** бегун Лиги с непройденной открытой гонкой (метка над головой) */
	bool RunnerHasNews(const ATPCitizen* Citizen) const;
	bool IsRunner(const ATPCitizen* Citizen) const;
	bool IsPainter(const ATPCitizen* Citizen) const;

	/* ---------- способности и укрытие ---------- */
	bool HasAbility(int32 Ability) const { return Ability >= 0 && Ability < NumAbilities && (Abilities & (1 << Ability)) != 0; }
	/** герой идёт шагом среди горожан — стража замечает его хуже */
	bool IsHeroInCrowd() const { return bHeroInCrowd; }

	/* ---------- сохранение ---------- */
	UFUNCTION(BlueprintCallable, Category = "Сюжет") void SaveProgress();

	static FString RaceTypeName(ETPRaceType Type);
	static FString FormatTime(float Seconds);
	static FString MedalName(int32 Medal);
	static FString AbilityName(int32 Ability);
	static FString AbilityDesc(int32 Ability);

protected:
	UFUNCTION() void HandleInteract();

	void LoadProgress();
	void SpawnCitizens();
	void BindHero(ATPCharacter* NewHero);
	void ApplyAbilities();
	void StartPrologue();
	void ChapterCheck();
	void Talk(ATPCitizen* Citizen);
	void OpenRaceMenu(int32 Giver);
	bool RaceOpen(int32 Race) const;
	bool ChapterDone(int32 InChapter) const;
	int32 PoolNext() const;
	bool AwardPortrait(int32 Index, TArray<int32>& OutNew);

	/* гонки */
	void BuildCourse(int32 Race);
	bool TraceDown(const FVector& XY, FVector& OutHit, FVector& OutNormal) const;
	bool TraceRange(const FVector& From, const FVector& To, FVector& OutHit, FVector& OutNormal) const;
	bool PickStreetPoint(const FVector& From, const FVector& Heading, const TArray<FVector>& Used, FRandomStream& Rng, FVector& Out) const;
	bool PickRoofPoint(const FVector& From, const FVector& Heading, bool bFromRoof, float StreetZ, FRandomStream& Rng, FVector& Out) const;
	bool PickSummit(const FVector& From, FRandomStream& Rng, FVector& Out) const;
	void BuildRunnerPath(float StartAhead);
	void StepRunner(float Dt);
	void UpdateRace(float Dt);
	void UpdateGateHighlights();
	void FinishRace(bool bSuccess, const FString& Why);
	void SetNight(bool bOn);
	bool NearGate(const FVector& Hero, const FVector& Gate, bool bTower) const;

	ATPHUD* GetHUD() const;
	ATPEnvironment* GetEnvironment() const;
	const UTPIdentitySubsystem* GetIdentities() const;
	ATPCitizen* CitizenFor(int32 IdentityIndex) const;

	/* прогресс */
	int32 Chapter = 0;
	bool bEnding = false;
	int32 Abilities = 0;
	TArray<int32> Medals;
	TArray<float> BestTimes;
	TArray<uint8> Album;
	float SavedTimeOfDay = -1.f;
	float CurrentTimeOfDay = -1.f;
	mutable TWeakObjectPtr<ATPEnvironment> EnvCache;
	bool bDirty = false;
	float AutoSaveTimer = 0.f;

	/* мир */
	/** Initialize прошёл, Deinitialize ещё нет */
	bool bInitialized = false;
	/** старт игры (замена OnWorldBeginPlay): ждём W->HasBegunPlay() в Tick */
	void StartWorld(UWorld& InWorld);
	bool bStarted = false;
	bool bSpawned = false;
	bool bPendingPrologue = false;
	int32 SpawnTries = 0;
	FTimerHandle SpawnTimer;
	TWeakObjectPtr<ATPCharacter> Hero;
	float BaseSprintSpeed = -1.f;
	float BaseClimbSpeed = -1.f;
	TArray<TWeakObjectPtr<ATPCitizen>> Citizens;
	TMap<int32, TWeakObjectPtr<ATPCitizen>> ByIdentity;
	TArray<FVector> WalkPoints;
	FBox CityBounds = FBox(ForceInit);
	FVector PlayerStart = FVector::ZeroVector;
	int32 TalkSalt = 0;
	bool bHeroInCrowd = false;
	float CrowdTimer = 0.f;
	void UpdateCrowd(float Dt);

	/* текущая гонка */
	ETPRaceState RaceState = ETPRaceState::None;
	int32 CurRace = -1;
	float Countdown = 0.f;
	float RaceTime = 0.f;
	int32 NextGate = 0;
	int32 FlagsLeft = 0;
	TArray<bool> GotFlag;
	TArray<FTPRaceCourse> Courses;
	UPROPERTY(Transient) TArray<ATPStoryGate*> GateActors;
	TWeakObjectPtr<ATPCitizen> RaceRunner;
	TArray<FVector> RunnerPath;
	TArray<int32> RunnerPathGate;
	int32 RunnerWp = 0;
	int32 RunnerGate = 0;
	float RunnerSpeed = 600.f;
	float RunnerTotal = 60.f;
	bool bRunnerDone = false;
	bool bNightSet = false;
	float PrevTimeOfDay = 12.f;
};
