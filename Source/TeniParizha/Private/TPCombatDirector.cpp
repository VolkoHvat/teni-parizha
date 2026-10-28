#include "TPCombatDirector.h"
#include "TeniParizha.h"
#include "TPGuard.h"
#include "TPCombatComponent.h"
#include "TPCityGenerator.h"
#include "TPCitizen.h"
#include "TPHUD.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "CollisionQueryParams.h"

static TAutoConsoleVariable<int32> CVarTPGuardCount(
	TEXT("tp.GuardCount"), -1,
	TEXT("Сколько стражников создать при старте: -1 — случайно 12–16, 0 — не создавать."),
	ECVF_Default);

bool UTPCombatDirector::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer)) return false;
	const UWorld* W = Cast<UWorld>(Outer);
	return W && W->IsGameWorld();
}

void UTPCombatDirector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bInitialized = true;
}

void UTPCombatDirector::Deinitialize()
{
	bInitialized = false;
	if (UWorld* W = GetWorld()) W->GetTimerManager().ClearTimer(SpawnTimer);
	Guards.Reset();
	TokenOwner.Reset();
	Super::Deinitialize();
}

ETickableTickType UTPCombatDirector::GetTickableTickType() const
{
	// объект-образец класса (CDO) не должен тикать
	return HasAnyFlags(RF_ClassDefaultObject) ? ETickableTickType::Never : ETickableTickType::Conditional;
}

bool UTPCombatDirector::IsTickable() const
{
	const UWorld* W = GetWorld();
	return bInitialized && W && W->IsGameWorld() && !IsPendingKill();
}

TStatId UTPCombatDirector::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTPCombatDirector, STATGROUP_Tickables);
}

ACharacter* UTPCombatDirector::GetHero() const
{
	return UGameplayStatics::GetPlayerCharacter(this, 0);
}

void UTPCombatDirector::Tick(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	// генератор города создаётся в StartPlay режима игры и может перестроиться в своём BeginPlay —
	// поэтому расставляем стражу после начала игры, чуть позже, и повторяем попытку, пока не появятся точки прогулок
	if (!bSpawnScheduled && W->HasBegunPlay())
	{
		bSpawnScheduled = true;
		W->GetTimerManager().SetTimer(SpawnTimer, this, &UTPCombatDirector::TrySpawnGuards, 0.5f, true, 0.5f);
	}

	if (TokenCooldown > 0.f) TokenCooldown -= DeltaTime;
	// жетон держит только стражник, который в погоне и замахивается
	if (AActor* Owner = TokenOwner.Get())
	{
		const ATPGuard* G = Cast<ATPGuard>(Owner);
		if (!G || G->State != ETPGuardState::Chase || !G->Combat || G->Combat->WindUp <= 0.f) TokenOwner.Reset();
	}
	else TokenOwner.Reset();

	if (bSlowMo && W->GetRealTimeSeconds() >= SlowMoEnd)
	{
		bSlowMo = false;
		UGameplayStatics::SetGlobalTimeDilation(W, 1.f);
	}
	if (Concealment < 1.f && W->GetTimeSeconds() >= ConcealUntil) Concealment = 1.f;
	CrowdTimer -= DeltaTime;
	if (CrowdTimer <= 0.f)
	{
		CrowdTimer = 0.25f;
		UpdateCrowd();
	}

	Guards.RemoveAll([](const TWeakObjectPtr<ATPGuard>& G) { return !G.IsValid(); });
}

/** толпа: 3 и больше жителей в 3.6 м от героя прячут его от стражи (incrowd в первой версии) */
void UTPCombatDirector::UpdateCrowd()
{
	CrowdScale = 1.f;
	UWorld* W = GetWorld();
	const ACharacter* Hero = GetHero();
	if (!W || !Hero) return;
	const FVector H = Hero->GetActorLocation();
	int32 N = 0;
	for (TActorIterator<ATPCitizen> It(W); It; ++It)
	{
		const ATPCitizen* C = *It;
		if (!C || C->IsDown() || C->IsRacing()) continue;
		const FVector D = C->GetActorLocation() - H;
		if (D.SizeSquared2D() < FMath::Square(360.f) && FMath::Abs(D.Z) < 150.f && ++N >= 3)
		{
			CrowdScale = 0.35f;
			return;
		}
	}
}

/* ---------------- расстановка стражи ---------------- */

void UTPCombatDirector::TrySpawnGuards()
{
	UWorld* W = GetWorld();
	if (!W || bSpawned) return;
	++SpawnAttempts;

	ATPCityGenerator* Gen = nullptr;
	for (TActorIterator<ATPCityGenerator> It(W); It; ++It) { Gen = *It; break; }
	if (!Gen || Gen->GetWalkPoints().Num() < 2)
	{
		if (SpawnAttempts >= 16)
		{
			W->GetTimerManager().ClearTimer(SpawnTimer);
			UE_LOG(LogTeni, Log, TEXT("Стража не расставлена: нет генератора города или точек прогулок"));
		}
		return;
	}
	W->GetTimerManager().ClearTimer(SpawnTimer);
	bSpawned = true;

	const int32 Wanted = CVarTPGuardCount.GetValueOnGameThread();
	if (Wanted == 0) return;
	if (Wanted < 0 && Guards.Num() > 0)
	{
		UE_LOG(LogTeni, Log, TEXT("На уровне уже есть стража (%d) — автоматическая расстановка пропущена"), Guards.Num());
		return;
	}

	const TArray<FVector> Pts = Gen->GetWalkPoints(); // копия: генератор может перестроиться
	const FVector Start = Gen->GetPlayerStart();
	FRandomStream Rng(Gen->Seed * 7919 + 17);
	const int32 Count = Wanted > 0 ? Wanted : Rng.RandRange(MinGuards, MaxGuards);

	// сначала точки подальше от старта героя, чтобы игра не начиналась с драки
	TArray<int32> Far, Near;
	for (int32 i = 0; i < Pts.Num(); ++i) (FVector::Dist2D(Pts[i], Start) > 1800.f ? Far : Near).Add(i);
	auto Shuffle = [&Rng](TArray<int32>& A) {
		for (int32 i = A.Num() - 1; i > 0; --i) A.Swap(i, Rng.RandRange(0, i));
	};
	Shuffle(Far);
	Shuffle(Near);
	TArray<int32> Order = Far;
	Order.Append(Near);

	FCollisionQueryParams Q(SCENE_QUERY_STAT(TPGuardSpawn), false);
	for (int32 k = 0; k < Count; ++k)
	{
		const int32 Idx = Order[k % Order.Num()];
		const FVector P0 = Pts[Idx];

		// маршрут: точка и 1–3 соседние
		TArray<FVector> Route;
		Route.Add(P0);
		TArray<int32> Nb;
		for (int32 j = 0; j < Pts.Num(); ++j)
		{
			if (j == Idx) continue;
			const float D = FVector::Dist2D(Pts[j], P0);
			if (D > 250.f && D < 3000.f) Nb.Add(j);
		}
		Nb.Sort([&Pts, &P0](const int32& A, const int32& B) { return FVector::DistSquared2D(Pts[A], P0) < FVector::DistSquared2D(Pts[B], P0); });
		const int32 Extra = FMath::Min(Nb.Num(), Rng.RandRange(1, 3));
		for (int32 e = 0; e < Extra; ++e) Route.Add(Pts[Nb[e]]);

		// место появления: на земле под точкой; повтор точки — со сдвигом
		FVector Loc = P0;
		if (k >= Order.Num()) Loc += FVector(Rng.FRandRange(-150.f, 150.f), Rng.FRandRange(-150.f, 150.f), 0.f);
		FHitResult Hit;
		if (W->LineTraceSingleByChannel(Hit, Loc + FVector(0.f, 0.f, 150.f), Loc - FVector(0.f, 0.f, 600.f), ECC_Visibility, Q))
			Loc = Hit.ImpactPoint + FVector(0.f, 0.f, 95.f);
		else
			Loc += FVector(0.f, 0.f, 100.f);

		const FTransform T(FRotator(0.f, Rng.FRandRange(-180.f, 180.f), 0.f), Loc);
		ATPGuard* G = W->SpawnActorDeferred<ATPGuard>(ATPGuard::StaticClass(), T, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!G) continue;
		G->bMusketeer = (k % 3 == 2); // каждый третий — мушкетёр
		G->PatrolRoute = Route;
		G->FinishSpawning(T);
	}
	UE_LOG(LogTeni, Log, TEXT("Расставлена стража: %d"), Count);
}

/* ---------------- список стражи ---------------- */

void UTPCombatDirector::RegisterGuard(ATPGuard* Guard)
{
	if (Guard) Guards.AddUnique(TWeakObjectPtr<ATPGuard>(Guard));
}

void UTPCombatDirector::UnregisterGuard(ATPGuard* Guard)
{
	Guards.Remove(TWeakObjectPtr<ATPGuard>(Guard));
	if (TokenOwner.Get() == Guard) TokenOwner.Reset();
}

TArray<ATPGuard*> UTPCombatDirector::GetGuards() const
{
	TArray<ATPGuard*> Out;
	Out.Reserve(Guards.Num());
	for (const TWeakObjectPtr<ATPGuard>& G : Guards)
		if (ATPGuard* P = G.Get()) Out.Add(P);
	return Out;
}

/* ---------------- тревога ---------------- */

void UTPCombatDirector::AlertAll(FVector Location, float Radius)
{
	const ACharacter* Hero = GetHero();
	const FVector Seen = Hero ? Hero->GetActorLocation() : Location;
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
	{
		ATPGuard* G = W.Get();
		if (!G || G->IsKO()) continue;
		if (FVector::Dist2D(G->GetActorLocation(), Location) < Radius) G->EnterChase(Seen);
	}
}

void UTPCombatDirector::Witness(FVector Location, float Amount)
{
	const ACharacter* Hero = GetHero();
	const FVector Seen = Hero ? Hero->GetActorLocation() : Location;
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
	{
		ATPGuard* G = W.Get();
		if (!G || G->IsKO()) continue;
		if (!G->CanSeePoint(Location + FVector(0.f, 0.f, 30.f), 2400.f, Hero)) continue;
		G->AddSuspicion(Amount, Seen);
	}
}

void UTPCombatDirector::Panic(FVector Location, float Radius)
{
	if (OnPanic.IsBound())
	{
		OnPanic.Broadcast(Location, Radius);
		return;
	}
	// запасной вариант, пока жители не подписаны на OnPanic: оглянуться на шум драки
	UWorld* W = GetWorld();
	if (!W) return;
	for (TActorIterator<ATPCitizen> It(W); It; ++It)
	{
		ATPCitizen* C = *It;
		if (!C || C->IsDown() || C->IsRacing()) continue;
		if (FVector::DistSquared2D(C->GetActorLocation(), Location) < FMath::Square(Radius)) C->LookAtFor(Location, 2.5f);
	}
}

void UTPCombatDirector::ResetAlert()
{
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
		if (ATPGuard* G = W.Get()) G->ResetToPatrol();
	TokenOwner.Reset();
	TokenCooldown = 1.5f;
}

void UTPCombatDirector::RequestSlowMo(float RealSeconds, float Dilation)
{
	UWorld* W = GetWorld();
	if (!W || RealSeconds <= 0.f) return;
	const double End = W->GetRealTimeSeconds() + RealSeconds;
	if (!bSlowMo || End > SlowMoEnd) SlowMoEnd = End;
	bSlowMo = true;
	UGameplayStatics::SetGlobalTimeDilation(W, FMath::Clamp(Dilation, 0.05f, 1.f));
}

void UTPCombatDirector::SetHeroConcealment(float Scale, float Duration)
{
	UWorld* W = GetWorld();
	if (!W) return;
	Concealment = FMath::Clamp(Scale, 0.f, 1.f);
	ConcealUntil = W->GetTimeSeconds() + FMath::Max(0.f, Duration);
}

int32 UTPCombatDirector::CountChasing() const
{
	int32 N = 0;
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
		if (const ATPGuard* G = W.Get()) if (G->State == ETPGuardState::Chase) ++N;
	return N;
}

float UTPCombatDirector::GetMaxAlert() const
{
	float A = 0.f;
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
		if (const ATPGuard* G = W.Get()) if (!G->IsKO()) A = FMath::Max(A, G->Alert);
	return A;
}

ATPGuard* UTPCombatDirector::GetTelegraphingGuard(FVector Location, float Radius) const
{
	ATPGuard* Best = nullptr;
	float BestD = Radius;
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
	{
		ATPGuard* G = W.Get();
		if (!G || G->IsKO() || !G->Combat || G->Combat->WindUp <= 0.f) continue;
		const float D = FVector::Dist2D(G->GetActorLocation(), Location);
		if (D < BestD) { BestD = D; Best = G; }
	}
	return Best;
}

bool UTPCombatDirector::IsHeroThreatened(FVector Location, float Radius) const
{
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
	{
		const ATPGuard* G = W.Get();
		if (!G || G->State != ETPGuardState::Chase) continue;
		const FVector D = G->GetActorLocation() - Location;
		const float D2 = D.Size2D();
		if (D2 < Radius && FMath::Abs(D.Z) < 200.f) return true;
		if (G->bMusketeer && G->AimTime > 0.6f && D2 < 3200.f) return true;
	}
	return false;
}

bool UTPCombatDirector::IsGuardChasingNear(const FVector& Location, float Radius, float MaxHeight) const
{
	for (const TWeakObjectPtr<ATPGuard>& W : Guards)
	{
		const ATPGuard* G = W.Get();
		if (!G || G->State != ETPGuardState::Chase) continue;
		const FVector D = G->GetActorLocation() - Location;
		if (D.Size2D() < Radius && FMath::Abs(D.Z) < MaxHeight) return true;
	}
	return false;
}

void UTPCombatDirector::Message(const FString& Text)
{
	const UWorld* W = GetWorld();
	const double Now = W ? W->GetRealTimeSeconds() : 0.0;
	if (Text == LastMessage && Now - LastMessageTime < 2.5) return;
	LastMessage = Text;
	LastMessageTime = Now;
	OnCombatMessage.Broadcast(Text);
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
		if (ATPHUD* HUD = Cast<ATPHUD>(PC->GetHUD())) HUD->Toast(Text, 3.f);
}

/* ---------------- жетон атаки ---------------- */

bool UTPCombatDirector::CanTakeAttackToken(const AActor* Who, bool bIgnoreCooldown) const
{
	const AActor* Owner = TokenOwner.Get();
	if (Owner) return Owner == Who;
	return bIgnoreCooldown || TokenCooldown <= 0.f;
}

bool UTPCombatDirector::TryTakeAttackToken(AActor* Who, bool bIgnoreCooldown)
{
	if (!CanTakeAttackToken(Who, bIgnoreCooldown)) return false;
	TokenOwner = Who;
	return true;
}

void UTPCombatDirector::ReleaseAttackToken(const AActor* Who, float Cooldown)
{
	if (TokenOwner.Get() == Who) TokenOwner.Reset();
	if (Cooldown >= 0.f) TokenCooldown = FMath::Max(TokenCooldown, Cooldown);
}
