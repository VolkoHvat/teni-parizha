#include "TPEnvironment.h"
#include "TeniParizha.h"
#include "TPCityGenerator.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace
{
	// фонарь: световой поток в люменах и радиус затухания, см
	const float LampLumens = 1600.f;
	const float LampRadius = 1200.f;
	// шаг времени (игровых часов), после которого заново выставляется свет: ~0.6 с при сутках в 48 минут,
	// солнце сдвигается меньше чем на 0.1° — тени не «плывут» каждый кадр
	const float TimeStep = 0.005f;
}

ATPEnvironment::ATPEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// солнце — единственный источник с тенями: каскады 3 (на средних настройках движок ограничит до 2)
	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetMobility(EComponentMobility::Movable);
	Sun->SetAtmosphereSunLight(true);
	Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(9.f);
	Sun->SetLightColor(FLinearColor(1.f, .95f, .88f));
	Sun->bUseTemperature = false;
	Sun->SetCastShadows(true);
	Sun->SetDynamicShadowCascades(3);
	Sun->SetDynamicShadowDistanceMovableLight(9000.f);
	Sun->SetCascadeDistributionExponent(2.6f);

	// луна без теней: подсветка крыш и атмосферы ночью
	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetMobility(EComponentMobility::Movable);
	Moon->SetAtmosphereSunLight(true);
	Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(0.25f);
	Moon->SetLightColor(FLinearColor(.55f, .65f, 1.f));
	Moon->SetCastShadows(false);
	Moon->SetRelativeRotation(FRotator(-40.f, 120.f, 0.f));

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	// небесный свет: снимок сцены по запросу (не каждый кадр), маленький кубмап
	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->SetMobility(EComponentMobility::Movable);
	SkyLight->SourceType = SLS_CapturedScene;
	SkyLight->bRealTimeCapture = false;
	SkyLight->CubemapResolution = 128;
	SkyLight->bLowerHemisphereIsBlack = false;
	SkyLight->LowerHemisphereColor = FLinearColor(.08f, .07f, .06f);

	// обычный экспоненциальный туман, без объёмного
	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.012f);
	Fog->SetFogHeightFalloff(0.08f);
	Fog->SetVolumetricFog(false);
	Fog->SetFogMaxOpacity(0.85f);

	// стилизация — цветом и дешёвой пост-обработкой
	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->bUnbound = true;
	FPostProcessSettings& S = Post->Settings;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = 0.3f;
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 3.f;
	S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.5f;
	S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.35f;
	S.bOverride_ColorSaturation = true; S.ColorSaturation = FVector4(1.05f, 1.02f, .96f, 1.f);
	S.bOverride_ColorGain = true; S.ColorGain = FVector4(1.03f, 1.f, .96f, 1.f);
	S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.2f;
	S.bOverride_MotionBlurAmount = true; S.MotionBlurAmount = 0.f;
}

void ATPEnvironment::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyTime(true);
}

void ATPEnvironment::BeginPlay()
{
	Super::BeginPlay();
	CreateLampPool();
	ApplyTime(true);
	UpdateLamps();
}

bool ATPEnvironment::IsNight() const { return TimeOfDay < 6.f || TimeOfDay > 20.5f; }

ATPCityGenerator* ATPEnvironment::FindCity()
{
	if (City.IsValid()) return City.Get();
	if (UWorld* W = GetWorld())
		for (TActorIterator<ATPCityGenerator> It(W); It; ++It) { City = *It; break; }
	return City.Get();
}

void ATPEnvironment::CreateLampPool()
{
	const int32 N = FMath::Clamp(MaxLiveLamps, 0, 8);
	for (int32 i = 0; i < N; ++i)
	{
		UPointLightComponent* L = NewObject<UPointLightComponent>(this);
		L->SetupAttachment(Root);
		L->SetMobility(EComponentMobility::Movable);
		L->SetCastShadows(false);   // по правилам бюджета фонари без теней
		L->SetIntensityUnits(ELightUnits::Lumens);
		L->SetLightColor(FLinearColor(1.f, .62f, .3f));
		L->SetAttenuationRadius(LampRadius);
		L->SetSourceRadius(12.f);
		L->SetIntensity(0.f);
		L->SetVisibility(false);
		L->RegisterComponent();
		Lamps.Add(L);
		LampPointOf.Add(INDEX_NONE);
		LampLastIntensity.Add(0.f);
	}
}

FVector ATPEnvironment::GetViewPoint() const
{
	if (UWorld* W = GetWorld())
		if (APlayerController* PC = W->GetFirstPlayerController())
		{
			if (PC->PlayerCameraManager) return PC->PlayerCameraManager->GetCameraLocation();
			if (APawn* P = PC->GetPawn()) return P->GetActorLocation();
		}
	return GetActorLocation();
}

void ATPEnvironment::UpdateLamps()
{
	if (Lamps.Num() == 0) return;
	ATPCityGenerator* C = FindCity();
	const bool bOn = LampLevel > 0.001f && C != nullptr;
	const TArray<FVector> Empty;
	const TArray<FVector>& Points = bOn ? C->GetLampPoints() : Empty;

	// K ближайших точек (частичная сортировка вставками) и расстояние до следующей за ними
	const int32 K = Lamps.Num();
	const FVector View = GetViewPoint();
	TArray<int32, TInlineAllocator<9>> Best;
	TArray<float, TInlineAllocator<9>> BestD;
	Best.Reserve(K + 1); BestD.Reserve(K + 1);
	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const float D = FVector::DistSquared(View, Points[i]);
		if (Best.Num() == K + 1 && D >= BestD.Last()) continue;
		int32 At = BestD.Num();
		while (At > 0 && BestD[At - 1] > D) --At;
		Best.Insert(i, At); BestD.Insert(D, At);
		if (Best.Num() > K + 1) { Best.Pop(); BestD.Pop(); }
	}
	// граница отсечения: фонарь плавно гаснет к ней, поэтому замена 8-го на 9-й не видна
	const float Cut = Best.Num() > K ? FMath::Sqrt(BestD[K]) : LampRadius * 4.f;
	if (Best.Num() > K) { Best.SetNum(K); BestD.SetNum(K); }

	// свет, чья точка осталась в наборе, стоит на месте; остальные переезжают на новые точки
	TArray<bool, TInlineAllocator<9>> Taken; Taken.Init(false, Best.Num());
	TArray<bool, TInlineAllocator<9>> Kept; Kept.Init(false, K);
	for (int32 l = 0; l < K; ++l)
	{
		const int32 Idx = Best.Find(LampPointOf[l]);
		if (LampPointOf[l] != INDEX_NONE && Idx != INDEX_NONE) { Taken[Idx] = true; Kept[l] = true; }
	}
	int32 Next = 0;
	for (int32 l = 0; l < K; ++l)
	{
		if (Kept[l]) continue;
		while (Next < Best.Num() && Taken[Next]) ++Next;
		if (Next < Best.Num())
		{
			LampPointOf[l] = Best[Next];
			Taken[Next] = true;
			Lamps[l]->SetWorldLocation(Points[Best[Next]]);
		}
		else LampPointOf[l] = INDEX_NONE;
	}

	for (int32 l = 0; l < K; ++l)
	{
		UPointLightComponent* L = Lamps[l];
		if (!L) continue;
		float Target = 0.f;
		if (bOn && Points.IsValidIndex(LampPointOf[l]))
		{
			const float D = FVector::Dist(View, Points[LampPointOf[l]]);
			const float Fade = FMath::Clamp((Cut - D) / (Cut * 0.2f + 100.f), 0.f, 1.f);
			Target = LampLumens * LampLevel * Fade;
		}
		if (FMath::Abs(Target - LampLastIntensity[l]) > 8.f || (Target == 0.f) != (LampLastIntensity[l] == 0.f))
		{
			LampLastIntensity[l] = Target;
			L->SetIntensity(Target);
			L->SetVisibility(Target > 0.f);
		}
	}
}

void ATPEnvironment::Tick(float Dt)
{
	Super::Tick(Dt);
	if (DayLengthMinutes > 0.f)
	{
		TimeOfDay = FMath::Fmod(TimeOfDay + Dt * 24.f / (DayLengthMinutes * 60.f), 24.f);
		ApplyTime(false);
	}
	// выбор ближайших фонарей каждый кадр: это O(число фонарей), а плавное угасание у границы
	// без скачков яркости требует частого обновления (проверено моделью: при беге 7 м/с шаг яркости ≤ 5 %)
	if (LampLevel > 0.001f) UpdateLamps();
}

void ATPEnvironment::ApplyTime(bool bForce)
{
	if (!Sun || !Moon || !SkyLight || !Fog) return;
	float Moved = FMath::Abs(TimeOfDay - AppliedTime);
	Moved = FMath::Min(Moved, 24.f - Moved);
	if (!bForce && Moved < TimeStep) return;
	AppliedTime = TimeOfDay;

	// солнце: восход в 6, закат в 18; азимут меняется вместе с высотой
	const float DayFrac = (TimeOfDay - 6.f) / 12.f;
	const float Elev = FMath::Sin(DayFrac * PI) * 62.f;            // градусы над горизонтом
	const float Azim = -60.f + DayFrac * 200.f;
	Sun->SetWorldRotation(FRotator(-Elev, Azim, 0.f));
	const float Day = FMath::Clamp((Elev + 4.f) / 12.f, 0.f, 1.f);
	Sun->SetIntensity(9.f * Day);
	Sun->SetVisibility(Day > 0.001f);   // ночью солнце выключено — тени не считаются
	const float Dusk = FMath::Clamp(1.f - FMath::Abs(Elev - 6.f) / 14.f, 0.f, 1.f);
	Sun->SetLightColor(FMath::Lerp(FLinearColor(1.f, .95f, .88f), FLinearColor(1.f, .55f, .25f), Dusk));
	Moon->SetIntensity(FMath::Lerp(0.f, 0.35f, 1.f - Day));
	SkyLight->SetIntensity(FMath::Lerp(0.25f, 1.f, Day));
	Fog->SetFogDensity(FMath::Lerp(0.02f, 0.012f, Day));

	// пересъёмка неба: раз в игровой час, в сумерках (свет меняется быстро) — раз в четверть часа
	UWorld* W = GetWorld();
	if (W && W->IsGameWorld())
	{
		const bool bTwilight = Elev > -8.f && Elev < 12.f;
		const float Slot = bTwilight ? FMath::FloorToFloat(TimeOfDay * 4.f) / 4.f : FMath::FloorToFloat(TimeOfDay);
		if (Slot != CaptureSlot)
		{
			CaptureSlot = Slot;
			SkyLight->RecaptureSky();
		}
	}

	// фонари зажигаются в сумерках
	const float Lamp = FMath::Clamp((1.f - Day) * 1.5f, 0.f, 1.f);
	if (FMath::Abs(Lamp - LampLevel) > 0.01f || (Lamp == 0.f && LampLevel != 0.f))
	{
		LampLevel = Lamp;
		if (ATPCityGenerator* C = FindCity()) C->SetLampGlow(Lamp);   // стёкла фонарей светятся только в темноте
		UpdateLamps();
	}
}
