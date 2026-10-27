#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class USkeletalBodySetup; class UPhysicsConstraintTemplate;
class UPhysicsAsset : public UObject
{
public:
	typedef UObject Super; typedef UPhysicsAsset ThisClass; static UClass* StaticClass();
	UPhysicsAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<USkeletalBodySetup*> SkeletalBodySetups; TArray<UPhysicsConstraintTemplate*> ConstraintSetup;
	int32 FindBodyIndex(FName BodyName) const; int32 FindConstraintIndex(FName ConstraintName);
};
