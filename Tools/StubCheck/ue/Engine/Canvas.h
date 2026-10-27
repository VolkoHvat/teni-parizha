// Макет Engine/Canvas.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CanvasTypes.h"
#include "Engine/Font.h"
#include "Engine/EngineTypes.h"
class FCanvasItem; class UTexture; class UTexture2D; class UMaterialInterface; class FSceneView;
struct FCanvasIcon { UTexture* Texture; float U; float V; float UL; float VL; };
class UCanvas : public UObject
{
public:
	typedef UObject Super; typedef UCanvas ThisClass; static UClass* StaticClass();
	UCanvas(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float OrgX; float OrgY; float ClipX; float ClipY; FColor DrawColor; uint32 bCenterX : 1; uint32 bCenterY : 1; uint32 bNoSmooth : 1;
	int32 SizeX; int32 SizeY; int32 UnsafeSizeX; int32 UnsafeSizeY; FPlane ColorModulate;
	UTexture2D* DefaultTexture; UTexture2D* GradientTexture0; FCanvas* Canvas; FSceneView* SceneView;
	void DrawItem(FCanvasItem& Item); void DrawItem(FCanvasItem& Item, const FVector2D& InPosition); void DrawItem(FCanvasItem& Item, float X, float Y);
	float DrawText(const UFont* InFont, const FString& InText, float X, float Y, float XScale = 1.f, float YScale = 1.f, const FFontRenderInfo& RenderInfo = FFontRenderInfo());
	float DrawText(const UFont* InFont, const FText& InText, float X, float Y, float XScale = 1.f, float YScale = 1.f, const FFontRenderInfo& RenderInfo = FFontRenderInfo());
	void StrLen(const UFont* InFont, const FString& InText, float& XL, float& YL, bool bDPIAware = false);
	void TextSize(const UFont* InFont, const FString& InText, float& XL, float& YL, float ScaleX = 1.f, float ScaleY = 1.f);
	void DrawTile(UTexture* Tex, float X, float Y, float XL, float YL, float U, float V, float UL, float VL, EBlendMode BlendMode = BLEND_Translucent);
	void DrawIcon(FCanvasIcon Icon, float X, float Y, float Scale = 0.f);
	void SetDrawColor(uint8 R, uint8 G, uint8 B, uint8 A = 255); void SetDrawColor(FColor const& C);
	void SetLinearDrawColor(FLinearColor InColor); void SetLinearDrawColor(FLinearColor InColor, float OverrideAlpha);
	FVector Project(FVector Location) const;
	void Deproject(FVector2D ScreenPosition, FVector& WorldOrigin, FVector& WorldDirection) const;
	void K2_DrawLine(FVector2D ScreenPositionA = FVector2D::ZeroVector, FVector2D ScreenPositionB = FVector2D::ZeroVector, float Thickness = 1.0f, FLinearColor RenderColor = FLinearColor::White);
	void K2_DrawTexture(UTexture* RenderTexture, FVector2D ScreenPosition, FVector2D ScreenSize, FVector2D CoordinatePosition, FVector2D CoordinateSize = FVector2D::UnitVector, FLinearColor RenderColor = FLinearColor::White, EBlendMode BlendMode = BLEND_Translucent, float Rotation = 0.f, FVector2D PivotPoint = FVector2D(0.5f, 0.5f));
	void K2_DrawMaterial(UMaterialInterface* RenderMaterial, FVector2D ScreenPosition, FVector2D ScreenSize, FVector2D CoordinatePosition, FVector2D CoordinateSize = FVector2D::UnitVector, float Rotation = 0.f, FVector2D PivotPoint = FVector2D(0.5f, 0.5f));
	void K2_DrawText(UFont* RenderFont, const FString& RenderText, FVector2D ScreenPosition, FVector2D Scale = FVector2D(1.0f, 1.0f), FLinearColor RenderColor = FLinearColor::White, float Kerning = 0.0f, FLinearColor ShadowColor = FLinearColor::Black, FVector2D ShadowOffset = FVector2D::UnitVector, bool bCentreX = false, bool bCentreY = false, bool bOutlined = false, FLinearColor OutlineColor = FLinearColor::Black);
	void K2_DrawBox(FVector2D ScreenPosition, FVector2D ScreenSize, float Thickness = 1.0f, FLinearColor RenderColor = FLinearColor::White);
	void K2_DrawTriangle(UTexture* RenderTexture, TArray<FCanvasUVTri> Triangles);
	void K2_DrawMaterialTriangle(UMaterialInterface* RenderMaterial, TArray<FCanvasUVTri> Triangles);
	void K2_DrawPolygon(UTexture* RenderTexture, FVector2D ScreenPosition, FVector2D Radius = FVector2D::UnitVector, int32 NumberOfSides = 3, FLinearColor RenderColor = FLinearColor::White);
	FVector K2_Project(FVector WorldLocation); FVector2D K2_StrLen(UFont* RenderFont, const FString& RenderText); FVector2D K2_TextSize(UFont* RenderFont, const FString& RenderText, FVector2D Scale = FVector2D(1.0f, 1.0f));
	void Reset(bool bKeepOrigin = false);
	void SetClip(float X, float Y);
	static FFontRenderInfo CreateFontRenderInfo(bool bClipText = false, bool bEnableShadow = false, FLinearColor GlowColor = FLinearColor(), FVector2D GlowOuterRadius = FVector2D(), FVector2D GlowInnerRadius = FVector2D());
};
