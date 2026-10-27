// Макет CanvasItem.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "CanvasTypes.h"
#include "Engine/Font.h"
#include "RenderResource.h"
class FMaterialRenderProxy; class UFont;
class FCanvasItem
{
public:
	FCanvasItem(const FVector2D& InPosition); virtual ~FCanvasItem();
	virtual void Draw(FCanvas* InCanvas) = 0;
	virtual void Draw(FCanvas* InCanvas, const FVector2D& InPosition);
	virtual void Draw(FCanvas* InCanvas, float X, float Y);
	void SetColor(const FLinearColor& InColor);
	FVector2D Position; bool bFreezeTime; ESimpleElementBlendMode BlendMode; FLinearColor Color;
};
class FCanvasTileItem : public FCanvasItem
{
public:
	FCanvasTileItem(const FVector2D& InPosition, const FTexture* InTexture, const FLinearColor& InColor);
	FCanvasTileItem(const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InSize, const FLinearColor& InColor);
	FCanvasTileItem(const FVector2D& InPosition, const FVector2D& InSize, const FLinearColor& InColor);
	FCanvasTileItem(const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InUV0, const FVector2D& InUV1, const FLinearColor& InColor);
	FCanvasTileItem(const FVector2D& InPosition, const FTexture* InTexture, const FVector2D& InSize, const FVector2D& InUV0, const FVector2D& InUV1, const FLinearColor& InColor);
	FCanvasTileItem(const FVector2D& InPosition, const FMaterialRenderProxy* InMaterialRenderProxy, const FVector2D& InSize);
	FCanvasTileItem(const FVector2D& InPosition, const FMaterialRenderProxy* InMaterialRenderProxy, const FVector2D& InSize, const FVector2D& InUV0, const FVector2D& InUV1);
	virtual void Draw(FCanvas* InCanvas) override;
	FVector2D Size; float Z; FVector2D UV0; FVector2D UV1; const FTexture* Texture; const FMaterialRenderProxy* MaterialRenderProxy; FRotator Rotation; FVector2D PivotPoint;
};
class FCanvasTextItemBase : public FCanvasItem
{
public:
	FCanvasTextItemBase(const FVector2D& InPosition, const FLinearColor& InColor);
	bool bCentreX; bool bCentreY; FVector2D DrawnSize; bool bOutlined; FLinearColor OutlineColor; FVector2D ShadowOffset; FLinearColor ShadowColor; float HorizSpacingAdjust;
	FVector2D Scale; FFontRenderInfo FontRenderInfo; float Depth; bool bDontCorrectStereoscopic;
	void EnableShadow(const FLinearColor& InColor, const FVector2D& InOffset = FVector2D(1.0f, 1.0f));
	void DisableShadow();
};
class FCanvasTextItem : public FCanvasTextItemBase
{
public:
	FCanvasTextItem(const FVector2D& InPosition, const FText& InText, const class UFont* InFont, const FLinearColor& InColor);
	virtual void Draw(FCanvas* InCanvas) override;
	FText Text; const UFont* Font;
};
class FCanvasLineItem : public FCanvasItem
{
public:
	FCanvasLineItem(); FCanvasLineItem(const FVector2D& InPosition, const FVector2D& InEndPos); FCanvasLineItem(const FVector& InPosition, const FVector& InEndPos);
	virtual void Draw(FCanvas* InCanvas) override;
	void SetEndPos(const FVector2D& InEndPos);
	FVector Origin; FVector EndPos; float LineThickness;
};
class FCanvasBoxItem : public FCanvasItem
{
public:
	FCanvasBoxItem(const FVector2D& InPosition, const FVector2D& InSize);
	virtual void Draw(FCanvas* InCanvas) override;
	FVector2D Size; float LineThickness;
};
class FCanvasTriangleItem : public FCanvasItem
{
public:
	FCanvasTriangleItem(const FVector2D& InPointA, const FVector2D& InPointB, const FVector2D& InPointC, const FTexture* InTexture);
	FCanvasTriangleItem(const FVector2D& InPointA, const FVector2D& InPointB, const FVector2D& InPointC, const FVector2D& InTexCoordPointA, const FVector2D& InTexCoordPointB, const FVector2D& InTexCoordPointC, const FTexture* InTexture);
	FCanvasTriangleItem(const TArray<FCanvasUVTri>& InTriangleList, const FTexture* InTexture);
	virtual void Draw(FCanvas* InCanvas) override;
	void SetColor(const FLinearColor& InColor);
	const FTexture* Texture; const FMaterialRenderProxy* MaterialRenderProxy; TArray<FCanvasUVTri> TriangleList;
};
class FCanvasNGonItem : public FCanvasItem
{
public:
	FCanvasNGonItem(const FVector2D& InPosition, const FVector2D& InRadius, int32 InNumSides, const FLinearColor& InColor);
	FCanvasNGonItem(const FVector2D& InPosition, const FVector2D& InRadius, int32 InNumSides, const FTexture* InTexture, const FLinearColor& InColor);
	virtual void Draw(FCanvas* InCanvas) override;
};
