#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
class USpringArmComponent : public USceneComponent
{
public:
	typedef USceneComponent Super; typedef USpringArmComponent ThisClass; static UClass* StaticClass();
	USpringArmComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float TargetArmLength; FVector SocketOffset; FVector TargetOffset; float ProbeSize; TEnumAsByte<ECollisionChannel> ProbeChannel;
	uint32 bDoCollisionTest : 1; uint32 bUsePawnControlRotation : 1; uint32 bInheritPitch : 1; uint32 bInheritYaw : 1; uint32 bInheritRoll : 1;
	uint32 bEnableCameraLag : 1; uint32 bEnableCameraRotationLag : 1; uint32 bUseCameraLagSubstepping : 1; uint32 bDrawDebugLagMarkers : 1;
	float CameraLagSpeed; float CameraRotationLagSpeed; float CameraLagMaxTimeStep; float CameraLagMaxDistance; uint32 bClampToMaxPhysicsDeltaTime : 1;
	static const FName SocketName;
	virtual FRotator GetTargetRotation() const;
	FVector GetUnfixedCameraPosition() const; bool IsCollisionFixApplied() const;
};
