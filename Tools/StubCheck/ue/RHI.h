#pragma once
// TP_STUB_MODULE: RHI
#include "CoreMinimal.h"
#include "PixelFormat.h"
struct FUpdateTextureRegion2D
{
	uint32 DestX, DestY; int32 SrcX, SrcY; uint32 Width, Height;
	FUpdateTextureRegion2D(); FUpdateTextureRegion2D(uint32 InDestX, uint32 InDestY, int32 InSrcX, int32 InSrcY, uint32 InWidth, uint32 InHeight);
};
extern bool GIsRHIInitialized;
