#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class UTexture2D;
enum class EFontCacheType : uint8 { Offline, Runtime };
struct FFontCharacter { int32 StartU; int32 StartV; int32 USize; int32 VSize; uint8 TextureIndex; int32 VerticalOffset; };
struct FFontRenderInfo { uint32 bClipText : 1; uint32 bEnableShadow : 1; FFontRenderInfo(); };
class UFont : public UObject
{
public:
	typedef UObject Super; typedef UFont ThisClass; static UClass* StaticClass();
	UFont(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	EFontCacheType FontCacheType; TArray<FFontCharacter> Characters; TArray<UTexture2D*> Textures; int32 IsRemapped;
	float EmScale; float Ascent; float Descent; float Leading; int32 Kerning; int32 NumCharacters; TArray<int32> MaxCharHeight; float ScalingFactor; int32 LegacyFontSize; FName LegacyFontName;
	void GetCharSize(TCHAR InCh, float& Width, float& Height) const;
	int32 GetStringSize(const TCHAR* Text) const; int32 GetStringHeightSize(const TCHAR* Text) const;
	float GetMaxCharHeight() const;
	void GetStringHeightAndWidth(const FString& InString, int32& Height, int32& Width) const;
};
