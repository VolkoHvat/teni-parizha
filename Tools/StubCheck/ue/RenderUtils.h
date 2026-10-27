#pragma once
// TP_STUB_MODULE: RenderCore
#include "CoreMinimal.h"
#include "PixelFormat.h"
#include "RenderResource.h"
SIZE_T CalcTextureSize(uint32 SizeX, uint32 SizeY, EPixelFormat Format, uint32 MipCount);
SIZE_T CalcTextureMipMapSize(uint32 TextureSizeX, uint32 TextureSizeY, EPixelFormat Format, uint32 MipIndex);
uint32 CalcTextureMipWidthInBlocks(uint32 TextureSizeX, EPixelFormat Format, uint32 MipIndex);
