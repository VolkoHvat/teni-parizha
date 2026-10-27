#include "TPEnvironment.h"
#include "TeniParizha.h"
#include "TPCityGenerator.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"

ATPEnvironment::ATPEnvironment()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); RootComponent = Root;

	Sun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Sun"));
	Sun->SetupAttachment(Root);
	Sun->SetAtmosphereSunLight(true); Sun->SetAtmosphereSunLightIndex(0);
	Sun->SetIntensity(9.f); Sun->SetLightColor(FLinearColor(1.f, .95f, .88f));
	Sun->SetDynamicShadowCascades(4); Sun->bUseTemperature = false;
	Sun->SetCastShadows(true);

	Moon = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("Moon"));
	Moon->SetupAttachment(Root);
	Moon->SetAtmosphereSunLight(true); Moon->SetAtmosphereSunLightIndex(1);
	Moon->SetIntensity(0.25f); Moon->SetLightColor(FLinearColor(.55f, .65f, 1.f));
	Moon->SetRelativeRotation(FRotator(-40.f, 120.f, 0.f));

	Atmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("Atmosphere"));
	Atmosphere->SetupAttachment(Root);

	SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
	SkyLight->SetupAttachment(Root);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetMobility(EComponentMobility::Movable);

	Fog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("Fog"));
	Fog->SetupAttachment(Root);
	Fog->SetFogDensity(0.012f); Fog->SetFogHeightFalloff(0.08f);
	Fog->SetVolumetricFog(true); Fog->SetVolumetricFogScatteringDistribution(0.6f);

	Clouds = CreateDefaultSubobject<UVolumetricCloudComponent>(TEXT("Clouds"));
	Clouds->SetupAttachment(Root);

	Post = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Post"));
	Post->SetupAttachment(Root);
	Post->bUnbound = true;
	FPostProcessSettings& S = Post->Settings;
	S.bOverride_AutoExposureMinBrightness = true; S.AutoExposureMinBrightness = 0.3f;
	S.bOverride_AutoExposureMaxBrightness = true; S.AutoExposureMaxBrightness = 3.f;
	S.bOverride_BloomIntensity = true; S.BloomIntensity = 0.5f;
	S.bOverride_VignetteIntensity = true; S.VignetteIntensity = 0.35f;
	S.bOverride_ColorSaturation = true; S.ColorSaturation = FVector4(1.05f, 1.02f, .96f, 1.f);
	S.bOverride_SceneFringeIntensity = true; S.SceneFringeIntensity = 0.2f;
}

void ATPEnvironment::OnConstruction(const FTransform& T)
{
	Super::OnConstruction(T);
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		Clouds->SetMaterial(M);
	ApplyTime();
}

void ATPEnvironment::BeginPlay()
{
	Super::BeginPlay();
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineSky/VolumetricClouds/m_SimpleVolumetricCloud_Inst.m_SimpleVolumetricCloud_Inst"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		Clouds->SetMaterial(M);
	SpawnLamps();
	ApplyTime();
}

bool ATPEnvironment::IsNight() const { return TimeOfDay < 6.f || TimeOfDay > 20.5f; }

void ATPEnvironment::SpawnLamps()
{
	for (TActorIterator<ATPCityGenerator> It(GetWorld()); It; ++It)
	{
		int32 N = 0;
		for (const FVector& P : It->GetLampPoints())
		{
			if (++N > 96) break;   // разумный предел для динамических источников
			UPointLightComponent* L = NewObject<UPointLightComponent>(this);
			L->SetupAttachment(Root); L->RegisterComponent();
			L->SetWorldLocation(P); L->SetMobility(EComponentMobility::Movable); L->SetIntensityUnits(ELightUnits::Lumens);
			L->SetLightColor(FLinearColor(1.f, .62f, .3f)); L->SetAttenuationRadius(1400.f);
			L->SetSourceRadius(12.f); L->SetCastShadows(N <= 16); L->SetIntensity(0.f);
			Lamps.Add(L);
		}
		break;
	}
}

void ATPEnvironment::Tick(float Dt)
{
	Super::Tick(Dt);
	if (DayLengthMinutes > 0.f) { TimeOfDay = FMath::Fmod(TimeOfDay + Dt * 24.f / (DayLengthMinutes * 60.f), 24.f); ApplyTime(); }
}

void ATPEnvironment::ApplyTime()
{
	// солнце: восход в 6, закат в 18; азимут меняется вместе с высотой
	const float DayFrac = (TimeOfDay - 6.f) / 12.f;
	const float Elev = FMath::Sin(DayFrac * PI) * 62.f;            // градусы над горизонтом
	const float Azim = -60.f + DayFrac * 200.f;
	Sun->SetWorldRotation(FRotator(-Elev, Azim, 0.f));
	const float Day = FMath::Clamp((Elev + 4.f) / 12.f, 0.f, 1.f);
	Sun->SetIntensity(9.f * Day);
	Sun->SetVisibility(Day > 0.001f);
	const float Dusk = FMath::Clamp(1.f - FMath::Abs(Elev - 6.f) / 14.f, 0.f, 1.f);
	Sun->SetLightColor(FMath::Lerp(FLinearColor(1.f, .95f, .88f), FLinearColor(1.f, .55f, .25f), Dusk));
	Moon->SetIntensity(FMath::Lerp(0.f, 0.35f, 1.f - Day));
	SkyLight->SetIntensity(FMath::Lerp(0.25f, 1.f, Day));
	Fog->SetFogDensity(FMath::Lerp(0.02f, 0.012f, Day));
	// фонари зажигаются в сумерках
	const float Lamp = FMath::Clamp((1.f - Day) * 1.5f, 0.f, 1.f);
	if (FMath::Abs(Lamp - LampLevel) > 0.01f)
	{
		LampLevel = Lamp;
		for (UPointLightComponent* L : Lamps) if (L) L->SetIntensity(Lamp * 1600.f);
		if (UWorld* W = GetWorld()) for (TActorIterator<ATPCityGenerator> It(W); It; ++It) It->SetLampGlow(Lamp);   // стёкла фонарей светятся только в темноте
	}
}
