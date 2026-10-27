// Макет ProceduralMeshComponent.h (UE 4.27, плагин ProceduralMeshComponent)
#pragma once
// TP_STUB_MODULE: ProceduralMeshComponent
#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
struct FProcMeshTangent { FVector TangentX; bool bFlipTangentY; FProcMeshTangent(); FProcMeshTangent(float X, float Y, float Z); FProcMeshTangent(FVector InTangentX, bool bInFlipTangentY); };
struct FProcMeshVertex { FVector Position; FVector Normal; FProcMeshTangent Tangent; FColor Color; FVector2D UV0; FVector2D UV1; FVector2D UV2; FVector2D UV3; };
struct FProcMeshSection { TArray<FProcMeshVertex> ProcVertexBuffer; TArray<uint32> ProcIndexBuffer; FBox SectionLocalBox; bool bEnableCollision; bool bSectionVisible; void Reset(); };
class UProceduralMeshComponent : public UMeshComponent
{
public:
	typedef UMeshComponent Super; typedef UProceduralMeshComponent ThisClass; static UClass* StaticClass();
	UProceduralMeshComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	bool bUseComplexAsSimpleCollision; bool bUseAsyncCooking;
	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);
	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);
	void CreateMeshSection_LinearColor(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);
	void CreateMeshSection_LinearColor(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FVector2D>& UV2, const TArray<FVector2D>& UV3, const TArray<FLinearColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents, bool bCreateCollision);
	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents);
	void UpdateMeshSection_LinearColor(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0, const TArray<FLinearColor>& VertexColors, const TArray<FProcMeshTangent>& Tangents);
	void ClearMeshSection(int32 SectionIndex); void ClearAllMeshSections();
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility); bool IsMeshSectionVisible(int32 SectionIndex) const;
	int32 GetNumSections() const;
	void AddCollisionConvexMesh(TArray<FVector> ConvexVerts); void ClearCollisionConvexMeshes(); void SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes);
	FProcMeshSection* GetProcMeshSection(int32 SectionIndex); void SetProcMeshSection(int32 SectionIndex, const FProcMeshSection& Section);
};
