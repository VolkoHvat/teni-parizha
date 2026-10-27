// Тени Парижа — сюжет, Лига крыш, альбом и сохранение.
#include "TPStorySubsystem.h"
#include "TeniParizha.h"
#include "TPCharacter.h"
#include "TPCitizen.h"
#include "TPStoryGate.h"
#include "TPHUD.h"
#include "TPEnvironment.h"
#include "TPCityGenerator.h"
#include "TPTraversalComponent.h"
#include "TPCombatComponent.h"
#include "TPIdentitySubsystem.h"
#include "TPSaveGame.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "Stats/Stats.h"

// Подсистему боя пишет другой агент: подключаем, только если заголовок уже есть
#if __has_include("TPCombatDirector.h")
#include "TPCombatDirector.h"
#define TP_STORY_HAS_DIRECTOR 1
#else
#define TP_STORY_HAS_DIRECTOR 0
#endif

namespace TPStoryData
{
	struct FRaceDef
	{
		const TCHAR* Title;
		ETPRaceType Type;
		int32 Giver;      // номер бегуна Лиги, который предлагает гонку
		int32 Chapter;    // с какой главы открыта
		int32 Reward;     // способность за первую медаль (-1 — нет)
		const TCHAR* Gates; // 'G' — улица, 'R' — крыша, 'T' — вершина
		const TCHAR* Intro;
		bool bNight;
		float Difficulty;
	};

	// 12 гонок первой версии игры (RD[] в game.c); трассы строятся по геометрии квартала
	static const FRaceDef Races[UTPStorySubsystem::NumRaces] = {
		{TEXT("Первые шаги"), ETPRaceType::Sprint, 0, 1, 0, TEXT("GGGRR"), TEXT("Жак|Новичок? Беги по светящимся кольцам: улица, площадь, а потом наверх, на крышу. Лезть можно по любой стене — просто беги в неё!"), false, 1.f},
		{TEXT("Колокольня"), ETPRaceType::Tower, 0, 1, 1, TEXT("T"), TEXT("Жак|Видишь самую высокую крышу в округе? Кто первым наверху — тот и прав. Время пошло!"), false, 1.f},
		{TEXT("Не касаясь мостовой"), ETPRaceType::Roofs, 1, 1, 2, TEXT("RRRRR"), TEXT("Лиза|Правило одно: как залез на первую крышу — мостовая для тебя лава. Улицу перепрыгивай с разбега: Shift и Пробел у самого края."), false, 1.f},
		{TEXT("Флажки Лиги"), ETPRaceType::Hunt, 2, 2, 3, TEXT("RRRRRR"), TEXT("Эмиль|Шесть флажков Лиги на крышах вокруг. Порядок любой, время ограничено. По моим расчётам, успеть можно!"), false, 1.f},
		{TEXT("Держи вора!"), ETPRaceType::Thief, 4, 2, 4, TEXT("GGRRGG"), TEXT("Ворон|Видишь того карманника? Он только что срезал кошелёк у торговки. Поймаешь — поговорим всерьёз."), false, 1.f},
		{TEXT("Прачка против курьера"), ETPRaceType::Rival, 1, 2, 5, TEXT("GRRRGG"), TEXT("Лиза|Теперь со мной наперегонки. Я бегаю с корзинами белья по этим крышам каждый день. Не отставай!"), false, 1.f},
		{TEXT("Мосты Сены"), ETPRaceType::Sprint, 0, 3, -1, TEXT("GGGGGGGG"), TEXT("Жак|Длинный забег по набережным и переулкам: восемь колец. Береги силы на спринт."), false, 1.f},
		{TEXT("Часовая башня"), ETPRaceType::Sprint, 2, 3, -1, TEXT("GGGT"), TEXT("Эмиль|Три кольца по улицам — и на самый верх башни. Кольца покажут дорогу."), false, 1.f},
		{TEXT("Крыши Сент-Антуана"), ETPRaceType::Roofs, 1, 3, -1, TEXT("RRRRRR"), TEXT("Лиза|Длинная цепочка крыш. Мостовая — лава, ты помнишь."), false, 1.f},
		{TEXT("Дуэль с Вороном"), ETPRaceType::Rival, 4, 4, -1, TEXT("GGGGGT"), TEXT("Ворон|До самой высокой крыши квартала. Ещё никто не обгонял меня на этом пути. Посмотрим, курьер."), false, 1.f},
		{TEXT("Ночной забег"), ETPRaceType::Hunt, 3, 5, -1, TEXT("RRRRRR"), TEXT("Тень|Ночью стража дремлет, а флажки светятся. Шесть штук на крышах вокруг."), true, 1.f},
		{TEXT("Сад маркиза"), ETPRaceType::Tower, 3, 5, -1, TEXT("T"), TEXT("Тень|Самая высокая крыша у поместья маркиза. Стража не спит. Наверх — и ни звука."), false, 1.f}};

	static const TCHAR* AbilityNames[UTPStorySubsystem::NumAbilities] = {
		TEXT("Хватка"), TEXT("Перекат"), TEXT("Прыжок с разбега"), TEXT("Перемах"), TEXT("Быстрые ноги I"), TEXT("Быстрые ноги II")};
	static const TCHAR* AbilityDescs[UTPStorySubsystem::NumAbilities] = {
		TEXT("Ловите выступы и карнизы в падении и после прыжка."),
		TEXT("Приземление с кувырком: падение с большой высоты без урона и без потери скорости."),
		TEXT("На свободном бегу герой сам отталкивается у края крыши и перелетает улицу."),
		TEXT("Низкие ограды, бочки и парапеты преодолеваются на бегу одним движением."),
		TEXT("Свободный бег становится быстрее."),
		TEXT("Свободный бег ещё быстрее, а лазание — заметно проворнее.")};

	static const TCHAR* ChapterNames[6] = {TEXT("Пролог"), TEXT("Глава 1. Новичок"), TEXT("Глава 2. Свой среди крыш"), TEXT("Глава 3. Вызов Ворону"), TEXT("Финал"), TEXT("Лига крыш")};

	static const TCHAR* Prologue[] = {
		TEXT("Мэтр Огюстен|Ах, молодой человек! Вы ведь Адриен, курьер? Я Огюстен Брюн, художник. Я задумал великое: «Лица квартала», портреты всех ста двадцати восьми соседей."),
		TEXT("Мэтр Огюстен|Но парижане не желают позировать безвестному мазиле. Им подавай знаменитость! А кто нынче знаменит в квартале? Лига крыш: сорвиголовы, что носятся по черепице наперегонки."),
		TEXT("Мэтр Огюстен|Станьте звездой Лиги, и квартал сам выстроится ко мне в очередь. За каждую вашу победу я напишу новые портреты. Альбом всегда при вас: клавиша Tab."),
		TEXT("Мэтр Огюстен|Начните с Жака-Трубочиста: он учит новичков. А главный в Лиге — Ворон. Говорят, его ещё никто не обгонял."),
		TEXT("Подсказка|Бегуны Лиги отмечены золотыми ромбами на карте и над головой. Подойдите и нажмите G. Говорить можно с любым жителем — так вы с ним познакомитесь. F1 — управление.")};
	static const TCHAR* Chapter2[] = {
		TEXT("Записка на трубе|«Новичок неплох. Хочешь в Лигу по-настоящему — поймай карманника, а потом обгони Лизу. — В.»"),
		TEXT("Мэтр Огюстен|Весь рынок только о вас и судачит! Прачка Лиза и студент Эмиль согласились позировать. Новые бегуны ждут вас.")};
	static const TCHAR* Chapter3[] = {
		TEXT("Эмиль|Ворон заметил тебя. Он такое редко говорит, но сказал: «Курьер бегает почти как я». Почти!"),
		TEXT("Мэтр Огюстен|Тень и даже тот карманник пришли ко мне сами. Три последних испытания — и Ворон примет вызов.")};
	static const TCHAR* Chapter4[] = {
		TEXT("Ворон|Ладно, курьер. Дуэль: до самой высокой крыши квартала. Жду тебя, если не струсишь.")};
	static const TCHAR* Ending[] = {
		TEXT("Ворон|…Обогнал. Меня. На моём собственном пути. Ну что ж, Лига крыш теперь и твоя, Адриен."),
		TEXT("Мэтр Огюстен|Браво! Весь квартал хочет попасть на мои холсты. Собирайте портреты дальше: гонки можно проходить снова, ради лучших медалей."),
		TEXT("Тень|А для тех, кто не боится темноты и стражи, у меня есть ещё два забега. Найдёшь меня — ты знаешь где."),
		TEXT("Конец главной истории|Спасибо за игру! Париж открыт: портреты, медали и крыши ждут.")};

	template <int32 N>
	static TArray<FString> Lines(const TCHAR* (&Src)[N])
	{
		TArray<FString> Out;
		for (int32 I = 0; I < N; ++I) Out.Add(Src[I]);
		return Out;
	}

	constexpr float GateLift = 110.f;   // центр кольца над поверхностью
}

/* ======================= жизненный цикл ======================= */

void UTPStorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Medals.Init(0, NumRaces);
	BestTimes.Init(0.f, NumRaces);
	Album.Init(0, TPCitizens::Num);
	Courses.SetNum(NumRaces);
}

void UTPStorySubsystem::Deinitialize()
{
	if (bStarted && bDirty) SaveProgress();
	if (ATPCharacter* H = Hero.Get()) H->OnInteractPressed.RemoveDynamic(this, &UTPStorySubsystem::HandleInteract);
	Hero = nullptr;
	Super::Deinitialize();
}

bool UTPStorySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UTPStorySubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTPStorySubsystem, STATGROUP_Tickables);
}

void UTPStorySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	bStarted = true;
	LoadProgress();
	// генератор города заполняет точки прогулок в своём BeginPlay — жителей создаём чуть позже
	InWorld.GetTimerManager().SetTimer(SpawnTimer, FTimerDelegate::CreateUObject(this, &UTPStorySubsystem::SpawnCitizens), 0.75f, false);
}

void UTPStorySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bStarted) return;
	UWorld* W = GetWorld();
	if (!W) return;

	// герой может появиться (или смениться) позже подсистемы
	ATPCharacter* H = Cast<ATPCharacter>(UGameplayStatics::GetPlayerPawn(W, 0));
	if (H != Hero.Get()) BindHero(H);

	if (bPendingPrologue) StartPrologue();
	UpdateRace(DeltaTime);
	UpdateCrowd(DeltaTime);

	if (ATPEnvironment* E = GetEnvironment()) CurrentTimeOfDay = bNightSet ? PrevTimeOfDay : E->TimeOfDay;
	AutoSaveTimer += DeltaTime;
	if (bDirty && AutoSaveTimer > 30.f && !IsRaceActive()) SaveProgress();
}

void UTPStorySubsystem::UpdateCrowd(float Dt)
{
	CrowdTimer -= Dt;
	if (CrowdTimer > 0.f) return;
	CrowdTimer = 0.2f;
	bHeroInCrowd = false;
	const ATPCharacter* H = Hero.Get();
	if (!H || IsRaceActive()) return;
	// в толпе — значит шагом и рядом хотя бы двое горожан
	if (H->GetVelocity().Size2D() > 320.f || (H->Combat && H->Combat->IsInCombat())) return;
	const UCharacterMovementComponent* M = H->GetCharacterMovement();
	if (M && !M->IsMovingOnGround()) return;
	const FVector HL = H->GetActorLocation();
	int32 Near = 0;
	for (const TWeakObjectPtr<ATPCitizen>& W : Citizens)
	{
		const ATPCitizen* C = W.Get();
		if (!C || C->IsDown() || C->IsRacing() || C->IsFleeing()) continue;
		const FVector D = C->GetActorLocation() - HL;
		if (D.Size2D() < 300.f && FMath::Abs(D.Z) < 150.f && ++Near >= 2) break;
	}
	bHeroInCrowd = Near >= 2;
#if TP_STORY_HAS_DIRECTOR
	if (bHeroInCrowd)
		if (UWorld* W = GetWorld())
			if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
				Dir->SetHeroConcealment(0.35f, 0.3f);
#endif
}

/* ======================= сохранение ======================= */

void UTPStorySubsystem::LoadProgress()
{
	const FString Slot = UTPSaveGame::SlotName();
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0)) return;
	const UTPSaveGame* S = Cast<UTPSaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!S) return;
	Chapter = FMath::Clamp(S->Chapter, 0, 5);
	bEnding = S->bEnding;
	Abilities = S->Abilities;
	for (int32 I = 0; I < NumRaces; ++I)
	{
		if (S->Medals.IsValidIndex(I)) Medals[I] = FMath::Clamp(S->Medals[I], 0, 3);
		if (S->BestTimes.IsValidIndex(I)) BestTimes[I] = FMath::Max(0.f, S->BestTimes[I]);
	}
	for (int32 I = 0; I < Album.Num(); ++I)
		if (S->Album.IsValidIndex(I)) Album[I] = S->Album[I] <= 2 ? S->Album[I] : (uint8)0;
	SavedTimeOfDay = S->TimeOfDay;
	UE_LOG(LogTeni, Log, TEXT("Загружено сохранение: глава %d, портретов %d"), Chapter, CountAlbum(2));
}

void UTPStorySubsystem::SaveProgress()
{
	UTPSaveGame* S = Cast<UTPSaveGame>(UGameplayStatics::CreateSaveGameObject(UTPSaveGame::StaticClass()));
	if (!S) return;
	S->Version = UTPSaveGame::CurrentVersion;
	S->Chapter = Chapter;
	S->bEnding = bEnding;
	S->Abilities = Abilities;
	S->Medals = Medals;
	S->BestTimes = BestTimes;
	S->Album = Album;
	S->TimeOfDay = CurrentTimeOfDay >= 0.f ? CurrentTimeOfDay : (SavedTimeOfDay >= 0.f ? SavedTimeOfDay : 9.5f);
	if (UGameplayStatics::SaveGameToSlot(S, UTPSaveGame::SlotName(), 0)) bDirty = false;
	AutoSaveTimer = 0.f;
}

/* ======================= мир и жители ======================= */

const UTPIdentitySubsystem* UTPStorySubsystem::GetIdentities() const
{
	const UWorld* W = GetWorld();
	const UGameInstance* GI = W ? W->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UTPIdentitySubsystem>() : nullptr;
}

ATPHUD* UTPStorySubsystem::GetHUD() const
{
	if (const UWorld* W = GetWorld())
		if (APlayerController* PC = W->GetFirstPlayerController())
			return Cast<ATPHUD>(PC->GetHUD());
	return nullptr;
}

ATPEnvironment* UTPStorySubsystem::GetEnvironment() const
{
	if (ATPEnvironment* E = EnvCache.Get()) return E;
	if (UWorld* W = GetWorld())
		for (TActorIterator<ATPEnvironment> It(W); It; ++It) { EnvCache = *It; return *It; }
	return nullptr;
}

ATPCitizen* UTPStorySubsystem::CitizenFor(int32 IdentityIndex) const
{
	if (const TWeakObjectPtr<ATPCitizen>* P = ByIdentity.Find(IdentityIndex)) return P->Get();
	return nullptr;
}

bool UTPStorySubsystem::TraceRange(const FVector& From, const FVector& To, FVector& OutHit, FVector& OutNormal) const
{
	UWorld* W = GetWorld();
	if (!W) return false;
	FCollisionObjectQueryParams Obj;
	Obj.AddObjectTypesToQuery(ECC_WorldStatic);
	Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TPStoryTrace), false);
	FHitResult Hit;
	if (!W->LineTraceSingleByObjectType(Hit, From, To, Obj, Q)) return false;
	OutHit = Hit.ImpactPoint;
	OutNormal = Hit.ImpactNormal;
	return true;
}

bool UTPStorySubsystem::TraceDown(const FVector& XY, FVector& OutHit, FVector& OutNormal) const
{
	const float BaseZ = CityBounds.IsValid ? CityBounds.GetCenter().Z : XY.Z;
	return TraceRange(FVector(XY.X, XY.Y, BaseZ + 20000.f), FVector(XY.X, XY.Y, BaseZ - 5000.f), OutHit, OutNormal);
}

void UTPStorySubsystem::SpawnCitizens()
{
	UWorld* W = GetWorld();
	if (!W || bSpawned) return;
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!Ids || Ids->Num() == 0) return;

	ATPCityGenerator* City = nullptr;
	for (TActorIterator<ATPCityGenerator> It(W); It; ++It) { City = *It; break; }
	WalkPoints.Reset();
	if (City) { WalkPoints = City->GetWalkPoints(); PlayerStart = City->GetPlayerStart(); }
	else if (const APawn* P = UGameplayStatics::GetPlayerPawn(W, 0)) PlayerStart = P->GetActorLocation();

	if (WalkPoints.Num() < 4 && ++SpawnTries < 8)
	{
		// город ещё не готов — попробуем ещё раз
		W->GetTimerManager().SetTimer(SpawnTimer, FTimerDelegate::CreateUObject(this, &UTPStorySubsystem::SpawnCitizens), 0.5f, false);
		return;
	}
	if (WalkPoints.Num() < 4)
	{
		UE_LOG(LogTeni, Warning, TEXT("Нет точек прогулок от генератора города — жители гуляют вокруг старта"));
		for (int32 I = 0; I < 24; ++I)
		{
			const float A = 2.f * UE_PI * I / 24.f, R = 600.f + (I % 3) * 700.f;
			WalkPoints.Add(PlayerStart + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f));
		}
	}
	bSpawned = true;
	CityBounds = FBox(WalkPoints).ExpandBy(FVector(1500.f, 1500.f, 0.f));

	// время суток из сохранения
	if (SavedTimeOfDay >= 0.f)
		if (ATPEnvironment* E = GetEnvironment()) { E->TimeOfDay = SavedTimeOfDay; E->AddHours(0.f); }

	TArray<FVector> Sorted = WalkPoints;
	const FVector PS = PlayerStart;
	Sorted.Sort([PS](const FVector& A, const FVector& B) { return FVector::DistSquared2D(A, PS) < FVector::DistSquared2D(B, PS); });

	auto SpawnOne = [this, W](int32 IdIndex, const FVector& At, float Yaw, bool bStationary) -> ATPCitizen*
	{
		if (IdIndex < 0 || ByIdentity.Contains(IdIndex)) return nullptr;
		FVector Loc = At, Hit, N;
		if (TraceRange(At + FVector(0.f, 0.f, 250.f), At - FVector(0.f, 0.f, 1500.f), Hit, N)) Loc = Hit;
		Loc.Z += 100.f;
		const FTransform T(FRotator(0.f, Yaw, 0.f), Loc);
		ATPCitizen* C = W->SpawnActorDeferred<ATPCitizen>(ATPCitizen::StaticClass(), T, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!C) return nullptr;
		C->InitCitizen(IdIndex, WalkPoints, bStationary);
		C->FinishSpawning(T);
		Citizens.Add(C);
		ByIdentity.Add(IdIndex, C);
		return C;
	};
	auto YawTo = [](const FVector& From, const FVector& To) { return (To - From).Rotation().Yaw; };

	// художник — рядом со стартом героя (с ним начинается пролог)
	{
		const FVector At = PlayerStart + FVector(260.f, 160.f, 0.f);
		SpawnOne(Ids->GetPainter(), At, YawTo(At, PlayerStart), true);
	}
	// бегуны Лиги — всё дальше от старта: Жак ближе всех, Ворон дальше всех
	const int32 NS = Sorted.Num();
	for (int32 R = 0; R < TPCitizens::NumRunners; ++R)
	{
		const int32 K = FMath::Clamp(NS * (R + 1) / (TPCitizens::NumRunners + 2), 0, NS - 1);
		SpawnOne(Ids->GetRunner(R), Sorted[K], YawTo(Sorted[K], PlayerStart), true);
	}
	// остальные — гуляют (солдат не берём: стражей создаёт модуль боя)
	FRandomStream Rng(1789);
	TArray<int32> Pool;
	for (int32 I = 0; I < Ids->Num(); ++I)
	{
		const FTPIdentity& D = Ids->Get(I);
		if (D.Class == ETPSocialClass::Soldier || D.Role == ETPCitizenRole::Painter || D.Role == ETPCitizenRole::Runner) continue;
		Pool.Add(I);
	}
	for (int32 I = Pool.Num() - 1; I > 0; --I) Pool.Swap(I, Rng.RandRange(0, I));
	const int32 Thief = Ids->GetThief();
	if (Thief >= 0) { Pool.Remove(Thief); Pool.Insert(Thief, 0); }
	const int32 Want = 44;
	for (int32 I = 0; I < Pool.Num() && I < Want; ++I)
	{
		const FVector& P = WalkPoints[Rng.RandRange(0, WalkPoints.Num() - 1)];
		const FVector At = P + FVector(Rng.FRandRange(-150.f, 150.f), Rng.FRandRange(-150.f, 150.f), 0.f);
		SpawnOne(Pool[I], At, Rng.FRandRange(-180.f, 180.f), false);
	}
	UE_LOG(LogTeni, Log, TEXT("В квартале появилось жителей: %d"), Citizens.Num());

	if (Chapter == 0) bPendingPrologue = true;
}

void UTPStorySubsystem::BindHero(ATPCharacter* NewHero)
{
	if (ATPCharacter* Old = Hero.Get()) Old->OnInteractPressed.RemoveDynamic(this, &UTPStorySubsystem::HandleInteract);
	Hero = NewHero;
	BaseSprintSpeed = BaseClimbSpeed = -1.f;
	if (!NewHero) return;
	NewHero->OnInteractPressed.AddUniqueDynamic(this, &UTPStorySubsystem::HandleInteract);
	if (NewHero->Traversal)
	{
		BaseSprintSpeed = NewHero->Traversal->SprintSpeed;
		BaseClimbSpeed = NewHero->Traversal->ClimbSpeed;
	}
	ApplyAbilities();
}

void UTPStorySubsystem::ApplyAbilities()
{
	ATPCharacter* H = Hero.Get();
	if (!H || !H->Traversal) return;
	auto Has = [this](int32 A) { return (Abilities & (1 << A)) != 0; };
	UTPTraversalComponent* T = H->Traversal;
	// лазание доступно всегда; остальное открывается наградами Лиги
	T->bAutoClimb = true;
	T->bLedgeGrab = Has(0);
	T->bRoll = Has(1);
	T->bAutoLeap = Has(2);
	T->bAutoVault = Has(3);
	if (BaseSprintSpeed > 0.f) T->SprintSpeed = BaseSprintSpeed * (1.f + (Has(4) ? 0.06f : 0.f) + (Has(5) ? 0.06f : 0.f));
	if (BaseClimbSpeed > 0.f) T->ClimbSpeed = BaseClimbSpeed * (Has(5) ? 1.25f : 1.f);
}

/* ======================= сюжет ======================= */

FString UTPStorySubsystem::GetChapterTitle() const
{
	return TPStoryData::ChapterNames[FMath::Clamp(Chapter, 0, 5)];
}

FString UTPStorySubsystem::GetObjective() const
{
	if (Chapter == 0) return TEXT("Поговорите с Мэтром Огюстеном (G)");
	if (Chapter <= 1 && Medals[0] == 0) return TEXT("Найдите Жака-Трубочиста (золотой ромб на карте) и пройдите первую гонку");
	if (Chapter == 4 && Medals[9] == 0) return TEXT("Финал: вызовите Ворона на дуэль");
	if (Chapter >= 5) return TEXT("Вольная игра: медали, портреты и ночные забеги Тени");
	return TEXT("Бегуны Лиги отмечены золотыми ромбами. Пройдите гонки главы. Портреты — в альбоме (Tab)");
}

int32 UTPStorySubsystem::CountAlbum(int32 MinStatus) const
{
	int32 N = 0;
	for (const uint8 V : Album) if (V >= MinStatus) ++N;
	return N;
}

void UTPStorySubsystem::StartPrologue()
{
	ATPHUD* HUD = GetHUD();
	if (!HUD) { bPendingPrologue = true; return; }
	bPendingPrologue = false;
	if (Chapter != 0) return;
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (Ids && Album.IsValidIndex(Ids->GetPainter())) Album[Ids->GetPainter()] = FMath::Max<uint8>(Album[Ids->GetPainter()], 1);
	TWeakObjectPtr<UTPStorySubsystem> Weak(this);
	HUD->ShowDialogThen(TPStoryData::Lines(TPStoryData::Prologue), [Weak]()
	{
		UTPStorySubsystem* S = Weak.Get();
		if (!S) return;
		S->Chapter = 1;
		if (const UTPIdentitySubsystem* I = S->GetIdentities())
		{
			if (S->Album.IsValidIndex(I->GetPainter())) S->Album[I->GetPainter()] = 2;
			if (S->Album.IsValidIndex(I->GetRunner(0))) S->Album[I->GetRunner(0)] = 2;
		}
		if (ATPHUD* H = S->GetHUD()) H->Toast(TEXT("В альбоме 2 портрета: Мэтр Огюстен и Жак-Трубочист (Tab)"), 5.f);
		S->SaveProgress();
	});
}

bool UTPStorySubsystem::ChapterDone(int32 InChapter) const
{
	for (int32 R = 0; R < NumRaces; ++R)
		if (TPStoryData::Races[R].Chapter == InChapter && Medals[R] == 0) return false;
	return true;
}

bool UTPStorySubsystem::RaceOpen(int32 Race) const
{
	return Race >= 0 && Race < NumRaces && TPStoryData::Races[Race].Chapter <= Chapter;
}

int32 UTPStorySubsystem::PoolNext() const
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!Ids) return -1;
	TArray<int32> Cand;
	for (int32 I = 0; I < Album.Num(); ++I)
	{
		if (Album[I] == 2) continue;
		const FTPIdentity& D = Ids->Get(I);
		if (D.Role != ETPCitizenRole::None) continue;   // художник, бегуны и карманник появляются по сюжету
		Cand.Add(I);
	}
	return Cand.Num() ? Cand[FMath::RandRange(0, Cand.Num() - 1)] : -1;
}

bool UTPStorySubsystem::AwardPortrait(int32 Index, TArray<int32>& OutNew)
{
	if (!Album.IsValidIndex(Index) || Album[Index] == 2) return false;
	Album[Index] = 2;
	OutNew.Add(Index);
	return true;
}

void UTPStorySubsystem::ChapterCheck()
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	ATPHUD* HUD = GetHUD();
	if (!Ids || !HUD) return;
	auto Paint = [this](int32 I) { if (Album.IsValidIndex(I)) Album[I] = 2; };
	TWeakObjectPtr<UTPStorySubsystem> Weak(this);
	auto Done = [Weak]() { if (UTPStorySubsystem* S = Weak.Get()) S->SaveProgress(); };
	using namespace TPStoryData;
	if (Chapter == 1 && ChapterDone(1))
	{
		Chapter = 2; Paint(Ids->GetRunner(1)); Paint(Ids->GetRunner(2));
		HUD->Toast(TEXT("Глава 2. Новые портреты: Лиза и Эмиль"), 5.f);
		HUD->ShowDialogThen(Lines(Chapter2), Done);
	}
	else if (Chapter == 2 && ChapterDone(2))
	{
		Chapter = 3; Paint(Ids->GetRunner(3)); Paint(Ids->GetThief());
		HUD->Toast(TEXT("Глава 3. Новые портреты: Тень и карманник"), 5.f);
		HUD->ShowDialogThen(Lines(Chapter3), Done);
	}
	else if (Chapter == 3 && ChapterDone(3))
	{
		Chapter = 4; Paint(PoolNext());
		HUD->Toast(TEXT("Финал открыт. Новый портрет в альбоме"), 5.f);
		HUD->ShowDialogThen(Lines(Chapter4), Done);
	}
	else if (Chapter == 4 && Medals[9] > 0)
	{
		Chapter = 5; bEnding = true; Paint(Ids->GetRunner(4));
		HUD->ShowDialogThen(Lines(Ending), Done);
	}
	bDirty = true;
	SaveProgress();
}

/* ======================= разговоры ======================= */

void UTPStorySubsystem::HandleInteract()
{
	ATPHUD* HUD = GetHUD();
	if (HUD && HUD->IsBlockingUI()) return;
	if (IsRaceActive())
	{
		if (HUD) HUD->Toast(TEXT("Идёт гонка! Esc или P — пауза и выход из гонки"), 2.5f);
		return;
	}
	ATPCharacter* H = Hero.Get();
	if (!H) return;
	ATPCitizen* C = FindTalkTarget(H);
	if (!C) return;
	if (H->Combat && H->Combat->IsInCombat())
	{
		if (HUD) HUD->Toast(TEXT("Не до разговоров — рядом стража!"), 2.f);
		return;
	}
	Talk(C);
}

ATPCitizen* UTPStorySubsystem::FindTalkTarget(const AActor* InHero, float Radius) const
{
	if (!InHero) return nullptr;
	const FVector HL = InHero->GetActorLocation();
	const FVector F = InHero->GetActorForwardVector();
	ATPCitizen* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<ATPCitizen>& W : Citizens)
	{
		ATPCitizen* C = W.Get();
		if (!C || C->IsDown() || C->IsRacing()) continue;
		FVector D = C->GetActorLocation() - HL;
		if (FMath::Abs(D.Z) > 160.f) continue;
		D.Z = 0.f;
		const float Dist = D.Size();
		if (Dist > Radius) continue;
		const float Score = Dist - 60.f * FVector::DotProduct(F, D.GetSafeNormal());
		if (Score < BestScore) { BestScore = Score; Best = C; }
	}
	return Best;
}

bool UTPStorySubsystem::IsRunner(const ATPCitizen* C) const
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	return C && Ids && Ids->Get(C->GetIdentityIndex()).Role == ETPCitizenRole::Runner;
}

bool UTPStorySubsystem::IsPainter(const ATPCitizen* C) const
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	return C && Ids && Ids->Get(C->GetIdentityIndex()).Role == ETPCitizenRole::Painter;
}

bool UTPStorySubsystem::RunnerHasNews(const ATPCitizen* C) const
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!C || !Ids) return false;
	const FTPIdentity& D = Ids->Get(C->GetIdentityIndex());
	if (D.Role != ETPCitizenRole::Runner) return false;
	for (int32 R = 0; R < NumRaces; ++R)
		if (TPStoryData::Races[R].Giver == D.RunnerIndex && RaceOpen(R) && Medals[R] == 0) return true;
	return false;
}

FString UTPStorySubsystem::GetTalkPrompt(const ATPCitizen* C) const
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!C || !Ids) return FString();
	const int32 I = C->GetIdentityIndex();
	const FTPIdentity& D = Ids->Get(I);
	switch (D.Role)
	{
	case ETPCitizenRole::Runner: return FString::Printf(TEXT("G — гонки Лиги: %s «%s»"), *D.Name, *D.Nick);
	case ETPCitizenRole::Painter: return TEXT("G — Мэтр Огюстен Брюн, художник");
	default: break;
	}
	if (GetAlbumStatus(I) > 0) return FString::Printf(TEXT("G — поговорить: %s"), *D.Name);
	return D.bFemale ? TEXT("G — поговорить: незнакомка") : TEXT("G — поговорить: незнакомец");
}

void UTPStorySubsystem::Talk(ATPCitizen* C)
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	ATPHUD* HUD = GetHUD();
	if (!C || !Ids || !HUD) return;
	const int32 I = C->GetIdentityIndex();
	if (!Album.IsValidIndex(I)) return;
	const FTPIdentity& D = Ids->Get(I);
	const bool bFresh = Album[I] == 0;
	if (bFresh) { Album[I] = 1; bDirty = true; }
	C->OnTalkedTo(Hero.Get());

	TWeakObjectPtr<UTPStorySubsystem> Weak(this);
	auto SaveAfter = [Weak]() { if (UTPStorySubsystem* S = Weak.Get()) if (S->bDirty) S->SaveProgress(); };
	if (bFresh && D.Role != ETPCitizenRole::Painter) HUD->Toast(FString::Printf(TEXT("Новое знакомство: %s, %s"), *D.Name, *D.Job));

	if (D.Role == ETPCitizenRole::Runner)
	{
		static const TCHAR* Hello[5] = {TEXT("Новичок! Побежали?"), TEXT("Опять ты? Ну давай."), TEXT("По моим расчётам, ты готов."), TEXT("…"), TEXT("Курьер. Хм.")};
		C->Say(Hello[FMath::Clamp(D.RunnerIndex, 0, 4)], 2.f);
		OpenRaceMenu(D.RunnerIndex);
		return;
	}
	TArray<FString> L;
	if (D.Role == ETPCitizenRole::Painter)
	{
		L.Add(FString::Printf(TEXT("Мэтр Огюстен|Портретов написано: %d из %d. Знакомых у вас в квартале: %d. Побеждайте в Лиге — и холсты не будут пустовать!"),
			CountAlbum(2), TPCitizens::Num, CountAlbum(1)));
		HUD->ShowDialogThen(L, SaveAfter);
		return;
	}
	++TalkSalt;
	const FString Rumor = D.Role == ETPCitizenRole::Thief ? FString(TEXT("Чего уставился? Иди своей дорогой.")) : Ids->GetRumor(I, TalkSalt * 31 + I);
	if (bFresh)
	{
		L.Add(FString::Printf(TEXT("%s|Bonjour! Меня зовут %s. Я %s."), *D.Name, *D.Name, *D.Job));
		L.Add(FString::Printf(TEXT("%s · %d лет · %s|%s"), *D.Name, D.Age, *UTPIdentitySubsystem::ClassName(D.Class), *D.Bio));
	}
	L.Add(FString::Printf(TEXT("%s|%s"), *D.Name, *Rumor));
	HUD->ShowDialogThen(L, SaveAfter);
}

void UTPStorySubsystem::OpenRaceMenu(int32 Giver)
{
	const UTPIdentitySubsystem* Ids = GetIdentities();
	ATPHUD* HUD = GetHUD();
	if (!Ids || !HUD) return;
	using namespace TPStoryData;
	TArray<int32> List;
	TArray<FString> Items, Details;
	TArray<bool> Enabled;
	for (int32 R = 0; R < NumRaces; ++R)
	{
		if (Races[R].Giver != Giver) continue;
		List.Add(R);
		if (RaceOpen(R))
		{
			Items.Add(FString::Printf(TEXT("%s — %s"), Races[R].Title, *RaceTypeName(Races[R].Type)));
			if (Medals[R] > 0)
				Details.Add(FString::Printf(TEXT("медаль: %s · лучшее время %s"), *MedalName(Medals[R]), *FormatTime(BestTimes[R])));
			else if (Races[R].Reward >= 0)
				Details.Add(FString::Printf(TEXT("не пройдена · награда: способность «%s»"), AbilityNames[Races[R].Reward]));
			else Details.Add(TEXT("не пройдена"));
			Enabled.Add(true);
		}
		else
		{
			const int32 Ch = Races[R].Chapter;
			Items.Add(FString::Printf(TEXT("%s — откроется %s"), Races[R].Title, Ch >= 5 ? TEXT("после финала") : Ch == 4 ? TEXT("в финале") : TEXT("в следующей главе")));
			Details.Add(FString());
			Enabled.Add(false);
		}
	}
	const FTPIdentity& G = Ids->Get(Ids->GetRunner(Giver));
	const FString Title = FString::Printf(TEXT("%s «%s» · гонки Лиги крыш"), *G.Name, *G.Nick);
	TWeakObjectPtr<UTPStorySubsystem> Weak(this);
	HUD->ShowChoice(Title, Items, Details, Enabled, [Weak, List](int32 Idx)
	{
		UTPStorySubsystem* S = Weak.Get();
		if (!S || !List.IsValidIndex(Idx)) return;
		const int32 Race = List[Idx];
		if (!S->RaceOpen(Race)) return;
		ATPHUD* H = S->GetHUD();
		if (!H) { S->BeginRace(Race); return; }
		TArray<FString> Intro;
		Intro.Add(TPStoryData::Races[Race].Intro);
		H->ShowDialogThen(Intro, [Weak, Race]() { if (UTPStorySubsystem* S2 = Weak.Get()) S2->BeginRace(Race); });
	});
}

/* ======================= трассы ======================= */

bool UTPStorySubsystem::PickStreetPoint(const FVector& From, const FVector& Heading, const TArray<FVector>& Used, FRandomStream& Rng, FVector& Out) const
{
	int32 BestI = -1;
	float BestScore = -1e9f;
	for (int32 Pass = 0; Pass < 2 && BestI < 0; ++Pass)
	{
		const float MinD = Pass == 0 ? 1200.f : 500.f, MaxD = Pass == 0 ? 4200.f : 8000.f;
		for (int32 I = 0; I < WalkPoints.Num(); ++I)
		{
			const FVector& P = WalkPoints[I];
			const float D = FVector::Dist2D(From, P);
			if (D < MinD || D > MaxD) continue;
			bool bNearUsed = false;
			if (Pass == 0) for (const FVector& U : Used) if (FVector::Dist2D(U, P) < 900.f) { bNearUsed = true; break; }
			if (bNearUsed) continue;
			const FVector Dir = (P - From).GetSafeNormal2D();
			const float Score = FVector::DotProduct(Dir, Heading) * 1.2f - FMath::Abs(D - 2400.f) / 1600.f + Rng.FRand() * 0.5f;
			if (Score > BestScore) { BestScore = Score; BestI = I; }
		}
	}
	if (BestI < 0) return false;
	FVector Hit, N;
	const FVector& P = WalkPoints[BestI];
	Out = TraceRange(P + FVector(0.f, 0.f, 200.f), P - FVector(0.f, 0.f, 1500.f), Hit, N) ? Hit : P;
	return true;
}

bool UTPStorySubsystem::PickRoofPoint(const FVector& From, const FVector& Heading, bool bFromRoof, float StreetZ, FRandomStream& Rng, FVector& Out) const
{
	const float MinD = bFromRoof ? 600.f : 900.f, MaxD = bFromRoof ? 1700.f : 2600.f;
	float BestScore = -1e9f;
	bool bFound = false;
	for (int32 I = 0; I < 72; ++I)
	{
		const float Ang = Rng.FRandRange(-110.f, 110.f);
		const FVector Dir = Heading.RotateAngleAxis(Ang, FVector::UpVector);
		const float D = Rng.FRandRange(MinD, MaxD);
		const FVector XY = From + Dir * D;
		if (CityBounds.IsValid && !CityBounds.IsInsideXY(XY)) continue;
		FVector Hit, N;
		if (!TraceDown(XY, Hit, N) || N.Z < 0.75f || Hit.Z - StreetZ < 350.f) continue;
		// на крыше должно быть где встать: соседние точки примерно той же высоты
		int32 Flat = 0;
		for (int32 K = 0; K < 4; ++K)
		{
			const FVector O = XY + FVector(K == 0 ? 90.f : K == 1 ? -90.f : 0.f, K == 2 ? 90.f : K == 3 ? -90.f : 0.f, 0.f);
			FVector H2, N2;
			if (TraceDown(O, H2, N2) && FMath::Abs(H2.Z - Hit.Z) < 40.f) ++Flat;
		}
		if (Flat < 2) continue;
		const float Dz = FMath::Abs(Hit.Z - From.Z);
		const float Score = FMath::Cos(FMath::DegreesToRadians(Ang)) - FMath::Abs(D - (MinD + MaxD) * 0.5f) / MaxD - (bFromRoof ? Dz / 600.f : 0.f) + Rng.FRand() * 0.3f;
		if (Score > BestScore) { BestScore = Score; Out = Hit; bFound = true; }
	}
	return bFound;
}

bool UTPStorySubsystem::PickSummit(const FVector& From, FRandomStream& Rng, FVector& Out) const
{
	bool bFound = false;
	float BestZ = -1e9f;
	for (int32 I = 0; I < 160; ++I)
	{
		const float A = Rng.FRandRange(0.f, 2.f * UE_PI);
		const float R = 6000.f * FMath::Sqrt(Rng.FRand());
		const FVector XY = From + FVector(FMath::Cos(A) * R, FMath::Sin(A) * R, 0.f);
		if (CityBounds.IsValid && !CityBounds.IsInsideXY(XY)) continue;
		FVector Hit, N;
		if (!TraceDown(XY, Hit, N) || N.Z < 0.5f) continue;
		if (Hit.Z > BestZ) { BestZ = Hit.Z; Out = Hit; bFound = true; }
	}
	return bFound;
}

void UTPStorySubsystem::BuildCourse(int32 Race)
{
	using namespace TPStoryData;
	const FRaceDef& Def = Races[Race];
	FTPRaceCourse& C = Courses[Race];
	C = FTPRaceCourse();
	FRandomStream Rng(1789 + Race * 977);
	const UTPIdentitySubsystem* Ids = GetIdentities();

	// старт — рядом с бегуном, который предлагает гонку
	FVector Start = PlayerStart;
	float GiverYaw = 0.f;
	if (Ids)
		if (ATPCitizen* G = CitizenFor(Ids->GetRunner(Def.Giver))) { Start = G->HomeLocation; GiverYaw = G->HomeYaw; }
	Start += FRotator(0.f, GiverYaw, 0.f).Vector() * 220.f;
	FVector Hit, N;
	if (TraceRange(Start + FVector(0.f, 0.f, 150.f), Start - FVector(0.f, 0.f, 1500.f), Hit, N)) Start = Hit;
	else Start.Z -= 90.f;
	C.Start = Start;
	C.StreetZ = Start.Z;

	FVector Heading = CityBounds.IsValid ? (CityBounds.GetCenter() - Start).GetSafeNormal2D() : FVector::ForwardVector;
	if (Heading.IsNearlyZero()) Heading = FVector::ForwardVector;
	Heading = Heading.RotateAngleAxis(Rng.FRandRange(-60.f, 60.f), FVector::UpVector);

	FVector Prev = Start;
	bool bPrevHigh = false;
	TArray<FVector> Used;
	for (const TCHAR* K = Def.Gates; *K; ++K)
	{
		FVector P;
		TCHAR Kind = *K;
		bool bOk = false;
		if (Kind == TEXT('R')) bOk = PickRoofPoint(Prev, Heading, bPrevHigh, C.StreetZ, Rng, P);
		else if (Kind == TEXT('T')) bOk = PickSummit(Prev, Rng, P);
		if (!bOk) { Kind = TEXT('G'); bOk = PickStreetPoint(Prev, Heading, Used, Rng, P); }
		if (!bOk) continue;
		C.Gates.Add(P + FVector(0.f, 0.f, GateLift));
		C.Kinds.Add(Kind);
		Used.Add(P);
		const FVector Dir = (P - Prev).GetSafeNormal2D();
		if (!Dir.IsNearlyZero()) Heading = Dir;
		Prev = P;
		bPrevHigh = Kind != TEXT('G');
	}
	if (C.Gates.Num() == 0) return;

	// охота за флажками: порядок «ближайший следующий» — так бегут люди
	if (Def.Type == ETPRaceType::Hunt)
	{
		TArray<FVector> G = C.Gates, Ordered;
		TArray<TCHAR> Kd = C.Kinds, KOrdered;
		FVector Cur = Start + FVector(0.f, 0.f, GateLift);
		while (G.Num())
		{
			int32 Bi = 0;
			for (int32 I = 1; I < G.Num(); ++I) if (FVector::DistSquared(G[I], Cur) < FVector::DistSquared(G[Bi], Cur)) Bi = I;
			Cur = G[Bi];
			Ordered.Add(G[Bi]); KOrdered.Add(Kd[Bi]);
			G.RemoveAt(Bi); Kd.RemoveAt(Bi);
		}
		C.Gates = Ordered; C.Kinds = KOrdered;
	}
	{
		FVector ToFirst = C.Gates[0] - Start;
		ToFirst.Z = 0.f;
		C.StartYaw = ToFirst.IsNearlyZero() ? 0.f : ToFirst.Rotation().Yaw;
	}

	// эталонное время: бег ~5.6 м/с, лазание ~1.5 м/с, спуск быстрый, по 0.6 с на ворота
	float T = 2.f;
	FVector P = Start;
	for (const FVector& G : C.Gates)
	{
		const FVector F = G - FVector(0.f, 0.f, GateLift);
		const float Dz = F.Z - P.Z;
		T += FVector::Dist2D(P, F) / 560.f + FMath::Max(0.f, Dz) / 150.f + FMath::Max(0.f, -Dz) / 1200.f + 0.6f;
		P = F;
	}
	const bool bLong = Def.Type == ETPRaceType::Sprint || Def.Type == ETPRaceType::Roofs;
	C.Ref = T;
	C.Gold = T * (bLong ? 1.25f : 1.18f);
	C.Silver = T * (bLong ? 1.6f : 1.5f);
	C.Bronze = T * (Def.Type == ETPRaceType::Hunt ? 2.3f : bLong ? 2.2f : 2.0f);
	C.Limit = (Def.Type == ETPRaceType::Rival || Def.Type == ETPRaceType::Thief) ? 1e6f : C.Bronze;
	C.bBuilt = true;
	UE_LOG(LogTeni, Log, TEXT("Трасса «%s»: ворот %d, эталон %.1f с"), Def.Title, C.Gates.Num(), C.Ref);
}

void UTPStorySubsystem::BuildRunnerPath(float StartAhead)
{
	RunnerPath.Reset();
	RunnerPathGate.Reset();
	const FTPRaceCourse& C = Courses[CurRace];
	// соперник стартует рядом с героем, чуть правее
	FVector Cur = C.Start + FVector(0.f, 0.f, 95.f) + FRotator(0.f, C.StartYaw, 0.f).RotateVector(FVector(0.f, 140.f, 0.f));
	RunnerPath.Add(Cur); RunnerPathGate.Add(-1);
	for (int32 I = 0; I < C.Gates.Num(); ++I)
	{
		const FVector G = C.Gates[I];
		const FVector Dir = (G - Cur).GetSafeNormal2D();
		const float Dz = G.Z - Cur.Z;
		if (Dz > 200.f)
		{
			// подбежать к стене и вскарабкаться
			FVector Wall = G - Dir * 120.f; Wall.Z = Cur.Z;
			RunnerPath.Add(Wall); RunnerPathGate.Add(-1);
			FVector Up = Wall; Up.Z = G.Z + 40.f;
			RunnerPath.Add(Up); RunnerPathGate.Add(-1);
		}
		else if (Dz < -200.f)
		{
			// прыжок вниз с края
			FVector Edge = Cur + Dir * FMath::Min(250.f, FVector::Dist2D(Cur, G) * 0.3f); Edge.Z = Cur.Z + 60.f;
			RunnerPath.Add(Edge); RunnerPathGate.Add(-1);
		}
		RunnerPath.Add(G); RunnerPathGate.Add(I);
		Cur = G;
	}
	// фора (вор стартует впереди)
	float Left = StartAhead;
	while (Left > 0.f && RunnerPath.Num() > 1)
	{
		const float Seg = FVector::Dist(RunnerPath[0], RunnerPath[1]);
		if (Seg > Left) { RunnerPath[0] += (RunnerPath[1] - RunnerPath[0]) * (Left / Seg); break; }
		Left -= Seg;
		RunnerPath.RemoveAt(0); RunnerPathGate.RemoveAt(0);
	}
}

/* ======================= гонка ======================= */

void UTPStorySubsystem::BeginRace(int32 Race)
{
	using namespace TPStoryData;
	if (IsRaceActive() || Race < 0 || Race >= NumRaces) return;
	ATPCharacter* H = Hero.Get();
	UWorld* W = GetWorld();
	ATPHUD* HUD = GetHUD();
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!H || !W || !Ids) return;
	if (!Courses[Race].bBuilt) BuildCourse(Race);
	const FTPRaceCourse& C = Courses[Race];
	const FRaceDef& Def = Races[Race];
	if (C.Gates.Num() == 0)
	{
		if (HUD) HUD->Toast(TEXT("Не удалось проложить трассу в этом квартале"));
		return;
	}

	CurRace = Race;
	RaceState = ETPRaceState::Countdown;
	Countdown = 3.4f;
	RaceTime = 0.f;
	NextGate = 0;
	FlagsLeft = C.Gates.Num();
	GotFlag.Init(false, C.Gates.Num());
	bRunnerDone = false;
	RunnerWp = 1;
	RunnerGate = 0;

	// отметки
	FActorSpawnParameters P;
	P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 I = 0; I < C.Gates.Num(); ++I)
	{
		const ETPGateKind Kind = Def.Type == ETPRaceType::Hunt ? ETPGateKind::Flag : (C.Kinds[I] == TEXT('T') ? ETPGateKind::Summit : ETPGateKind::Ring);
		const FVector From = I > 0 ? C.Gates[I - 1] : C.Start;
		if (ATPStoryGate* G = W->SpawnActor<ATPStoryGate>(ATPStoryGate::StaticClass(), C.Gates[I], FRotator::ZeroRotator, P))
		{
			G->Setup(Kind, C.Gates[I] - From);
			GateActors.Add(G);
		}
	}
	UpdateGateHighlights();

	// герой — на старт
	if (H->Traversal) H->Traversal->ResetTo(C.Start + FVector(0.f, 0.f, 100.f), C.StartYaw);
	if (APlayerController* PC = Cast<APlayerController>(H->GetController())) PC->SetControlRotation(FRotator(-12.f, C.StartYaw, 0.f));

	// соперник или вор
	if (Def.Type == ETPRaceType::Rival || Def.Type == ETPRaceType::Thief)
	{
		const bool bThief = Def.Type == ETPRaceType::Thief;
		ATPCitizen* R = CitizenFor(bThief ? Ids->GetThief() : Ids->GetRunner(Def.Giver));
		if (R)
		{
			RaceRunner = R;
			BuildRunnerPath(bThief ? 1200.f : 0.f);
			float Len = 0.f;
			for (int32 I = 1; I < RunnerPath.Num(); ++I) Len += FVector::Dist(RunnerPath[I - 1], RunnerPath[I]);
			const float WantTime = bThief ? C.Ref * 0.95f : C.Ref * 1.32f * Def.Difficulty;
			RunnerSpeed = FMath::Clamp(Len / FMath::Max(WantTime, 1.f), 300.f, 950.f);
			RunnerTotal = Len / RunnerSpeed;
			R->SetRacing(true);
			R->SetActorLocation(RunnerPath[0], false, nullptr, ETeleportType::TeleportPhysics);
			if (bThief) R->Say(TEXT("Ха! Попробуй догони!"), 2.5f);
		}
		else if (bThief)
		{
			FinishRace(false, TEXT("Карманника нигде не видно."));
			return;
		}
	}
	if (Def.bNight) SetNight(true);
	if (HUD) HUD->Toast(FString::Printf(TEXT("«%s»"), Def.Title), 3.f);
}

void UTPStorySubsystem::SetNight(bool bOn)
{
	ATPEnvironment* E = GetEnvironment();
	if (!E) return;
	if (bOn && !bNightSet)
	{
		PrevTimeOfDay = E->TimeOfDay;
		E->TimeOfDay = 23.5f;
		E->AddHours(0.f);
		bNightSet = true;
	}
	else if (!bOn && bNightSet)
	{
		E->TimeOfDay = PrevTimeOfDay;
		E->AddHours(0.f);
		bNightSet = false;
	}
}

void UTPStorySubsystem::UpdateGateHighlights()
{
	if (CurRace < 0) return;
	const ETPRaceType Type = TPStoryData::Races[CurRace].Type;
	for (int32 I = 0; I < GateActors.Num(); ++I)
	{
		ATPStoryGate* G = GateActors[I];
		if (!G) continue;
		int32 Level = 0;
		if (Type == ETPRaceType::Hunt) Level = (GotFlag.IsValidIndex(I) && GotFlag[I]) ? 0 : 1;
		else if (Type == ETPRaceType::Thief) Level = 0;
		else Level = I == NextGate ? 1 : I == NextGate + 1 ? 2 : 0;
		G->SetHighlight(Level);
	}
}

bool UTPStorySubsystem::NearGate(const FVector& HeroLoc, const FVector& Gate, bool bTower) const
{
	return FVector::Dist2D(HeroLoc, Gate) < (bTower ? 260.f : 280.f) && FMath::Abs(HeroLoc.Z - Gate.Z) < (bTower ? 240.f : 300.f);
}

void UTPStorySubsystem::StepRunner(float Dt)
{
	ATPCitizen* R = RaceRunner.Get();
	if (!R || RunnerWp >= RunnerPath.Num()) { bRunnerDone = true; return; }
	const FVector T = RunnerPath[RunnerWp];
	if (FVector::Dist(R->GetActorLocation(), T) < FMath::Max(60.f, RunnerSpeed * Dt * 1.5f))
	{
		if (RunnerPathGate[RunnerWp] >= 0) RunnerGate = RunnerPathGate[RunnerWp] + 1;
		++RunnerWp;
		if (RunnerWp >= RunnerPath.Num()) bRunnerDone = true;
		return;
	}
	R->RaceStep(T, RunnerSpeed);
}

void UTPStorySubsystem::UpdateRace(float Dt)
{
	if (RaceState == ETPRaceState::None || CurRace < 0) return;
	ATPCharacter* H = Hero.Get();
	if (!H) { FinishRace(false, TEXT("Гонка прервана")); return; }
	if (RaceState == ETPRaceState::Countdown)
	{
		Countdown -= Dt;
		if (Countdown <= 0.4f) RaceState = ETPRaceState::Running;
		return;
	}
	using namespace TPStoryData;
	const FRaceDef& Def = Races[CurRace];
	const FTPRaceCourse& C = Courses[CurRace];
	ATPHUD* HUD = GetHUD();
	RaceTime += Dt;
	const FVector HL = H->GetActorLocation();

	if (Def.Type == ETPRaceType::Hunt)
	{
		for (int32 I = 0; I < C.Gates.Num(); ++I)
		{
			if (GotFlag[I] || !NearGate(HL, C.Gates[I], false)) continue;
			GotFlag[I] = true;
			--FlagsLeft;
			UpdateGateHighlights();
			if (HUD && FlagsLeft > 0) HUD->Toast(FString::Printf(TEXT("Флажок! Осталось %d"), FlagsLeft), 1.8f);
		}
		if (FlagsLeft <= 0) { FinishRace(true, FString()); return; }
	}
	else if (Def.Type == ETPRaceType::Thief)
	{
		StepRunner(Dt);
		if (ATPCitizen* R = RaceRunner.Get())
		{
			const FVector D = R->GetActorLocation() - HL;
			if (FMath::Sqrt(D.X * D.X + D.Y * D.Y + D.Z * D.Z * 0.5f) < 220.f)
			{
				R->Say(TEXT("Пусти, пусти! Всё отдам!"), 3.f);
				FinishRace(true, FString());
				return;
			}
		}
		if (bRunnerDone) { FinishRace(false, TEXT("Вор ушёл.")); return; }
	}
	else
	{
		if (RaceRunner.IsValid()) StepRunner(Dt);
		if (C.Gates.IsValidIndex(NextGate) && NearGate(HL, C.Gates[NextGate], Def.Type == ETPRaceType::Tower || C.Kinds[NextGate] == TEXT('T')))
		{
			++NextGate;
			UpdateGateHighlights();
			if (NextGate >= C.Gates.Num())
			{
				FinishRace(true, FString());
				return;
			}
		}
		if (Def.Type == ETPRaceType::Rival && bRunnerDone) { FinishRace(false, TEXT("Соперник пришёл первым.")); return; }
		// «не касаясь мостовой»: после первой крыши улица — лава
		if (Def.Type == ETPRaceType::Roofs && NextGate >= 1)
		{
			const UCharacterMovementComponent* M = H->GetCharacterMovement();
			const float FeetZ = HL.Z - H->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			if (M && M->IsMovingOnGround() && FeetZ < C.StreetZ + 150.f) { FinishRace(false, TEXT("Вы коснулись мостовой!")); return; }
		}
	}
	if (H->Combat && H->Combat->IsDefeated()) { FinishRace(false, TEXT("Вы без сил.")); return; }
	if (RaceTime > C.Limit) { FinishRace(false, TEXT("Время вышло.")); return; }
}

void UTPStorySubsystem::AbortRace()
{
	if (IsRaceActive()) FinishRace(false, TEXT("Гонка прервана."));
}

void UTPStorySubsystem::FinishRace(bool bSuccess, const FString& Why)
{
	using namespace TPStoryData;
	if (CurRace < 0) { RaceState = ETPRaceState::None; return; }
	const int32 Race = CurRace;
	const FRaceDef& Def = Races[Race];
	const FTPRaceCourse& C = Courses[Race];
	const float T = RaceTime;

	for (ATPStoryGate* G : GateActors) if (G) G->Destroy();
	GateActors.Reset();
	SetNight(false);
	if (ATPCitizen* R = RaceRunner.Get()) { R->SetRacing(false); R->ReturnHome(); }
	RaceRunner = nullptr;
	RaceState = ETPRaceState::None;
	CurRace = -1;

	ATPHUD* HUD = GetHUD();
	const UTPIdentitySubsystem* Ids = GetIdentities();
	if (!bSuccess)
	{
		if (HUD)
		{
			TArray<FString> L;
			L.Add(FString::Printf(TEXT("Не вышло|«%s»: %s"), Def.Title, *Why));
			L.Add(TEXT("Подсказка|Поговорите с бегуном Лиги ещё раз (G), чтобы повторить гонку."));
			HUD->ShowDialog(L);
		}
		return;
	}

	int32 M = 1;
	if (Def.Type == ETPRaceType::Thief) { const float Q = T / FMath::Max(RunnerTotal, 1.f); M = Q < 0.5f ? 3 : Q < 0.75f ? 2 : 1; }
	else M = T <= C.Gold ? 3 : T <= C.Silver ? 2 : 1;
	const int32 Old = Medals[Race];
	if (BestTimes[Race] <= 0.f || T < BestTimes[Race]) BestTimes[Race] = T;
	// за каждую новую ступень медали Мэтр пишет новые портреты
	static const int32 PerTier[4] = {0, 3, 3, 4};
	TArray<int32> NewPortraits;
	for (int32 Tier = Old + 1; Tier <= M; ++Tier)
		for (int32 K = 0; K < PerTier[Tier]; ++K) AwardPortrait(PoolNext(), NewPortraits);
	if (M > Old) Medals[Race] = M;
	int32 NewAbility = -1;
	if (Old == 0 && Def.Reward >= 0 && (Abilities & (1 << Def.Reward)) == 0)
	{
		Abilities |= 1 << Def.Reward;
		NewAbility = Def.Reward;
		ApplyAbilities();
	}
	bDirty = true;
	SaveProgress();

	if (!HUD) { ChapterCheck(); return; }
	TArray<FString> L;
	FString Line = FString::Printf(TEXT("Победа!|«%s» — время %s. Медаль: %s."), Def.Title, *FormatTime(T), *MedalName(M));
	if (Def.Type != ETPRaceType::Rival && Def.Type != ETPRaceType::Thief)
		Line += FString::Printf(TEXT(" Золото ≤ %s · серебро ≤ %s · бронза ≤ %s."), *FormatTime(C.Gold), *FormatTime(C.Silver), *FormatTime(C.Bronze));
	L.Add(Line);
	if (NewAbility >= 0) L.Add(FString::Printf(TEXT("Новая способность|«%s». %s"), AbilityNames[NewAbility], AbilityDescs[NewAbility]));
	if (NewPortraits.Num() && Ids)
	{
		TArray<FString> Names;
		for (const int32 I : NewPortraits) Names.Add(Ids->Get(I).Name);
		L.Add(FString::Printf(TEXT("Мэтр Огюстен|Я написал новые портреты (%d): %s. Загляните в альбом (Tab)."), NewPortraits.Num(), *FString::Join(Names, TEXT(", "))));
	}
	else if (NewAbility < 0) L.Add(TEXT("Мэтр Огюстен|Новых портретов нет: улучшите медаль, чтобы я взялся за кисть."));
	TWeakObjectPtr<UTPStorySubsystem> Weak(this);
	HUD->ShowDialogThen(L, [Weak]() { if (UTPStorySubsystem* S = Weak.Get()) S->ChapterCheck(); });
}

/* ======================= сведения для интерфейса ======================= */

FString UTPStorySubsystem::GetRaceTitle() const
{
	return CurRace >= 0 ? FString(TPStoryData::Races[CurRace].Title) : FString();
}

FString UTPStorySubsystem::GetRaceStatus() const
{
	if (CurRace < 0) return FString();
	const TPStoryData::FRaceDef& Def = TPStoryData::Races[CurRace];
	const FTPRaceCourse& C = Courses[CurRace];
	const int32 N = C.Gates.Num();
	switch (Def.Type)
	{
	case ETPRaceType::Hunt:
		return FString::Printf(TEXT("Флажки: %d из %d · лимит %s"), N - FlagsLeft, N, *FormatTime(C.Limit));
	case ETPRaceType::Thief:
	{
		const ATPCitizen* R = RaceRunner.Get();
		const ATPCharacter* H = Hero.Get();
		const int32 M = (R && H) ? static_cast<int32>(FVector::Dist(R->GetActorLocation(), H->GetActorLocation()) / 100.0 + 0.5) : 0;
		return FString::Printf(TEXT("До вора %d м — догоните его!"), M);
	}
	case ETPRaceType::Rival:
		return FString::Printf(TEXT("Кольца %d/%d · соперник %s"), NextGate, N,
			RunnerGate > NextGate ? TEXT("впереди") : RunnerGate < NextGate ? TEXT("позади") : TEXT("рядом"));
	case ETPRaceType::Tower:
		return FString::Printf(TEXT("Заберитесь на вершину · лимит %s"), *FormatTime(C.Limit));
	case ETPRaceType::Roofs:
		return FString::Printf(TEXT("Кольца %d из %d%s"), NextGate, N, NextGate >= 1 ? TEXT(" · мостовая — лава!") : TEXT(" · заберитесь на крышу"));
	default:
		return FString::Printf(TEXT("Кольца %d из %d"), NextGate, N);
	}
}

bool UTPStorySubsystem::GetMedalTimes(float& Gold, float& Silver, float& Bronze) const
{
	if (CurRace < 0) return false;
	const ETPRaceType Type = TPStoryData::Races[CurRace].Type;
	if (Type == ETPRaceType::Rival || Type == ETPRaceType::Thief) return false;
	const FTPRaceCourse& C = Courses[CurRace];
	Gold = C.Gold; Silver = C.Silver; Bronze = C.Bronze;
	return true;
}

bool UTPStorySubsystem::GetRaceTarget(FVector& Out) const
{
	if (CurRace < 0) return false;
	const ETPRaceType Type = TPStoryData::Races[CurRace].Type;
	const FTPRaceCourse& C = Courses[CurRace];
	if (Type == ETPRaceType::Thief)
	{
		if (const ATPCitizen* R = RaceRunner.Get()) { Out = R->GetActorLocation(); return true; }
		return false;
	}
	if (Type == ETPRaceType::Hunt)
	{
		const ATPCharacter* H = Hero.Get();
		const FVector From = H ? H->GetActorLocation() : C.Start;
		float Best = TNumericLimits<float>::Max();
		bool bFound = false;
		for (int32 I = 0; I < C.Gates.Num(); ++I)
		{
			if (GotFlag.IsValidIndex(I) && GotFlag[I]) continue;
			const float D = FVector::DistSquared(From, C.Gates[I]);
			if (D < Best) { Best = D; Out = C.Gates[I]; bFound = true; }
		}
		return bFound;
	}
	if (!C.Gates.IsValidIndex(NextGate)) return false;
	Out = C.Gates[NextGate];
	return true;
}

void UTPStorySubsystem::GetRaceMarkers(TArray<FVector>& Out) const
{
	Out.Reset();
	if (CurRace < 0) return;
	const ETPRaceType Type = TPStoryData::Races[CurRace].Type;
	const FTPRaceCourse& C = Courses[CurRace];
	if (Type == ETPRaceType::Thief) { FVector P; if (GetRaceTarget(P)) Out.Add(P); return; }
	for (int32 I = 0; I < C.Gates.Num(); ++I)
	{
		if (Type == ETPRaceType::Hunt) { if (!(GotFlag.IsValidIndex(I) && GotFlag[I])) Out.Add(C.Gates[I]); }
		else if (I == NextGate || I == NextGate + 1) Out.Add(C.Gates[I]);
	}
}

FString UTPStorySubsystem::RaceTypeName(ETPRaceType Type)
{
	// не через UMETA(DisplayName): метаданные есть только в редакторе
	static const TCHAR* N[6] = {TEXT("спринт"), TEXT("не касаясь мостовой"), TEXT("дуэль"), TEXT("охота за флажками"), TEXT("восхождение"), TEXT("погоня за вором")};
	return N[FMath::Clamp((int32)Type, 0, 5)];
}

FString UTPStorySubsystem::FormatTime(float Seconds)
{
	Seconds = FMath::Max(0.f, Seconds);
	const int32 M = FMath::FloorToInt(Seconds / 60.f);
	const float S = Seconds - M * 60.f;
	return FString::Printf(TEXT("%d:%04.1f"), M, S);
}

FString UTPStorySubsystem::MedalName(int32 Medal)
{
	static const TCHAR* N[4] = {TEXT("нет"), TEXT("Бронза"), TEXT("Серебро"), TEXT("Золото")};
	return N[FMath::Clamp(Medal, 0, 3)];
}

FString UTPStorySubsystem::AbilityName(int32 Ability)
{
	return (Ability >= 0 && Ability < NumAbilities) ? FString(TPStoryData::AbilityNames[Ability]) : FString();
}

FString UTPStorySubsystem::AbilityDesc(int32 Ability)
{
	return (Ability >= 0 && Ability < NumAbilities) ? FString(TPStoryData::AbilityDescs[Ability]) : FString();
}
