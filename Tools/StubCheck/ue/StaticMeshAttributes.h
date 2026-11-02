// Макет StaticMeshAttributes.h (UE 4.27, модуль StaticMeshDescription): FStaticMeshAttributes с атрибутами на float-типах 4.27.
#pragma once
// TP_STUB_MODULE: StaticMeshDescription
#include "CoreMinimal.h"
#include "MeshDescription.h"
class FMeshAttributes
{
public:
	explicit FMeshAttributes(FMeshDescription& InMeshDescription);
	virtual ~FMeshAttributes();
	virtual void Register();
	TVertexAttributesRef<FVector> GetVertexPositions();
};
class FStaticMeshAttributes : public FMeshAttributes
{
public:
	explicit FStaticMeshAttributes(FMeshDescription& InMeshDescription);
	virtual void Register() override;
	TVertexInstanceAttributesRef<FVector2D> GetVertexInstanceUVs();
	TVertexInstanceAttributesRef<FVector> GetVertexInstanceNormals();
	TVertexInstanceAttributesRef<FVector> GetVertexInstanceTangents();
	TVertexInstanceAttributesRef<float> GetVertexInstanceBinormalSigns();
	TVertexInstanceAttributesRef<FVector4> GetVertexInstanceColors();
	TPolygonGroupAttributesRef<FName> GetPolygonGroupMaterialSlotNames();
};
