// Тени Парижа — интерфейс: подсказки, здоровье, миникарта, диалоги, альбом портретов, меню паузы.
// Владелец модуля: агент «жители, сюжет и интерфейс». Рисуется через Canvas — без ассетов виджетов.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPHUD.generated.h"

class ATPCharacter;
class UFont;
class UTPStorySubsystem;
class UTPIdentitySubsystem;
struct FKey;

/** диалог, ждущий своей очереди (если сюжет показывает новый, пока открыт прежний) */
struct FTPQueuedDialog
{
	TArray<FString> Lines;
	TFunction<void()> OnDone;
};

UCLASS()
class TENIPARIZHA_API ATPHUD : public AHUD
{
	GENERATED_BODY()
public:
	ATPHUD();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

	/** всплывающее сообщение внизу экрана */
	UFUNCTION(BlueprintCallable, Category = "Интерфейс") void Toast(const FString& Text, float Seconds = 3.5f);
	/** реплики диалога «Имя|Текст»; вызывается сюжетом */
	UFUNCTION(BlueprintCallable, Category = "Интерфейс") void ShowDialog(const TArray<FString>& Lines);
	UFUNCTION(BlueprintPure, Category = "Интерфейс") bool IsDialogOpen() const { return DialogIndex >= 0; }

	/** диалог с действием по завершении (C++); если диалог уже открыт — встаёт в очередь */
	void ShowDialogThen(const TArray<FString>& Lines, TFunction<void()> OnDone);
	/** меню выбора (гонки Лиги); OnChosen получает индекс пункта или -1 при отмене */
	void ShowChoice(const FString& Title, const TArray<FString>& Items, const TArray<FString>& Details, const TArray<bool>& Enabled, TFunction<void(int32)> OnChosen);
	bool IsChoiceOpen() const { return bChoice; }
	bool IsAlbumOpen() const { return bAlbum; }
	bool IsPauseMenuOpen() const { return bPaused; }
	bool IsHelpOpen() const { return bHelp; }
	/** открыт ли экран, который забирает ввод у героя */
	bool IsBlockingUI() const { return IsDialogOpen() || bChoice || bAlbum || bPaused || bHelp; }

protected:
	UFUNCTION() void HandleInteract();
	UFUNCTION() void HandleAlbum();
	UFUNCTION() void HandlePause();
	UFUNCTION() void HandleHelp();
	/** сообщения режиссёра боя («Вас заметили!» …) */
	UFUNCTION() void HandleCombatMessage(const FString& Text);
	/** герой возродился — затемнение экрана */
	UFUNCTION() void HandleRespawned();

	void BindHero(ATPCharacter* NewHero);
	void BindDirector();
	void UpdateInput();
	void ApplyInputLock();
	void AdvanceDialog();
	void FinishDialog();
	void CloseChoice(int32 Result);
	void OpenAlbum(bool bFromPause);
	void CloseAlbum();
	void OpenHelp(bool bFromPause);
	void CloseHelp();
	void OpenPause();
	void ClosePause();
	void ActivatePauseItem(int32 Item);
	void GetPauseItems(TArray<FString>& OutItems, TArray<int32>& OutIds) const;
	void SetOverlayPause(bool bOn);
	bool KeyHit(const FKey& Key) const;
	bool AnyHit(std::initializer_list<FKey> Keys) const;
	void RefreshActorCache();
	void UpdateFonts();

	/* слои интерфейса */
	void DrawTopPanel();
	void DrawRaceHUD();
	void DrawClock();
	void DrawVitals();
	void DrawMinimap();
	void DrawWorldMarkers();
	void DrawPrompts();
	void DrawToast();
	void DrawCountdown();
	void DrawFade();
	void DrawDialog();
	void DrawChoice();
	void DrawHelp();
	void DrawAlbum();
	void DrawPause();

	/* примитивы */
	void Rect(float X, float Y, float W, float H, const FLinearColor& Color);
	void Panel(float X, float Y, float W, float H, float Alpha = 0.74f, bool bBorder = true);
	/** Align: 0 — влево, 1 — по центру, 2 — вправо */
	void Txt(const FString& S, float X, float Y, UFont* Font, float Scale, const FLinearColor& Color, int32 Align = 0, bool bShadow = true);
	float TextW(const FString& S, UFont* Font, float Scale);
	float LineH(UFont* Font, float Scale);
	TArray<FString> Wrap(const FString& S, UFont* Font, float Scale, float MaxW);
	/** текст с переносами; возвращает высоту */
	float TxtWrapped(const FString& S, float X, float Y, float MaxW, UFont* Font, float Scale, const FLinearColor& Color, int32 MaxLines = 100);
	void Tri(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FLinearColor& Color);
	void Disc(float CX, float CY, float RX, float RY, const FLinearColor& Color, int32 Sides = 24);
	void Ring(float CX, float CY, float R, float Thick, const FLinearColor& Color, int32 Sides = 32);
	void Diamond(float CX, float CY, float R, const FLinearColor& Color);
	void MedalIcon(float CX, float CY, float R, int32 Medal);
	/** ForceStatus >= 0 — рисовать как при этом статусе альбома (портрет собеседника в диалоге) */
	void DrawPortrait(int32 Index, float X, float Y, float W, float H, int32 ForceStatus = -1);
	/** житель-собеседник по имени из реплики «Имя|Текст» (или -1) */
	int32 FindSpeaker(const FString& Speaker) const;
	void GildedFrame(float X, float Y, float W, float H);
	bool ToScreen(const FVector& World, FVector2D& Out) const;

	UTPStorySubsystem* Story() const;
	const UTPIdentitySubsystem* Identities() const;
	ATPCharacter* Hero() const;

	/* очередь диалогов */
	TArray<FTPQueuedDialog> DialogQueue;

	FString ToastText; float ToastTime = 0.f;
	TArray<FString> Dialog; int32 DialogIndex = -1;
	float DialogReveal = 0.f;
	TFunction<void()> DialogDone;
	bool bHelp = false, bAlbum = false, bPaused = false;
	/** кадр, в котором интерфейс уже обработал нажатие (опрос клавиш в этом кадре пропускается) */
	uint64 UIOpenFrame = 0;
	/** время (FPlatformTime) последнего закрытия экрана: короткая защита от повторного срабатывания той же клавиши */
	double UICloseTime = -100.0;
	void MarkUIClosed();
	bool InCloseGrace() const;

	bool bChoice = false;
	FString ChoiceTitle;
	TArray<FString> ChoiceItems;
	TArray<FString> ChoiceDetails;
	TArray<bool> ChoiceEnabled;
	int32 ChoiceSel = 0;
	TFunction<void(int32)> ChoiceDone;

	bool bAlbumFromPause = false;
	bool bHelpFromPause = false;
	/** игру поставил на паузу альбом или справка (а не меню паузы) */
	bool bPausedByOverlay = false;
	int32 AlbumSel = 0;
	int32 PauseSel = 0;

	TWeakObjectPtr<ATPCharacter> BoundHero;
	bool bDirectorBound = false;
	bool bLockedByHUD = false;
	double LastRealTime = -1.0;
	double RealTime = 0.0;
	float Dt = 0.f;
	float UI = 1.f;
	float SW = 1280.f, SH = 720.f;
	float CacheTimer = 0.f;
	float HintTime = 14.f;
	float FadeTime = 0.f;
	float DamageFlash = 0.f;
	float LastHealth = -1.f;
	float CountdownShown = 0.f;
	TArray<TWeakObjectPtr<AActor>> Guards;
	TWeakObjectPtr<AActor> Environment;

	/* шрифты: Roboto из движка (кириллица есть); масштабы под нужную высоту строки */
	UPROPERTY(Transient) TObjectPtr<UFont> FontS;
	UPROPERTY(Transient) TObjectPtr<UFont> FontM;
	UPROPERTY(Transient) TObjectPtr<UFont> FontL;
	float ScS = 1.f, ScM = 1.f, ScL = 1.f, ScXL = 1.f;
	float BaseHS = 0.f, BaseHM = 0.f, BaseHL = 0.f;
};
