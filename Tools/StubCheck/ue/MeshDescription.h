// Макет MeshDescription.h (UE 4.27, модуль MeshDescription): идентификаторы элементов, ссылки на атрибуты (FVector — float,
// как в 4.27; в UE5 атрибуты стали FVector3f) и FMeshDescription с CreateVertex/CreateVertexInstance/CreatePolygonGroup/CreatePolygon.
#pragma once
// TP_STUB_MODULE: MeshDescription
#include "CoreMinimal.h"
struct FElementID { int32 IDValue; FElementID(); explicit FElementID(int32 InIDValue); int32 GetValue() const; bool operator==(const FElementID& O) const; bool operator!=(const FElementID& O) const; };
struct FVertexID : public FElementID { FVertexID(); explicit FVertexID(int32 InIDValue); static const FVertexID Invalid; };
struct FVertexInstanceID : public FElementID { FVertexInstanceID(); explicit FVertexInstanceID(int32 InIDValue); static const FVertexInstanceID Invalid; };
struct FEdgeID : public FElementID { FEdgeID(); explicit FEdgeID(int32 InIDValue); static const FEdgeID Invalid; };
struct FPolygonGroupID : public FElementID { FPolygonGroupID(); explicit FPolygonGroupID(int32 InIDValue); static const FPolygonGroupID Invalid; };
struct FPolygonID : public FElementID { FPolygonID(); explicit FPolygonID(int32 InIDValue); static const FPolygonID Invalid; };
struct FTriangleID : public FElementID { FTriangleID(); explicit FTriangleID(int32 InIDValue); static const FTriangleID Invalid; };

/** ссылка на массив атрибутов (TMeshAttributesRef в 4.27) */
template<typename ElementIDType, typename AttributeType>
class TMeshAttributesRef
{
public:
	TMeshAttributesRef();
	AttributeType& operator[](const ElementIDType ElementID) const;
	AttributeType Get(const ElementIDType ElementID, const int32 AttributeIndex = 0) const;
	void Set(const ElementIDType ElementID, const int32 AttributeIndex, const AttributeType& Value) const;
	int32 GetNumIndices() const;
	void SetNumIndices(const int32 NumIndices) const;
	int32 GetNumElements() const;
	bool IsValid() const;
};
template<typename T> using TVertexAttributesRef = TMeshAttributesRef<FVertexID, T>;
template<typename T> using TVertexInstanceAttributesRef = TMeshAttributesRef<FVertexInstanceID, T>;
template<typename T> using TEdgeAttributesRef = TMeshAttributesRef<FEdgeID, T>;
template<typename T> using TPolygonAttributesRef = TMeshAttributesRef<FPolygonID, T>;
template<typename T> using TPolygonGroupAttributesRef = TMeshAttributesRef<FPolygonGroupID, T>;

struct FMeshDescription
{
	FMeshDescription();
	FMeshDescription(const FMeshDescription&);
	FMeshDescription(FMeshDescription&&);
	FMeshDescription& operator=(const FMeshDescription&);
	FMeshDescription& operator=(FMeshDescription&&);
	void Empty();
	bool IsEmpty() const;
	void ReserveNewVertices(const int32 NumberOfNewVertices);
	FVertexID CreateVertex();
	void ReserveNewVertexInstances(const int32 NumberOfNewVertexInstances);
	FVertexInstanceID CreateVertexInstance(const FVertexID VertexID);
	void ReserveNewEdges(const int32 NumberOfNewEdges);
	void ReservePolygonGroups(const int32 NumberOfNewPolygonGroups);
	FPolygonGroupID CreatePolygonGroup();
	void ReserveNewPolygons(const int32 NumberOfNewPolygons);
	FPolygonID CreatePolygon(const FPolygonGroupID PolygonGroupID, TArrayView<const FVertexInstanceID> VertexInstanceIDs, TArray<FEdgeID>* OutEdgeIDs = nullptr);
	FTriangleID CreateTriangle(const FPolygonGroupID PolygonGroupID, TArrayView<const FVertexInstanceID> VertexInstanceIDs, TArray<FEdgeID>* OutEdgeIDs = nullptr);
	int32 Vertices_Num() const;
	int32 VertexInstances_Num() const;
	int32 Polygons_Num() const;
	int32 PolygonGroups_Num() const;
};
