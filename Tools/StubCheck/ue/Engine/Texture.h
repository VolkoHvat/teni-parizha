#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Engine/TextureDefines.h"
#include "TextureResource.h"
#include "RHI.h"
class UTexture : public UObject
{
public:
	typedef UObject Super; typedef UTexture ThisClass; static UClass* StaticClass();
	UTexture(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	int32 LODBias; int32 NumCinematicMipLevels;
	uint8 SRGB : 1; uint8 NeverStream : 1; uint8 bNoTiling : 1; uint8 CompressionNone : 1; uint8 CompressionNoAlpha : 1; uint8 DeferCompression : 1; uint8 bUseCinematicMipLevels : 1;
	TEnumAsByte<enum TextureCompressionSettings> CompressionSettings;
	TEnumAsByte<enum TextureFilter> Filter;
	TEnumAsByte<enum TextureGroup> LODGroup;
#if WITH_EDITORONLY_DATA
	TEnumAsByte<enum TextureMipGenSettings> MipGenSettings;
	float AdjustBrightness; float AdjustSaturation; float AdjustVibrance;
#endif
	FTextureResource* Resource;
	virtual void UpdateResource();
	void ReleaseResource();
	virtual float GetSurfaceWidth() const; virtual float GetSurfaceHeight() const;
	virtual FTextureResource* CreateResource();
	virtual uint32 CalcTextureMemorySizeEnum(int32 Enum) const;
};
