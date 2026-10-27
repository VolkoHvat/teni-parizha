#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ReferenceSkeleton.h"
class USkeletalMeshSocket;
class USkeleton : public UObject
{
public:
	typedef UObject Super; typedef USkeleton ThisClass; static UClass* StaticClass();
	USkeleton(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<USkeletalMeshSocket*> Sockets;
	const FReferenceSkeleton& GetReferenceSkeleton() const;
	USkeletalMeshSocket* FindSocket(FName InSocketName) const;
	int32 GetMeshBoneIndexFromSkeletonBoneIndex(const class USkeletalMesh* InSkelMesh, const int32 SkeletonBoneIndex) const;
	int32 GetSkeletonBoneIndexFromMeshBoneIndex(const class USkeletalMesh* InSkelMesh, const int32 MeshBoneIndex) const;
};
