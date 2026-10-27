#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
class UKismetMathLibrary : public UBlueprintFunctionLibrary
{
public:
	typedef UBlueprintFunctionLibrary Super; typedef UKismetMathLibrary ThisClass; static UClass* StaticClass();
	static FRotator FindLookAtRotation(const FVector& Start, const FVector& Target);
	static FRotator MakeRotFromX(const FVector& X); static FRotator MakeRotFromZ(const FVector& Z); static FRotator MakeRotFromXZ(const FVector& X, const FVector& Z);
	static FRotator RInterpTo(FRotator Current, FRotator Target, float DeltaTime, float InterpSpeed);
	static FRotator NormalizedDeltaRotator(FRotator A, FRotator B);
	static FVector GetForwardVector(FRotator InRot); static FVector GetRightVector(FRotator InRot); static FVector GetUpVector(FRotator InRot);
	static float NormalizeAxis(float Angle); static float MapRangeClamped(float Value, float InRangeA, float InRangeB, float OutRangeA, float OutRangeB);
	static float FInterpTo(float Current, float Target, float DeltaTime, float InterpSpeed);
	static FVector VInterpTo(FVector Current, FVector Target, float DeltaTime, float InterpSpeed);
	static float RandomFloatInRange(float Min, float Max); static int32 RandomIntegerInRange(int32 Min, int32 Max);
	static FVector RandomUnitVector(); static FVector RandomPointInBoundingBox(const FVector& Origin, const FVector& BoxExtent);
	static FTransform MakeRelativeTransform(const FTransform& A, const FTransform& RelativeTo);
	static float Vector_Distance(FVector V1, FVector V2);
};
