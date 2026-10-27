// Макет Components/StaticMeshComponent.h (UE 4.27): StaticMesh закрыт, доступ через GetStaticMesh/SetStaticMesh.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
class UStaticMesh;
class UStaticMeshComponent : public UMeshComponent
{
public:
	typedef UMeshComponent Super; typedef UStaticMeshComponent ThisClass; static UClass* StaticClass();
	UStaticMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	int32 ForcedLodModel; int32 MinLOD; float StreamingDistanceMultiplier;
	uint8 bOverrideMinLOD : 1; uint8 bOverrideLightMapRes : 1; uint8 bDisallowMeshPaintPerInstance : 1; uint8 bIgnoreInstanceForTextureStreaming : 1;
	uint8 bUseDefaultCollision : 1; uint8 bReverseCulling : 1; uint8 bSortTriangles_DEPRECATED : 1;
	int32 OverriddenLightMapRes;
	virtual bool SetStaticMesh(UStaticMesh* NewMesh);
	UStaticMesh* GetStaticMesh() const;
	void SetForcedLodModel(int32 NewForcedLodModel);
	void SetDistanceFieldSelfShadowBias(float NewValue);
	void SetReverseCulling(bool ReverseCulling);
	void GetLocalBounds(FVector& Min, FVector& Max) const;
private:
	UStaticMesh* StaticMesh;
};
