// Макет CollisionShape.h (UE 4.27, модуль Engine)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
namespace ECollisionShape { enum Type { Line, Box, Sphere, Capsule }; }
struct FCollisionShape
{
	ECollisionShape::Type ShapeType;
	FCollisionShape();
	bool IsLine() const; bool IsBox() const; bool IsSphere() const; bool IsCapsule() const; bool IsNearlyZero() const;
	FVector GetExtent() const; float GetCapsuleHalfHeight() const; float GetCapsuleRadius() const; float GetSphereRadius() const; FVector GetBox() const;
	float GetCapsuleAxisHalfLength() const;
	void SetCapsule(const float Radius, const float HalfHeight); void SetCapsule(const FVector& Extent); void SetBox(const FVector& HalfExtent); void SetSphere(const float Radius);
	static FCollisionShape MakeBox(const FVector& BoxHalfExtent);
	static FCollisionShape MakeSphere(const float SphereRadius);
	static FCollisionShape MakeCapsule(const float CapsuleRadius, const float CapsuleHalfHeight);
	static FCollisionShape MakeCapsule(const FVector& Extent);
	static FCollisionShape LineShape;
};
