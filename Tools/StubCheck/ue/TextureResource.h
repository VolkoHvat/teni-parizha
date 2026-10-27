// Макет TextureResource.h (UE 4.27, модуль Engine)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "RenderResource.h"
#include "Serialization/BulkData.h"
#include "PixelFormat.h"
struct FTexture2DMipMap { int32 SizeX; int32 SizeY; int32 SizeZ; FByteBulkData BulkData; FTexture2DMipMap(); };
template<typename T> class TIndirectArray
{
public:
	int32 Num() const; T& operator[](int32 Index); const T& operator[](int32 Index) const; int32 Add(T* Item); void Empty(int32 Slack = 0); bool IsValidIndex(int32 Index) const;
	T* begin(); T* end();
};
struct FTexturePlatformData
{
	int32 SizeX; int32 SizeY; uint32 PackedData; EPixelFormat PixelFormat; TIndirectArray<FTexture2DMipMap> Mips;
	FTexturePlatformData(); int32 GetNumSlices() const;
};
class FTextureResource : public FTexture { public: virtual uint32 GetSizeX() const override; virtual uint32 GetSizeY() const override; };
