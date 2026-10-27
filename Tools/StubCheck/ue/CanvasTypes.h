#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
class FCanvas { public: void Flush_GameThread(bool bForce = false); float GetDPIScale() const; };
enum ESimpleElementBlendMode
{
	SE_BLEND_Opaque = 0, SE_BLEND_Masked, SE_BLEND_Translucent, SE_BLEND_Additive, SE_BLEND_Modulate, SE_BLEND_MaskedDistanceField, SE_BLEND_MaskedDistanceFieldShadowed,
	SE_BLEND_TranslucentDistanceField, SE_BLEND_TranslucentDistanceFieldShadowed, SE_BLEND_AlphaComposite, SE_BLEND_AlphaHoldout, SE_BLEND_AlphaBlend,
	SE_BLEND_TranslucentAlphaOnly, SE_BLEND_TranslucentAlphaOnlyWriteAlpha, SE_BLEND_RGBA_MASK_START, SE_BLEND_RGBA_MASK_END = SE_BLEND_RGBA_MASK_START + 31, SE_BLEND_MAX
};
struct FCanvasUVTri
{
	FVector2D V0_Pos; FVector2D V0_UV; FLinearColor V0_Color;
	FVector2D V1_Pos; FVector2D V1_UV; FLinearColor V1_Color;
	FVector2D V2_Pos; FVector2D V2_UV; FLinearColor V2_Color;
	FCanvasUVTri();
};
