// Тени Парижа — отметки гонок Лиги крыш.
#include "TPStoryGate.h"
#include "TPTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace TPGateConst
{
	constexpr int32 NumBeads = 20;
	constexpr float RingRadius = 150.f;
}

ATPStoryGate::ATPStoryGate()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);

	// LoadObject вместо ConstructorHelpers: без ассета не падаем, а просто остаёмся без меша
	UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"), nullptr, LOAD_NoWarn | LOAD_Quiet);

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	RingPivot = CreateDefaultSubobject<USceneComponent>(TEXT("RingPivot"));
	RingPivot->SetupAttachment(Root);

	auto MakePart = [this](const TCHAR* PartName, USceneComponent* AttachTo, UStaticMesh* PartMesh) {
		UStaticMeshComponent* C = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		C->SetupAttachment(AttachTo);
		if (PartMesh) C->SetStaticMesh(PartMesh);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCastShadow(false);
		C->SetCanEverAffectNavigation(false);
		return C;
	};

	// кольцо из светящихся бусин: сферы симметричны, поэтому кольцо выглядит одинаково при любом повороте
	for (int32 I = 0; I < TPGateConst::NumBeads; ++I)
	{
		const FName Name(*FString::Printf(TEXT("Bead%d"), I));
		UStaticMeshComponent* B = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		B->SetupAttachment(RingPivot);
		if (SphereMesh) B->SetStaticMesh(SphereMesh);
		B->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		B->SetGenerateOverlapEvents(false);
		B->SetCastShadow(false);
		B->SetCanEverAffectNavigation(false);
		const float A = 2.f * PI * I / TPGateConst::NumBeads;
		B->SetRelativeLocation(FVector(0.f, FMath::Cos(A) * TPGateConst::RingRadius, FMath::Sin(A) * TPGateConst::RingRadius));
		B->SetRelativeScale3D(FVector(0.26f));
		Segments.Add(B);
	}

	Pole = MakePart(TEXT("Pole"), Root, CylinderMesh);
	Pole->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	Pole->SetRelativeScale3D(FVector(0.07f, 0.07f, 2.6f));

	Cloth = MakePart(TEXT("Cloth"), Root, CubeMesh);
	Cloth->SetRelativeLocation(FVector(0.f, 45.f, 118.f));
	Cloth->SetRelativeScale3D(FVector(0.03f, 0.9f, 0.55f));

	Beam = MakePart(TEXT("Beam"), Root, CylinderMesh);
	Beam->SetRelativeLocation(FVector(0.f, 0.f, 3200.f));
	Beam->SetRelativeScale3D(FVector(0.18f, 0.18f, 60.f));

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetIntensity(6000.f);
	Glow->SetAttenuationRadius(650.f);
	Glow->SetCastShadows(false);
	Glow->SetLightColor(BaseColor);
}

void ATPStoryGate::SetColorParams(UMaterialInstanceDynamic* M, const FLinearColor& C, float Emissive)
{
	if (!M) return;
	// BasicShapeMaterial понимает «Color»; остальные имена — для материала свечения из tp_setup.py, если он есть
	M->SetVectorParameterValue(TEXT("Color"), C);
	M->SetVectorParameterValue(TEXT("BaseColor"), C);
	M->SetVectorParameterValue(TEXT("EmissiveColor"), C * Emissive);
	M->SetVectorParameterValue(TEXT("Emissive"), C * Emissive);
	M->SetScalarParameterValue(TEXT("EmissiveStrength"), Emissive);
	M->SetScalarParameterValue(TEXT("Intensity"), Emissive);
}

void ATPStoryGate::Setup(ETPGateKind InKind, const FVector& FacingDir)
{
	Kind = InKind;
	UMaterialInterface* Glowing = LoadObject<UMaterialInterface>(nullptr, TPPaths::EmissiveMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* Basic = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UMaterialInterface* Parent = Glowing ? Glowing : Basic;
	if (Parent)
	{
		MainMat = UMaterialInstanceDynamic::Create(Parent, this);
		BeamMat = UMaterialInstanceDynamic::Create(Parent, this);
	}
	if (Basic) PoleMat = UMaterialInstanceDynamic::Create(Basic, this);

	switch (Kind)
	{
	case ETPGateKind::Flag: BaseColor = FLinearColor(1.f, .16f, .1f); break;
	case ETPGateKind::Summit: BaseColor = FLinearColor(.45f, .85f, 1.f); break;
	default: BaseColor = FLinearColor(1.f, .74f, .2f); break;
	}

	for (UStaticMeshComponent* S : Segments)
		if (S) { S->SetVisibility(Kind != ETPGateKind::Flag); if (MainMat) S->SetMaterial(0, MainMat); }
	Pole->SetVisibility(Kind == ETPGateKind::Flag);
	Cloth->SetVisibility(Kind == ETPGateKind::Flag);
	if (PoleMat) Pole->SetMaterial(0, PoleMat);
	if (MainMat) Cloth->SetMaterial(0, MainMat);
	if (BeamMat) Beam->SetMaterial(0, BeamMat);

	FVector F = FacingDir; F.Z = 0.f;
	if (!F.Normalize()) F = FVector::ForwardVector;
	if (Kind == ETPGateKind::Summit)
	{
		// кольцо лежит горизонтально над вершиной
		RingPivot->SetWorldRotation(FRotator(90.f, 0.f, 0.f));
		RingPivot->SetRelativeScale3D(FVector(1.4f));
	}
	else RingPivot->SetWorldRotation(FRotator(0.f, F.Rotation().Yaw, 0.f));
	Glow->SetLightColor(BaseColor);
	ApplyColors();
}

void ATPStoryGate::SetHighlight(int32 Level)
{
	Highlight = Level;
	SetActorHiddenInGame(Level <= 0);
	Beam->SetVisibility(Level == 1);
	Glow->SetVisibility(Level == 1);
	ApplyColors();
}

void ATPStoryGate::ApplyColors()
{
	const float K = Highlight == 1 ? 1.f : 0.35f;
	SetColorParams(MainMat, BaseColor * K, Highlight == 1 ? 12.f : 2.f);
	SetColorParams(BeamMat, BaseColor, 6.f);
	SetColorParams(PoleMat, FLinearColor(.18f, .12f, .08f), 0.f);
}

void ATPStoryGate::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Highlight <= 0) return;
	Age += DeltaSeconds;
	const float Pulse = 1.f + 0.06f * FMath::Sin(Age * 4.f);
	if (Kind == ETPGateKind::Summit) RingPivot->SetRelativeScale3D(FVector(1.4f * Pulse));
	else if (Kind == ETPGateKind::Ring) RingPivot->SetRelativeScale3D(FVector(Pulse));
	if (Kind == ETPGateKind::Flag)
		Cloth->SetRelativeRotation(FRotator(0.f, 12.f * FMath::Sin(Age * 5.f), 4.f * FMath::Sin(Age * 7.3f)));
	// свет — только у ближних отметок (охота за флажками зажигает до шести сразу):
	// гаснет плавно между 40 и 60 м от камеры, дальше выключен совсем
	float Fade = 1.f;
	if (const UWorld* W = GetWorld())
		if (const APlayerController* PC = W->GetFirstPlayerController())
			if (PC->PlayerCameraManager)
			{
				const float D = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetActorLocation());
				Fade = 1.f - FMath::Clamp((D - 4000.f) / 2000.f, 0.f, 1.f);
			}
	const bool bLit = Highlight == 1 && Fade > 0.f;
	if (Glow->IsVisible() != bLit) Glow->SetVisibility(bLit);
	if (bLit) Glow->SetIntensity(6000.f * Pulse * Fade);
}
