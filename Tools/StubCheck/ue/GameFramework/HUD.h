// Макет GameFramework/HUD.h (UE 4.27)
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
class APlayerController; class UCanvas; class UFont; class UTexture; class UMaterialInterface; class APawn;
class AHUD : public AActor
{
public:
	typedef AActor Super; typedef AHUD ThisClass; static UClass* StaticClass();
	AHUD(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	APlayerController* PlayerOwner;
	uint8 bLostFocusPaused : 1; uint8 bShowHUD : 1; uint8 bShowDebugInfo : 1; uint8 bShowHitBoxDebugInfo : 1; uint8 bShowOverlays : 1; uint8 bEnableDebugTextShadow : 1;
	TArray<AActor*> PostRenderedActors; TArray<FName> DebugDisplay; TArray<FName> ToggledDebugCategories;
	UCanvas* Canvas; UCanvas* DebugCanvas;
	virtual void DrawHUD();
	virtual void PostRender();
	virtual void ShowHUD();
	virtual void ShowDebug(FName DebugType = NAME_None);
	virtual void DrawActorOverlays(FVector Viewpoint, FRotator ViewRotation);
	virtual void PostInitializeComponents() override;
	virtual void NotifyHitBoxClick(FName BoxName); virtual void NotifyHitBoxRelease(FName BoxName); virtual void NotifyHitBoxBeginCursorOver(FName BoxName); virtual void NotifyHitBoxEndCursorOver(FName BoxName);
	void DrawText(const FString& Text, FLinearColor TextColor, float ScreenX, float ScreenY, UFont* Font = nullptr, float Scale = 1.f, bool bScalePosition = false);
	void DrawRect(FLinearColor RectColor, float ScreenX, float ScreenY, float ScreenW, float ScreenH);
	void DrawLine(float StartScreenX, float StartScreenY, float EndScreenX, float EndScreenY, FLinearColor LineColor, float LineThickness = 0.f);
	void DrawTexture(UTexture* Texture, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float TextureU, float TextureV, float TextureUWidth, float TextureVHeight, FLinearColor TintColor = FLinearColor::White, EBlendMode BlendMode = BLEND_Translucent, float Scale = 1.f, bool bScalePosition = false, float Rotation = 0.f, FVector2D RotPivot = FVector2D::ZeroVector);
	void DrawTextureSimple(UTexture* Texture, float ScreenX, float ScreenY, float Scale = 1.f, bool bScalePosition = false);
	void DrawMaterial(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float MaterialU, float MaterialV, float MaterialUWidth, float MaterialVHeight, float Scale = 1.f, bool bScalePosition = false, float Rotation = 0.f, FVector2D RotPivot = FVector2D::ZeroVector);
	void DrawMaterialSimple(UMaterialInterface* Material, float ScreenX, float ScreenY, float ScreenW, float ScreenH, float Scale = 1.f, bool bScalePosition = false);
	void GetTextSize(const FString& Text, float& OutWidth, float& OutHeight, UFont* Font = nullptr, float Scale = 1.f) const;
	FVector Project(FVector Location) const;
	void Deproject(float ScreenX, float ScreenY, FVector& WorldPosition, FVector& WorldDirection) const;
	void AddHitBox(FVector2D Position, FVector2D Size, FName InName, bool bConsumesInput, int32 Priority = 0);
	APlayerController* GetOwningPlayerController() const; APawn* GetOwningPawn() const;
	void AddPostRenderedActor(AActor* A); void RemovePostRenderedActor(AActor* A);
	static const FColor WhiteColor; static const FColor GreenColor; static const FColor RedColor;
protected:
	virtual void BeginPlay() override;
};
