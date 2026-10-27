// Макет Engine/Texture2D.h (UE 4.27): PlatformData — открытый член (GetPlatformData() появился в UE5).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Engine/Texture.h"
class UTexture2D : public UTexture
{
public:
	typedef UTexture Super; typedef UTexture2D ThisClass; static UClass* StaticClass();
	UTexture2D(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FTexturePlatformData* PlatformData;
	TEnumAsByte<enum TextureAddress> AddressX; TEnumAsByte<enum TextureAddress> AddressY;
	int32 FirstResourceMemMip; uint32 bTemporarilyDisableStreaming : 1; uint32 bIsStreamable : 1;
	static UTexture2D* CreateTransient(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, const FName InName = NAME_None);
	int32 GetSizeX() const; int32 GetSizeY() const; int32 GetNumMips() const; EPixelFormat GetPixelFormat(uint32 LayerIndex = 0) const;
	FTexturePlatformData** GetRunningPlatformData();
	void UpdateTextureRegions(int32 MipIndex, uint32 NumRegions, const FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, TFunction<void(uint8* SrcData, const FUpdateTextureRegion2D* Regions)> DataCleanupFunc = TFunction<void(uint8*, const FUpdateTextureRegion2D*)>());
	virtual float GetSurfaceWidth() const override; virtual float GetSurfaceHeight() const override;
	FIntPoint GetImportedSize() const;
	bool IsReadyForAsyncPostLoad() const;
	virtual void UpdateResource() override;
};
