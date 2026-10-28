// Тени Парижа — настройка анимации мешей и оптимизация толпы.
#include "TPAnimSetup.h"
#include "TPAnimInstance.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/EngineTypes.h"

void UTPAnimSetup::ApplyAnimation(USkeletalMeshComponent* Mesh, bool bReplace)
{
	if (!Mesh)
	{
		return;
	}
	if (bReplace || !Mesh->GetAnimInstance())
	{
		Mesh->SetAnimInstanceClass(UTPAnimInstance::StaticClass());
	}
}

void UTPAnimSetup::ConfigureHero(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Mesh->bEnableUpdateRateOptimizations = false;
}

void UTPAnimSetup::ApplyCrowdUpdateRate(FAnimUpdateRateParameters* Params)
{
	if (!Params)
	{
		return;
	}
	// MaxDistanceFactor — доля экрана, которую занимает меш: крупнее 0.3 — каждый кадр, 0.3–0.15 — через кадр,
	// 0.15–0.07 — каждый 3-й, мельче — каждый 4-й (пропущенные кадры интерполируются)
	Params->bShouldUseLodMap = false;
	Params->BaseVisibleDistanceFactorThesholds.Reset();
	Params->BaseVisibleDistanceFactorThesholds.Add(0.3f);
	Params->BaseVisibleDistanceFactorThesholds.Add(0.15f);
	Params->BaseVisibleDistanceFactorThesholds.Add(0.07f);
	Params->MaxEvalRateForInterpolation = 4;
	Params->BaseNonRenderedUpdateRate = 8;
	Params->bInterpolateSkippedFrames = true;
}

void UTPAnimSetup::ConfigureCrowd(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh->bEnableUpdateRateOptimizations = true;
	Mesh->bComponentUseFixedSkelBounds = true;   // границы по физ. ассету, без пересчёта по костям каждый кадр
	Mesh->bDisableClothSimulation = true;
	Mesh->OnAnimUpdateRateParamsCreated.BindStatic(&UTPAnimSetup::ApplyCrowdUpdateRate);
	if (Mesh->AnimUpdateRateParams)
	{
		ApplyCrowdUpdateRate(Mesh->AnimUpdateRateParams);   // меш уже зарегистрирован — параметры созданы
	}
}
