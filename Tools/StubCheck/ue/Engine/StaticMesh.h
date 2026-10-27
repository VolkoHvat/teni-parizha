// Макет Engine/StaticMesh.h (UE 4.27): BuildFromMeshDescriptions с параметрами FBuildMeshDescriptionsParams.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/EngineTypes.h"
class UMaterialInterface; class UBodySetup; class UNavCollisionBase; struct FMeshDescription; class FStaticMeshRenderData;
struct FMeshUVChannelInfo { bool bInitialized; bool bOverrideDensities; float LocalUVDensities[4]; };
struct FStaticMaterial
{
	FStaticMaterial(); FStaticMaterial(class UMaterialInterface* InMaterialInterface, FName InMaterialSlotName = NAME_None, FName InImportedMaterialSlotName = NAME_None);
	UMaterialInterface* MaterialInterface; FName MaterialSlotName; FName ImportedMaterialSlotName; FMeshUVChannelInfo UVChannelData;
};
struct FPerPlatformInt { int32 Default; FPerPlatformInt(); FPerPlatformInt(int32 InDefault); };
struct FPerPlatformFloat { float Default; FPerPlatformFloat(); FPerPlatformFloat(float InDefault); };
struct FBuildMeshDescriptionsParams
{
	FBuildMeshDescriptionsParams();
	bool bMarkPackageDirty; bool bUseHashAsGuid; bool bBuildSimpleCollision; bool bCommitMeshDescription; bool bFastBuild; bool bAllowCpuAccess;
};
class UStaticMesh : public UObject
{
public:
	typedef UObject Super; typedef UStaticMesh ThisClass; static UClass* StaticClass();
	UStaticMesh(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TUniquePtr<FStaticMeshRenderData> RenderData;
	TArray<FStaticMaterial> StaticMaterials;
	UBodySetup* BodySetup; UNavCollisionBase* NavCollision;
	int32 LightMapResolution; int32 LightMapCoordinateIndex; int32 LODForCollision; float LightmapUVDensity; float DistanceFieldSelfShadowBias;
	FPerPlatformInt MinLOD; FName LODGroup;
	FBoxSphereBounds ExtendedBounds; FVector PositiveBoundsExtension; FVector NegativeBoundsExtension;
	uint8 bAllowCPUAccess : 1; uint8 bHasNavigationData : 1; uint8 bSupportUniformlyDistributedSampling : 1; uint8 bIsBuiltAtRuntime_DEPRECATED : 1; uint8 bGenerateMeshDistanceField : 1;
	uint8 bAutoComputeLODScreenSize : 1;
	FBoxSphereBounds GetBounds() const; FBox GetBoundingBox() const;
	int32 GetNumLODs() const; int32 GetNumVertices(int32 LODIndex) const; int32 GetNumTriangles(int32 LODIndex) const; int32 GetNumSections(int32 InLOD) const;
	bool HasValidRenderData(bool bCheckLODForVerts = true, int32 LODIndex = INDEX_NONE) const;
	UMaterialInterface* GetMaterial(int32 MaterialIndex) const; int32 GetMaterialIndex(FName MaterialSlotName) const;
	FName AddMaterial(UMaterialInterface* Material);
	void SetMaterial(int32 MaterialIndex, UMaterialInterface* NewMaterial);
	const TArray<FStaticMaterial>& GetStaticMaterials() const; TArray<FStaticMaterial>& GetStaticMaterials(); void SetStaticMaterials(const TArray<FStaticMaterial>& InStaticMaterials);
	bool BuildFromMeshDescriptions(const TArray<const FMeshDescription*>& MeshDescriptions, const FBuildMeshDescriptionsParams& Params = FBuildMeshDescriptionsParams());
	void BuildFromMeshDescription(const FMeshDescription& MeshDescription, struct FStaticMeshLODResources& LODResources);
	void CreateBodySetup(); UBodySetup* GetBodySetup() const;
	void CalculateExtendedBounds();
	void InitResources(); void ReleaseResources();
	void SetLightingGuid();
	FStaticMeshRenderData* GetRenderData() const; int32 GetLightMapResolution() const; void SetLightMapResolution(int32 InLightMapResolution);
	int32 GetLightMapCoordinateIndex() const; void SetLightMapCoordinateIndex(int32 InIndex); void SetBodySetup(UBodySetup* InBodySetup);
	UNavCollisionBase* GetNavCollision() const; const FBoxSphereBounds& GetExtendedBounds() const;
	FVector GetPositiveBoundsExtension() const; void SetPositiveBoundsExtension(FVector InExt); FVector GetNegativeBoundsExtension() const; void SetNegativeBoundsExtension(FVector InExt);
#if WITH_EDITOR
	void Build(bool bInSilent = false, TArray<FText>* OutErrors = nullptr);
	int32 GetNumSourceModels() const;
	FMeshDescription* CreateMeshDescription(int32 LodIndex);
	void CommitMeshDescription(int32 LodIndex);
#endif
};
