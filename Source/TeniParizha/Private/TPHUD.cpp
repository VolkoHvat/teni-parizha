// Тени Парижа — интерфейс на Canvas: панели сюжета и гонок, часы, здоровье, миникарта, отметки над головами,
// диалоги, меню гонок, справка, пауза и альбом «Лица квартала» (128 портретов).
#include "TPHUD.h"
#include "TeniParizha.h"
#include "TPCharacter.h"
#include "TPTraversalComponent.h"
#include "TPCombatComponent.h"
#include "TPEnvironment.h"
#include "TPStorySubsystem.h"
#include "TPIdentitySubsystem.h"
#include "TPCitizen.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "CanvasItem.h"
#include "RenderUtils.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"

// Стражу и режиссёра боя пишет другой агент: подключаем, только если заголовки уже есть
#if __has_include("TPCombatDirector.h")
#include "TPCombatDirector.h"
#define TP_HUD_HAS_DIRECTOR 1
#else
#define TP_HUD_HAS_DIRECTOR 0
#endif
#if __has_include("TPGuard.h")
#include "TPGuard.h"
#define TP_HUD_HAS_GUARD 1
#else
#define TP_HUD_HAS_GUARD 0
#endif

namespace TPUI
{
	/** цвет из sRGB 0..255 */
	static FLinearColor C(int32 R, int32 G, int32 B, float A = 1.f)
	{
		FLinearColor L(FColor((uint8)FMath::Clamp(R, 0, 255), (uint8)FMath::Clamp(G, 0, 255), (uint8)FMath::Clamp(B, 0, 255)));
		L.A = A;
		return L;
	}
	static FLinearColor WithA(FLinearColor L, float A) { L.A = A; return L; }
	static FLinearColor Mix(const FLinearColor& A, const FLinearColor& B, float T) { return A + (B - A) * FMath::Clamp(T, 0.f, 1.f); }

	static FLinearColor Gold() { return C(214, 176, 90); }
	static FLinearColor GoldBright() { return C(244, 214, 130); }
	static FLinearColor Paper() { return C(244, 238, 228); }
	static FLinearColor Dim() { return C(190, 180, 170); }
	static FLinearColor Ink() { return C(40, 30, 30); }
	static FLinearColor Blood() { return C(200, 50, 56); }

	/** состояния паркура (свои подписи: UMETA(DisplayName) в сборке игры недоступны) */
	static FString TraversalText(ETPTraversal S, bool bFree, bool bCrouch)
	{
		switch (S)
		{
		case ETPTraversal::Ground:  return bCrouch ? TEXT("Крадётесь · у края — спуск в вис") : (bFree ? TEXT("Свободный бег") : FString());
		case ETPTraversal::Jump:
		case ETPTraversal::Air:     return TEXT("Полёт");
		case ETPTraversal::Roll:    return TEXT("Кувырок");
		case ETPTraversal::Climb:   return TEXT("Лазание · W/S — вверх/вниз, A/D — вбок, Пробел — отпрыгнуть, Q — отпустить");
		case ETPTraversal::Hang:    return TEXT("Вис · W — подтянуться, A/D — по карнизу, Пробел — прыжок, Q — отпустить");
		case ETPTraversal::ClimbUp: return TEXT("Подтягивание");
		case ETPTraversal::Drop:    return TEXT("Спуск в вис");
		case ETPTraversal::Vault:   return TEXT("Перемах");
		case ETPTraversal::WallRun: return TEXT("Бег по стене");
		case ETPTraversal::Turn:    return TEXT("Разворот");
		case ETPTraversal::Hop:     return TEXT("Перескок");
		default:                    return FString();
		}
	}

	static const TCHAR* AbilityShort[UTPStorySubsystem::NumAbilities] = {TEXT("Хв"), TEXT("Пк"), TEXT("Пр"), TEXT("Пм"), TEXT("Б1"), TEXT("Б2")};

	/** справка F1: клавиша — действие */
	static const TCHAR* HelpKeys[][2] = {
		{TEXT("W A S D"), TEXT("бег; на стене — лазание, в висе — вдоль карниза")},
		{TEXT("Мышь"), TEXT("камера")},
		{TEXT("Shift / ПКМ"), TEXT("свободный бег: герой сам лезет на стены, перемахивает и прыгает")},
		{TEXT("Пробел"), TEXT("прыжок; со стены — отпрыгнуть")},
		{TEXT("Ctrl / C"), TEXT("присесть, красться; у края крыши — спуск в вис")},
		{TEXT("Q"), TEXT("отпустить руки")},
		{TEXT("R"), TEXT("разворот на 180°")},
		{TEXT("ЛКМ / F"), TEXT("удар (серия из трёх); со спины — тихое устранение")},
		{TEXT("E"), TEXT("контратака, когда стражник замахивается (красный знак)")},
		{TEXT("Alt"), TEXT("уклонение от удара и от пули")},
		{TEXT("G"), TEXT("говорить с жителем, гонки Лиги у бегунов; листать диалог")},
		{TEXT("Tab"), TEXT("альбом «Лица квартала»")},
		{TEXT("Esc / P"), TEXT("пауза")},
		{TEXT("F1"), TEXT("эта справка")}};
	static const TCHAR* HelpTips[] = {
		TEXT("Беги прямо в стену — залезешь на любое здание. Shift + Пробел у края крыши — прыжок через улицу."),
		TEXT("Сбитые на бегу прохожие кричат и зовут стражу. Шагом в толпе стража замечает вас хуже."),
		TEXT("Бегуны Лиги крыш отмечены золотыми ромбами. За медали Мэтр Огюстен пишет новые портреты.")};

	/** нарисовать список треугольников одним элементом (цвета заданы в вершинах) */
	static void DrawTriList(UCanvas* Canvas, const TArray<FCanvasUVTri>& Tris)
	{
		if (!Canvas || Tris.Num() == 0) return;
		FCanvasTriangleItem Item(Tris, GWhiteTexture);
		Item.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Item);
	}
	static FCanvasUVTri MakeTri(const FVector2D& A, const FVector2D& B, const FVector2D& Cc, const FLinearColor& Col)
	{
		FCanvasUVTri T;
		T.V0_Pos = A;  T.V0_UV = FVector2D::ZeroVector; T.V0_Color = Col;
		T.V1_Pos = B;  T.V1_UV = FVector2D::ZeroVector; T.V1_Color = Col;
		T.V2_Pos = Cc; T.V2_UV = FVector2D::ZeroVector; T.V2_Color = Col;
		return T;
	}
}


/* ======================= жизненный цикл ======================= */

ATPHUD::ATPHUD()
{
}

void ATPHUD::BeginPlay()
{
	Super::BeginPlay();
	BindDirector();
	LastRealTime = -1.0;
}

void ATPHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	BindHero(nullptr);
#if TP_HUD_HAS_DIRECTOR
	if (UWorld* W = GetWorld())
		if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
			Dir->OnCombatMessage.RemoveDynamic(this, &ATPHUD::HandleCombatMessage);
#endif
	bDirectorBound = false;
	Super::EndPlay(EndPlayReason);
}

void ATPHUD::BindDirector()
{
	if (bDirectorBound) return;
#if TP_HUD_HAS_DIRECTOR
	if (UWorld* W = GetWorld())
		if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
		{
			// сообщения боя директор сам показывает через Toast — повторная подписка дала бы двойной вывод
			bDirectorBound = true;
		}
#else
	bDirectorBound = true;
#endif
}

void ATPHUD::BindHero(ATPCharacter* NewHero)
{
	if (ATPCharacter* Old = BoundHero.Get())
	{
		Old->OnInteractPressed.RemoveDynamic(this, &ATPHUD::HandleInteract);
		Old->OnAlbumPressed.RemoveDynamic(this, &ATPHUD::HandleAlbum);
		Old->OnPausePressed.RemoveDynamic(this, &ATPHUD::HandlePause);
		Old->OnHelpPressed.RemoveDynamic(this, &ATPHUD::HandleHelp);
		if (Old->Combat) Old->Combat->OnRespawned.RemoveDynamic(this, &ATPHUD::HandleRespawned);
		if (bLockedByHUD) Old->bInputLocked = false;
	}
	bLockedByHUD = false;
	BoundHero = NewHero;
	LastHealth = -1.f;
	if (!NewHero) return;
	NewHero->OnInteractPressed.AddUniqueDynamic(this, &ATPHUD::HandleInteract);
	NewHero->OnAlbumPressed.AddUniqueDynamic(this, &ATPHUD::HandleAlbum);
	NewHero->OnPausePressed.AddUniqueDynamic(this, &ATPHUD::HandlePause);
	NewHero->OnHelpPressed.AddUniqueDynamic(this, &ATPHUD::HandleHelp);
	if (NewHero->Combat) NewHero->Combat->OnRespawned.AddUniqueDynamic(this, &ATPHUD::HandleRespawned);
}

UTPStorySubsystem* ATPHUD::Story() const
{
	UWorld* W = GetWorld();
	return W ? W->GetSubsystem<UTPStorySubsystem>() : nullptr;
}

const UTPIdentitySubsystem* ATPHUD::Identities() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UTPIdentitySubsystem>() : nullptr;
}

ATPCharacter* ATPHUD::Hero() const
{
	return BoundHero.Get();
}

void ATPHUD::RefreshActorCache()
{
	UWorld* W = GetWorld();
	if (!W) return;
	Guards.Reset();
#if TP_HUD_HAS_GUARD
	bool bFromDirector = false;
#if TP_HUD_HAS_DIRECTOR
	if (UTPCombatDirector* Dir = W->GetSubsystem<UTPCombatDirector>())
	{
		for (ATPGuard* G : Dir->GetGuards()) if (G) Guards.Add(G);
		bFromDirector = Guards.Num() > 0;
	}
#endif
	if (!bFromDirector)
		for (TActorIterator<ATPGuard> It(W); It; ++It) Guards.Add(*It);
#endif
	if (!Environment.IsValid())
		for (TActorIterator<ATPEnvironment> It(W); It; ++It) { Environment = *It; break; }
}

/* ======================= события ======================= */

void ATPHUD::Toast(const FString& Text, float Seconds)
{
	ToastText = Text;
	ToastTime = FMath::Max(0.5f, Seconds);
}

void ATPHUD::HandleCombatMessage(const FString& Text)
{
	Toast(Text, 2.8f);
}

void ATPHUD::HandleRespawned()
{
	FadeTime = 1.4f;
}

void ATPHUD::MarkUIClosed()
{
	UICloseTime = FPlatformTime::Seconds();
	UIOpenFrame = GFrameCounter;
}

bool ATPHUD::InCloseGrace() const
{
	return FPlatformTime::Seconds() - UICloseTime < 0.25;
}

void ATPHUD::HandleInteract()
{
	// обычно при открытом диалоге ввод героя заблокирован и событие не приходит; на всякий случай
	if (DialogIndex >= 0 && GFrameCounter != UIOpenFrame)
	{
		AdvanceDialog();
		UIOpenFrame = GFrameCounter;
	}
}

void ATPHUD::HandleAlbum()
{
	if (GFrameCounter == UIOpenFrame) return;
	if (bAlbum) { CloseAlbum(); return; }
	if (InCloseGrace() || IsDialogOpen() || bChoice || bHelp || bPaused) return;
	OpenAlbum(false);
}

void ATPHUD::HandlePause()
{
	if (GFrameCounter == UIOpenFrame) return;
	// Esc закрывает верхний экран, а если открытых нет — ставит паузу
	if (bHelp) { CloseHelp(); return; }
	if (bAlbum) { CloseAlbum(); return; }
	if (bChoice) { CloseChoice(-1); return; }
	if (DialogIndex >= 0) { FinishDialog(); return; }
	if (bPaused) { ClosePause(); return; }
	if (InCloseGrace()) return;
	OpenPause();
}

void ATPHUD::HandleHelp()
{
	if (GFrameCounter == UIOpenFrame) return;
	if (bHelp) { CloseHelp(); return; }
	if (bAlbum || InCloseGrace()) return;
	OpenHelp(bPaused);
}

/* ======================= диалоги и меню ======================= */

void ATPHUD::ShowDialog(const TArray<FString>& Lines)
{
	ShowDialogThen(Lines, TFunction<void()>());
}

void ATPHUD::ShowDialogThen(const TArray<FString>& Lines, TFunction<void()> OnDone)
{
	if (Lines.Num() == 0)
	{
		if (OnDone) OnDone();
		return;
	}
	if (DialogIndex >= 0)
	{
		FTPQueuedDialog Q;
		Q.Lines = Lines;
		Q.OnDone = MoveTemp(OnDone);
		DialogQueue.Add(MoveTemp(Q));
		return;
	}
	Dialog = Lines;
	DialogIndex = 0;
	DialogReveal = 0.f;
	DialogDone = MoveTemp(OnDone);
	UIOpenFrame = GFrameCounter;
}

void ATPHUD::AdvanceDialog()
{
	if (DialogIndex < 0) return;
	// первое нажатие дописывает реплику целиком
	int32 Bar = INDEX_NONE;
	const int32 TextLen = Dialog.IsValidIndex(DialogIndex) ? (Dialog[DialogIndex].FindChar(TEXT('|'), Bar) ? Dialog[DialogIndex].Len() - Bar - 1 : Dialog[DialogIndex].Len()) : 0;
	if (DialogReveal < TextLen)
	{
		DialogReveal = 100000.f;
		return;
	}
	++DialogIndex;
	DialogReveal = 0.f;
	if (DialogIndex >= Dialog.Num()) FinishDialog();
}

void ATPHUD::FinishDialog()
{
	TFunction<void()> Done = MoveTemp(DialogDone);
	DialogDone = nullptr;
	Dialog.Reset();
	DialogIndex = -1;
	MarkUIClosed();
	if (Done) Done();
	// следующий диалог из очереди (если действие по завершении не открыло свой)
	if (DialogIndex < 0 && DialogQueue.Num() > 0)
	{
		FTPQueuedDialog Q = MoveTemp(DialogQueue[0]);
		DialogQueue.RemoveAt(0);
		ShowDialogThen(Q.Lines, MoveTemp(Q.OnDone));
	}
}

void ATPHUD::ShowChoice(const FString& Title, const TArray<FString>& Items, const TArray<FString>& Details, const TArray<bool>& Enabled, TFunction<void(int32)> OnChosen)
{
	if (Items.Num() == 0)
	{
		if (OnChosen) OnChosen(-1);
		return;
	}
	bChoice = true;
	ChoiceTitle = Title;
	ChoiceItems = Items;
	ChoiceDetails = Details;
	ChoiceEnabled = Enabled;
	ChoiceDetails.SetNum(Items.Num());
	while (ChoiceEnabled.Num() < Items.Num()) ChoiceEnabled.Add(true);
	ChoiceDone = MoveTemp(OnChosen);
	ChoiceSel = 0;
	for (int32 I = 0; I < ChoiceItems.Num(); ++I) if (ChoiceEnabled[I]) { ChoiceSel = I; break; }
	UIOpenFrame = GFrameCounter;
}

void ATPHUD::CloseChoice(int32 Result)
{
	TFunction<void(int32)> Done = MoveTemp(ChoiceDone);
	ChoiceDone = nullptr;
	bChoice = false;
	ChoiceItems.Reset();
	ChoiceDetails.Reset();
	ChoiceEnabled.Reset();
	MarkUIClosed();
	if (Done) Done(Result);
}

void ATPHUD::SetOverlayPause(bool bOn)
{
	if (bOn)
	{
		if (!UGameplayStatics::IsGamePaused(this))
		{
			UGameplayStatics::SetGamePaused(this, true);
			bPausedByOverlay = true;
		}
	}
	else if (bPausedByOverlay)
	{
		bPausedByOverlay = false;
		if (!bPaused) UGameplayStatics::SetGamePaused(this, false);
	}
}

void ATPHUD::OpenAlbum(bool bFromPause)
{
	bAlbum = true;
	bAlbumFromPause = bFromPause;
	if (!bFromPause) SetOverlayPause(true);
	const UTPStorySubsystem* S = Story();
	if (S && S->GetAlbumStatus(AlbumSel) == 0)
		for (int32 I = 0; I < TPCitizens::Num; ++I) if (S->GetAlbumStatus(I) > 0) { AlbumSel = I; break; }
	UIOpenFrame = GFrameCounter;
}

void ATPHUD::CloseAlbum()
{
	bAlbum = false;
	if (!bAlbumFromPause) SetOverlayPause(false);
	bAlbumFromPause = false;
	MarkUIClosed();
}

void ATPHUD::OpenHelp(bool bFromPause)
{
	bHelp = true;
	bHelpFromPause = bFromPause;
	if (!bFromPause) SetOverlayPause(true);
	HintTime = 0.f;
	UIOpenFrame = GFrameCounter;
}

void ATPHUD::CloseHelp()
{
	bHelp = false;
	if (!bHelpFromPause) SetOverlayPause(false);
	bHelpFromPause = false;
	MarkUIClosed();
}

void ATPHUD::OpenPause()
{
	bPaused = true;
	PauseSel = 0;
	UGameplayStatics::SetGamePaused(this, true);
	bPausedByOverlay = false;
	UIOpenFrame = GFrameCounter;
}

void ATPHUD::ClosePause()
{
	bPaused = false;
	UGameplayStatics::SetGamePaused(this, false);
	MarkUIClosed();
}

void ATPHUD::GetPauseItems(TArray<FString>& OutItems, TArray<int32>& OutIds) const
{
	const UTPStorySubsystem* S = Story();
	OutItems.Reset(); OutIds.Reset();
	OutItems.Add(TEXT("Продолжить")); OutIds.Add(0);
	OutItems.Add(TEXT("Альбом «Лица квартала»")); OutIds.Add(1);
	OutItems.Add(TEXT("Управление")); OutIds.Add(2);
	if (S && S->IsRaceActive()) { OutItems.Add(TEXT("Сойти с трассы")); OutIds.Add(3); }
	OutItems.Add(TEXT("Сохранить игру")); OutIds.Add(4);
	OutItems.Add(TEXT("Выйти из игры")); OutIds.Add(5);
}

void ATPHUD::ActivatePauseItem(int32 Item)
{
	UTPStorySubsystem* S = Story();
	switch (Item)
	{
	case 0: ClosePause(); break;
	case 1: OpenAlbum(true); break;
	case 2: OpenHelp(true); break;
	case 3:
		ClosePause();
		if (S) S->AbortRace();
		break;
	case 4:
		if (S) { S->SaveProgress(); Toast(TEXT("Игра сохранена"), 2.f); }
		break;
	case 5:
		if (S) S->SaveProgress();
		UKismetSystemLibrary::QuitGame(this, PlayerOwner, EQuitPreference::Quit, false);
		break;
	default: break;
	}
}

/* ======================= ввод ======================= */

bool ATPHUD::KeyHit(const FKey& Key) const
{
	return PlayerOwner && PlayerOwner->WasInputKeyJustPressed(Key);
}

bool ATPHUD::AnyHit(std::initializer_list<FKey> Keys) const
{
	for (const FKey& K : Keys) if (KeyHit(K)) return true;
	return false;
}

void ATPHUD::UpdateInput()
{
	if (!PlayerOwner || GFrameCounter == UIOpenFrame) return;
	// пока игра на паузе, привязки BindAction не срабатывают — клавиши меню опрашиваем сами
	const bool bUp = AnyHit({EKeys::W, EKeys::Up, EKeys::Gamepad_DPad_Up, EKeys::Gamepad_LeftStick_Up});
	const bool bDown = AnyHit({EKeys::S, EKeys::Down, EKeys::Gamepad_DPad_Down, EKeys::Gamepad_LeftStick_Down});
	const bool bLeft = AnyHit({EKeys::A, EKeys::Left, EKeys::Gamepad_DPad_Left, EKeys::Gamepad_LeftStick_Left});
	const bool bRight = AnyHit({EKeys::D, EKeys::Right, EKeys::Gamepad_DPad_Right, EKeys::Gamepad_LeftStick_Right});
	const bool bOK = AnyHit({EKeys::SpaceBar, EKeys::Enter, EKeys::G, EKeys::Gamepad_FaceButton_Bottom});
	const bool bEsc = AnyHit({EKeys::Escape, EKeys::Gamepad_Special_Right});

	if (bHelp)
	{
		if (bEsc || AnyHit({EKeys::F1, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right}) || bOK) CloseHelp();
		return;
	}
	if (bAlbum)
	{
		const int32 N = TPCitizens::Num, Cols = 16;
		if (bEsc || AnyHit({EKeys::Tab, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right, EKeys::Gamepad_Special_Left})) { CloseAlbum(); return; }
		if (bLeft) AlbumSel = (AlbumSel + N - 1) % N;
		if (bRight) AlbumSel = (AlbumSel + 1) % N;
		if (bUp) AlbumSel = (AlbumSel + N - Cols) % N;
		if (bDown) AlbumSel = (AlbumSel + Cols) % N;
		// Q/E — к предыдущему/следующему знакомому
		const bool bPrev = AnyHit({EKeys::Q, EKeys::PageUp, EKeys::Gamepad_LeftShoulder});
		const bool bNext = AnyHit({EKeys::E, EKeys::PageDown, EKeys::Gamepad_RightShoulder});
		if ((bPrev || bNext) && Story())
		{
			const UTPStorySubsystem* S = Story();
			for (int32 K = 1; K < N; ++K)
			{
				const int32 I = (AlbumSel + (bNext ? K : N - K)) % N;
				if (S->GetAlbumStatus(I) > 0) { AlbumSel = I; break; }
			}
		}
		return;
	}
	if (bPaused)
	{
		TArray<FString> Items; TArray<int32> Ids;
		GetPauseItems(Items, Ids);
		PauseSel = FMath::Clamp(PauseSel, 0, Items.Num() - 1);
		if (bEsc || AnyHit({EKeys::P, EKeys::Gamepad_FaceButton_Right})) { ClosePause(); return; }
		if (bUp) PauseSel = (PauseSel + Items.Num() - 1) % Items.Num();
		if (bDown) PauseSel = (PauseSel + 1) % Items.Num();
		if (bOK || KeyHit(EKeys::E)) ActivatePauseItem(Ids[PauseSel]);
		return;
	}
	if (bChoice)
	{
		const int32 N = ChoiceItems.Num();
		if (bEsc || AnyHit({EKeys::Q, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right})) { CloseChoice(-1); return; }
		if (N == 0) { CloseChoice(-1); return; }
		if (bUp) ChoiceSel = (ChoiceSel + N - 1) % N;
		if (bDown) ChoiceSel = (ChoiceSel + 1) % N;
		if (bOK || KeyHit(EKeys::E))
		{
			if (ChoiceEnabled.IsValidIndex(ChoiceSel) && ChoiceEnabled[ChoiceSel]) CloseChoice(ChoiceSel);
			else Toast(TEXT("Эта гонка пока закрыта"), 1.6f);
		}
		return;
	}
	if (DialogIndex >= 0)
	{
		if (bEsc) { FinishDialog(); return; }
		if (bOK || AnyHit({EKeys::E, EKeys::LeftMouseButton})) AdvanceDialog();
		return;
	}
	// запасная клавиша паузы (в редакторе Esc останавливает игру)
	if (KeyHit(EKeys::P) && !InCloseGrace()) OpenPause();
}

void ATPHUD::ApplyInputLock()
{
	ATPCharacter* H = BoundHero.Get();
	if (!H) return;
	const UTPStorySubsystem* S = Story();
	const bool bWant = IsBlockingUI() || InCloseGrace() || (S && S->WantsInputLock());
	if (bWant)
	{
		H->bInputLocked = true;
		bLockedByHUD = true;
	}
	else if (bLockedByHUD)
	{
		H->bInputLocked = false;
		bLockedByHUD = false;
	}
}

/* ======================= примитивы ======================= */

void ATPHUD::UpdateFonts()
{
	if (!GEngine) return;
	if (!FontS) FontS = GEngine->GetSmallFont();
	if (!FontM) FontM = GEngine->GetMediumFont();
	if (!FontL) FontL = GEngine->GetLargeFont();
	if (!FontM) FontM = FontS;
	if (!FontL) FontL = FontM;
	// Small/Medium/Large в движке часто один и тот же Roboto — меряем высоту строки и подбираем масштаб
	auto Measure = [this](UFont* F) -> float
	{
		if (!F) return 0.f;
		float W = 0.f, H = 0.f;
		GetTextSize(TEXT("ЙАgy"), W, H, F, 1.f);
		return H;
	};
	if (BaseHS <= 0.f) BaseHS = Measure(FontS);
	if (BaseHM <= 0.f) BaseHM = Measure(FontM);
	if (BaseHL <= 0.f) BaseHL = Measure(FontL);
	ScS = BaseHS > 1.f ? 18.f * UI / BaseHS : UI;
	ScM = BaseHM > 1.f ? 23.f * UI / BaseHM : UI * 1.2f;
	ScL = BaseHL > 1.f ? 32.f * UI / BaseHL : UI * 1.6f;
	ScXL = BaseHL > 1.f ? 84.f * UI / BaseHL : UI * 4.f;
}

void ATPHUD::Rect(float X, float Y, float W, float H, const FLinearColor& Color)
{
	if (!Canvas || W <= 0.f || H <= 0.f || Color.A <= 0.f) return;
	FCanvasTileItem T(FVector2D(X, Y), FVector2D(W, H), Color);
	T.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(T);
}

void ATPHUD::Panel(float X, float Y, float W, float H, float Alpha, bool bBorder)
{
	Rect(X + 3.f * UI, Y + 3.f * UI, W, H, FLinearColor(0.f, 0.f, 0.f, Alpha * 0.45f));
	Rect(X, Y, W, H, TPUI::C(18, 14, 26, Alpha));
	if (bBorder)
	{
		const float T = FMath::Max(1.f, 2.f * UI);
		Rect(X, Y, W, T, TPUI::WithA(TPUI::Gold(), 0.9f));
		Rect(X, Y + H - T * 0.5f, W, T * 0.5f, TPUI::WithA(TPUI::Gold(), 0.5f));
	}
}

float ATPHUD::TextW(const FString& S, UFont* Font, float Scale)
{
	if (!Canvas || !Font || S.IsEmpty()) return 0.f;
	float W = 0.f, H = 0.f;
	GetTextSize(S, W, H, Font, Scale);
	return W;
}

float ATPHUD::LineH(UFont* Font, float Scale)
{
	if (Font && Font == FontS && BaseHS > 0.f) return BaseHS * Scale;
	if (Font && Font == FontM && BaseHM > 0.f) return BaseHM * Scale;
	if (Font && Font == FontL && BaseHL > 0.f) return BaseHL * Scale;
	if (!Canvas || !Font) return 16.f * Scale;
	float W = 0.f, H = 0.f;
	GetTextSize(TEXT("ЙАgy"), W, H, Font, Scale);
	return H;
}

void ATPHUD::Txt(const FString& S, float X, float Y, UFont* Font, float Scale, const FLinearColor& Color, int32 Align, bool bShadow)
{
	if (!Canvas || !Font || S.IsEmpty() || Color.A <= 0.f) return;
	float PX = X;
	if (Align != 0)
	{
		const float W = TextW(S, Font, Scale);
		PX = Align == 1 ? X - W * 0.5f : X - W;
	}
	FCanvasTextItem T(FVector2D(FMath::RoundToFloat(PX), FMath::RoundToFloat(Y)), FText::FromString(S), Font, Color);
	T.Scale = FVector2D(Scale, Scale);
	if (bShadow) T.EnableShadow(FLinearColor(0.f, 0.f, 0.f, 0.85f * Color.A), FVector2D(1.f, 1.f) * FMath::Max(1.f, FMath::RoundToFloat(UI)));
	Canvas->DrawItem(T);
}

TArray<FString> ATPHUD::Wrap(const FString& S, UFont* Font, float Scale, float MaxW)
{
	TArray<FString> Out;
	TArray<FString> Paragraphs;
	S.ParseIntoArray(Paragraphs, TEXT("\n"), false);
	for (const FString& Para : Paragraphs)
	{
		TArray<FString> Words;
		Para.ParseIntoArray(Words, TEXT(" "), true);
		FString Line;
		for (const FString& Word : Words)
		{
			const FString Try = Line.IsEmpty() ? Word : Line + TEXT(" ") + Word;
			if (!Line.IsEmpty() && TextW(Try, Font, Scale) > MaxW)
			{
				Out.Add(Line);
				Line = Word;
			}
			else Line = Try;
		}
		Out.Add(Line);
	}
	return Out;
}

float ATPHUD::TxtWrapped(const FString& S, float X, float Y, float MaxW, UFont* Font, float Scale, const FLinearColor& Color, int32 MaxLines)
{
	const TArray<FString> Lines = Wrap(S, Font, Scale, MaxW);
	const float LH = LineH(Font, Scale);
	int32 N = 0;
	for (const FString& L : Lines)
	{
		if (N >= MaxLines) break;
		FString Shown = L;
		if (N == MaxLines - 1 && Lines.Num() > MaxLines) Shown += TEXT("…");
		Txt(Shown, X, Y + N * LH, Font, Scale, Color);
		++N;
	}
	return N * LH;
}

void ATPHUD::Tri(const FVector2D& A, const FVector2D& B, const FVector2D& Cc, const FLinearColor& Color)
{
	TArray<FCanvasUVTri> T;
	T.Add(TPUI::MakeTri(A, B, Cc, Color));
	TPUI::DrawTriList(Canvas, T);
}

void ATPHUD::Disc(float CX, float CY, float RX, float RY, const FLinearColor& Color, int32 Sides)
{
	if (RX <= 0.f || RY <= 0.f || Color.A <= 0.f) return;
	Sides = FMath::Max(Sides, 3);
	TArray<FCanvasUVTri> T;
	T.Reserve(Sides);
	const FVector2D Ctr(CX, CY);
	for (int32 I = 0; I < Sides; ++I)
	{
		const float A0 = 2.f * PI * I / Sides, A1 = 2.f * PI * (I + 1) / Sides;
		T.Add(TPUI::MakeTri(Ctr, FVector2D(CX + FMath::Cos(A0) * RX, CY + FMath::Sin(A0) * RY), FVector2D(CX + FMath::Cos(A1) * RX, CY + FMath::Sin(A1) * RY), Color));
	}
	TPUI::DrawTriList(Canvas, T);
}

void ATPHUD::Ring(float CX, float CY, float R, float Thick, const FLinearColor& Color, int32 Sides)
{
	if (R <= 0.f || Thick <= 0.f) return;
	Sides = FMath::Max(Sides, 3);
	const float Ri = FMath::Max(0.f, R - Thick);
	TArray<FCanvasUVTri> T;
	T.Reserve(Sides * 2);
	for (int32 I = 0; I < Sides; ++I)
	{
		const float A0 = 2.f * PI * I / Sides, A1 = 2.f * PI * (I + 1) / Sides;
		const FVector2D O0(CX + FMath::Cos(A0) * R, CY + FMath::Sin(A0) * R), O1(CX + FMath::Cos(A1) * R, CY + FMath::Sin(A1) * R);
		const FVector2D I0(CX + FMath::Cos(A0) * Ri, CY + FMath::Sin(A0) * Ri), I1(CX + FMath::Cos(A1) * Ri, CY + FMath::Sin(A1) * Ri);
		T.Add(TPUI::MakeTri(O0, O1, I1, Color));
		T.Add(TPUI::MakeTri(O0, I1, I0, Color));
	}
	TPUI::DrawTriList(Canvas, T);
}

void ATPHUD::Diamond(float CX, float CY, float R, const FLinearColor& Color)
{
	const float O = R + FMath::Max(1.5f, 2.f * UI);
	const FLinearColor Edge(0.02f, 0.02f, 0.03f, 0.85f * Color.A);
	TArray<FCanvasUVTri> T;
	T.Add(TPUI::MakeTri(FVector2D(CX, CY - O), FVector2D(CX + O * 0.75f, CY), FVector2D(CX - O * 0.75f, CY), Edge));
	T.Add(TPUI::MakeTri(FVector2D(CX, CY + O), FVector2D(CX - O * 0.75f, CY), FVector2D(CX + O * 0.75f, CY), Edge));
	T.Add(TPUI::MakeTri(FVector2D(CX, CY - R), FVector2D(CX + R * 0.75f, CY), FVector2D(CX - R * 0.75f, CY), Color));
	T.Add(TPUI::MakeTri(FVector2D(CX, CY + R), FVector2D(CX - R * 0.75f, CY), FVector2D(CX + R * 0.75f, CY), TPUI::Mix(Color, FLinearColor::Black, 0.25f)));
	TPUI::DrawTriList(Canvas, T);
}

void ATPHUD::MedalIcon(float CX, float CY, float R, int32 Medal)
{
	static const int32 Col[4][3] = {{90, 86, 96}, {196, 130, 70}, {206, 212, 222}, {250, 206, 70}};
	const int32 M = FMath::Clamp(Medal, 0, 3);
	Disc(CX, CY, R, R, TPUI::C(20, 16, 24), 16);
	Disc(CX, CY, R * 0.8f, R * 0.8f, TPUI::C(Col[M][0], Col[M][1], Col[M][2]), 16);
	Disc(CX - R * 0.25f, CY - R * 0.25f, R * 0.26f, R * 0.26f, FLinearColor(1.f, 1.f, 1.f, M ? 0.6f : 0.12f), 8);
}

bool ATPHUD::ToScreen(const FVector& World, FVector2D& Out) const
{
	if (!Canvas || !PlayerOwner) return false;
	FVector CamLoc;
	FRotator CamRot;
	PlayerOwner->GetPlayerViewPoint(CamLoc, CamRot);
	if (FVector::DotProduct(World - CamLoc, CamRot.Vector()) < 20.f) return false;   // позади камеры
	const FVector P = Canvas->Project(World);
	Out = FVector2D(P.X, P.Y);
	return true;
}

/* ======================= кадр ======================= */

void ATPHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	// реальное время: интерфейс живёт и на паузе
	const double Now = FPlatformTime::Seconds();
	Dt = LastRealTime < 0.0 ? 0.f : (float)FMath::Clamp(Now - LastRealTime, 0.0, 0.1);
	LastRealTime = Now;
	RealTime += Dt;

	SW = Canvas->ClipX;
	SH = Canvas->ClipY;
	UI = FMath::Clamp(SH / 1080.f, 0.6f, 2.4f);
	UpdateFonts();

	ATPCharacter* H = PlayerOwner ? Cast<ATPCharacter>(PlayerOwner->GetPawn()) : nullptr;
	if (H != BoundHero.Get()) BindHero(H);
	if (!bDirectorBound) BindDirector();
	CacheTimer -= Dt;
	if (CacheTimer <= 0.f) { RefreshActorCache(); CacheTimer = 1.f; }

	UpdateInput();
	ApplyInputLock();

	ToastTime -= Dt;
	if (!IsBlockingUI()) HintTime -= Dt;   // подсказка не «сгорает», пока идёт пролог
	FadeTime -= Dt;
	DamageFlash -= Dt;
	if (DialogIndex >= 0) DialogReveal += Dt * 55.f;
	if (H && H->Combat)
	{
		const float HP = H->Combat->GetHealth();
		if (LastHealth >= 0.f && HP < LastHealth - 0.5f) DamageFlash = 0.45f;
		LastHealth = HP;
	}

	if (bAlbum)
	{
		DrawAlbum();
		return;
	}

	const UTPStorySubsystem* S = Story();
	const bool bRace = S && S->IsRaceActive();
	DrawWorldMarkers();
	if (bRace) DrawRaceHUD();
	else DrawTopPanel();
	DrawClock();
	DrawMinimap();
	DrawVitals();
	if (!IsBlockingUI()) DrawPrompts();
	DrawCountdown();
	DrawFade();
	if (DialogIndex >= 0) DrawDialog();
	if (bChoice) DrawChoice();
	DrawToast();
	if (bPaused && !bHelp) DrawPause();
	if (bHelp) DrawHelp();
}

/* ======================= верхние панели ======================= */

void ATPHUD::DrawTopPanel()
{
	const UTPStorySubsystem* S = Story();
	if (!S) return;
	const float X = 18.f * UI, Y = 16.f * UI, W = FMath::Min(520.f * UI, SW * 0.42f), Pad = 12.f * UI;
	const TArray<FString> Lines = Wrap(S->GetObjective(), FontS, ScS, W - 2.f * Pad);
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM);
	const float H = Pad + LHm + 4.f * UI + Lines.Num() * LHs + Pad * 0.8f;
	Panel(X, Y, W, H, 0.72f);
	Txt(S->GetChapterTitle(), X + Pad, Y + Pad * 0.7f, FontM, ScM, TPUI::GoldBright());
	for (int32 I = 0; I < Lines.Num(); ++I)
		Txt(Lines[I], X + Pad, Y + Pad * 0.7f + LHm + 4.f * UI + I * LHs, FontS, ScS, TPUI::Paper());
}

void ATPHUD::DrawClock()
{
	const UTPStorySubsystem* S = Story();
	FString Line;
	if (const ATPEnvironment* E = Cast<ATPEnvironment>(Environment.Get()))
	{
		const float T = FMath::Fmod(FMath::Max(0.f, E->TimeOfDay), 24.f);
		const int32 HH = FMath::FloorToInt(T), MM = FMath::Clamp(FMath::FloorToInt((T - HH) * 60.f), 0, 59);
		Line = FString::Printf(TEXT("%02d:%02d"), HH, MM);
		Line += E->IsNight() ? TEXT(" · ночь") : TEXT(" · день");
	}
	FString Line2;
	if (S)
	{
		int32 Medals = 0;
		for (int32 R = 0; R < UTPStorySubsystem::NumRaces; ++R) if (S->GetMedal(R) > 0) ++Medals;
		Line2 = FString::Printf(TEXT("Портреты %d/%d · медали %d/%d"), S->CountAlbum(2), TPCitizens::Num, Medals, UTPStorySubsystem::NumRaces);
	}
	if (Line.IsEmpty() && Line2.IsEmpty()) return;
	const float Pad = 10.f * UI;
	const float W = FMath::Max(TextW(Line, FontM, ScM), TextW(Line2, FontS, ScS)) + 2.f * Pad;
	const float LHm = LineH(FontM, ScM), LHs = LineH(FontS, ScS);
	const float H = Pad * 1.2f + (Line.IsEmpty() ? 0.f : LHm) + (Line2.IsEmpty() ? 0.f : LHs);
	const float X = SW - 18.f * UI - W, Y = 16.f * UI;
	Panel(X, Y, W, H, 0.7f);
	float TY = Y + Pad * 0.6f;
	if (!Line.IsEmpty()) { Txt(Line, X + W - Pad, TY, FontM, ScM, TPUI::Paper(), 2); TY += LHm; }
	if (!Line2.IsEmpty()) Txt(Line2, X + W - Pad, TY, FontS, ScS, TPUI::Gold(), 2);
}

void ATPHUD::DrawRaceHUD()
{
	const UTPStorySubsystem* S = Story();
	const ATPCharacter* H = Hero();
	if (!S) return;
	const float W = 460.f * UI, Pad = 10.f * UI;
	const float LHs = LineH(FontS, ScS), LHl = LineH(FontL, ScL);
	const float PH = Pad + LHs + LHl + LHs + Pad;
	const float X = SW * 0.5f - W * 0.5f, Y = 12.f * UI;
	Panel(X, Y, W, PH, 0.78f);
	Txt(S->GetRaceTitle(), SW * 0.5f, Y + Pad * 0.6f, FontS, ScS, TPUI::GoldBright(), 1);
	const bool bCount = S->GetRaceState() == ETPRaceState::Countdown;
	Txt(UTPStorySubsystem::FormatTime(bCount ? 0.f : S->GetRaceTime()), SW * 0.5f, Y + Pad * 0.6f + LHs, FontL, ScL, TPUI::Paper(), 1);
	Txt(S->GetRaceStatus(), SW * 0.5f, Y + Pad * 0.6f + LHs + LHl, FontS, ScS, TPUI::C(220, 214, 206), 1);

	// пороги медалей справа от панели
	float GoldT = 0.f, SilverT = 0.f, BronzeT = 0.f;
	if (S->GetMedalTimes(GoldT, SilverT, BronzeT))
	{
		const float MX = X + W + 14.f * UI;
		Panel(MX - 6.f * UI, Y, 130.f * UI, PH, 0.6f, false);
		const float Times[3] = {GoldT, SilverT, BronzeT};
		for (int32 I = 0; I < 3; ++I)
		{
			const float RY = Y + Pad + I * (PH - 2.f * Pad) / 3.f + LHs * 0.5f;
			MedalIcon(MX + 9.f * UI, RY, 8.f * UI, 3 - I);
			const bool bStill = S->GetRaceTime() <= Times[I];
			Txt(UTPStorySubsystem::FormatTime(Times[I]), MX + 24.f * UI, RY - LHs * 0.5f, FontS, ScS, bStill ? TPUI::C(255, 246, 220) : TPUI::C(140, 130, 130));
		}
	}

	// цель: подпись расстояния или стрелка у края экрана
	FVector Target;
	if (!H || !S->GetRaceTarget(Target)) return;
	const float DistM = FVector::Dist(H->GetActorLocation(), Target) / 100.f;
	const FString DistS = FString::Printf(TEXT("%d м"), FMath::RoundToInt(DistM));
	FVector2D P;
	const float Margin = 40.f * UI;
	if (ToScreen(Target + FVector(0.f, 0.f, 60.f), P) && P.X > Margin && P.X < SW - Margin && P.Y > Y + PH + Margin && P.Y < SH - Margin)
	{
		Diamond(P.X, P.Y, 9.f * UI, TPUI::WithA(TPUI::GoldBright(), 0.95f));
		Txt(DistS, P.X, P.Y + 14.f * UI, FontS, ScS, TPUI::C(255, 236, 170), 1);
		return;
	}
	if (!PlayerOwner) return;
	FVector CamLoc; FRotator CamRot;
	PlayerOwner->GetPlayerViewPoint(CamLoc, CamRot);
	const FRotationMatrix M(CamRot);
	const FVector D = Target - CamLoc;
	float Rt = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Y));
	float Up = FVector::DotProduct(D, M.GetUnitAxis(EAxis::Z));
	if (FVector::DotProduct(D, M.GetUnitAxis(EAxis::X)) < 0.f && FMath::Abs(Rt) < 1.f && FMath::Abs(Up) < 1.f) Up = -1.f;
	const float A = FMath::Atan2(Rt, Up);   // 0 — вверх по экрану, по часовой
	const float RX = SW * 0.5f - 70.f * UI, RY = SH * 0.5f - 80.f * UI;
	const FVector2D Dir(FMath::Sin(A), -FMath::Cos(A));
	const FVector2D Ctr(SW * 0.5f + Dir.X * RX, SH * 0.5f + Dir.Y * RY);
	const FVector2D Side(-Dir.Y, Dir.X);
	const float L = 22.f * UI;
	const float Pulse = 0.8f + 0.2f * FMath::Sin((float)RealTime * 6.f);
	Tri(Ctr + Dir * L, Ctr - Dir * L * 0.4f + Side * L * 0.75f, Ctr - Dir * L * 0.4f - Side * L * 0.75f, TPUI::WithA(TPUI::C(255, 214, 70), 0.95f * Pulse));
	Txt(DistS, Ctr.X - Dir.X * L * 1.6f, Ctr.Y - Dir.Y * L * 1.6f - LHs * 0.5f, FontS, ScS, TPUI::C(255, 236, 170), 1);
}

void ATPHUD::DrawCountdown()
{
	const UTPStorySubsystem* S = Story();
	if (!S || !S->IsRaceActive()) return;
	FString T;
	float Alpha = 1.f;
	if (S->GetRaceState() == ETPRaceState::Countdown)
	{
		const int32 N = FMath::Max(1, FMath::CeilToInt(S->GetCountdown() - 0.4f));
		T = FString::FromInt(N);
		Alpha = 0.6f + 0.4f * FMath::Frac(S->GetCountdown() - 0.4f);
	}
	else if (S->GetRaceTime() < 1.f)
	{
		T = TEXT("Марш!");
		Alpha = 1.f - S->GetRaceTime();
	}
	if (T.IsEmpty()) return;
	Txt(T, SW * 0.5f, SH * 0.26f, FontL, ScXL, TPUI::WithA(TPUI::C(255, 226, 140), Alpha), 1);
}

/* ======================= низ экрана ======================= */

void ATPHUD::DrawVitals()
{
	ATPCharacter* H = Hero();
	if (!H) return;
	const UTPStorySubsystem* S = Story();
	const float W = 340.f * UI, BH = 12.f * UI;
	const float X = SW * 0.5f - W * 0.5f, Y = SH - 40.f * UI;
	const float LHs = LineH(FontS, ScS);

	// здоровье
	if (H->Combat)
	{
		const float F = FMath::Clamp(H->Combat->GetHealthFraction(), 0.f, 1.f);
		Rect(X - 2.f * UI, Y - 2.f * UI, W + 4.f * UI, BH + 4.f * UI, FLinearColor(0.f, 0.f, 0.f, 0.65f));
		Rect(X, Y, W, BH, TPUI::C(60, 20, 24, 0.8f));
		const bool bLow = F < 0.35f;
		const float Blink = bLow ? 0.75f + 0.25f * FMath::Sin((float)RealTime * 8.f) : 1.f;
		Rect(X, Y, W * F, BH, TPUI::WithA(bLow ? TPUI::C(255, 60, 60) : TPUI::Blood(), Blink));
		Rect(X, Y, W * F, BH * 0.35f, FLinearColor(1.f, 1.f, 1.f, 0.15f));
	}

	// строка состояния над полосой: бой, погоня, толпа, паркур
	FString State;
	FLinearColor StateCol = TPUI::C(200, 200, 220);
	float MaxAlert = 0.f;
	int32 Chasing = 0;
#if TP_HUD_HAS_DIRECTOR
	if (UWorld* Wd = GetWorld())
		if (const UTPCombatDirector* Dir = Wd->GetSubsystem<UTPCombatDirector>())
		{
			MaxAlert = Dir->GetMaxAlert();
			Chasing = Dir->CountChasing();
		}
#endif
	if (H->Combat && H->Combat->IsInCombat()) { State = TEXT("БОЙ · ЛКМ/F — удар, E — контратака, Alt — уклонение"); StateCol = TPUI::C(255, 120, 100); }
	else if (Chasing > 0) { State = TEXT("ПОГОНЯ — уходите по крышам!"); StateCol = TPUI::C(255, 90, 80); }
	else if (MaxAlert > 0.3f) { State = TEXT("Стража начеку"); StateCol = TPUI::C(255, 210, 80); }
	else if (S && S->IsHeroInCrowd()) { State = TEXT("Вы в толпе"); StateCol = TPUI::C(160, 220, 180); }
	if (!State.IsEmpty()) Txt(State, SW * 0.5f, Y - LHs - 6.f * UI, FontS, ScS, StateCol, 1);

	if (H->Traversal)
	{
		const FString T = TPUI::TraversalText(H->Traversal->GetState(), H->Traversal->IsFreeRunning(), H->Traversal->IsCrouching());
		if (!T.IsEmpty()) Txt(T, SW * 0.5f, Y + BH + 3.f * UI, FontS, ScS * 0.85f, TPUI::C(214, 206, 196, 0.9f), 1);
	}

	// способности Лиги — справа внизу
	if (S)
	{
		const float R = 11.f * UI, Gap = 6.f * UI;
		const float RX = SW - 22.f * UI - UTPStorySubsystem::NumAbilities * (2.f * R + Gap);
		const float RY = SH - 34.f * UI;
		for (int32 A = 0; A < UTPStorySubsystem::NumAbilities; ++A)
		{
			const bool bHas = S->HasAbility(A);
			const float CX = RX + R + A * (2.f * R + Gap);
			Disc(CX, RY, R + 2.f * UI, R + 2.f * UI, TPUI::C(20, 16, 24, 0.85f), 20);
			Disc(CX, RY, R, R, bHas ? TPUI::C(240, 210, 120) : TPUI::C(70, 66, 76), 20);
			Txt(TPUI::AbilityShort[A], CX, RY - LineH(FontS, ScS * 0.7f) * 0.5f, FontS, ScS * 0.7f, bHas ? TPUI::Ink() : TPUI::C(150, 146, 156), 1, false);
		}
		Txt(TEXT("способности Лиги"), SW - 22.f * UI, RY - R - LHs - 2.f * UI, FontS, ScS * 0.8f, TPUI::C(210, 206, 200, 0.85f), 2);
	}
}

void ATPHUD::DrawPrompts()
{
	ATPCharacter* H = Hero();
	UTPStorySubsystem* S = Story();
	if (!H) return;
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM);
	const FVector HL = H->GetActorLocation();
	float PY = SH - 104.f * UI;

	// контратака — главное
#if TP_HUD_HAS_DIRECTOR
	if (UWorld* Wd = GetWorld())
		if (const UTPCombatDirector* Dir = Wd->GetSubsystem<UTPCombatDirector>())
			if (Dir->GetTelegraphingGuard(HL, 450.f) != nullptr)
			{
				const FString T = TEXT("E — контратака!");
				const float W = TextW(T, FontM, ScM) + 40.f * UI;
				const float Pulse = 0.75f + 0.25f * FMath::Sin((float)RealTime * 18.f);
				Rect(SW * 0.5f - W * 0.5f, PY - LHm - 8.f * UI, W, LHm + 10.f * UI, TPUI::C(150, 10, 10, 0.8f * Pulse));
				Txt(T, SW * 0.5f, PY - LHm - 3.f * UI, FontM, ScM, TPUI::C(255, 240, 220), 1);
				PY -= LHm + 16.f * UI;
			}
#endif

	// заговорить с жителем
	if (S && !S->IsRaceActive())
		if (ATPCitizen* Ct = S->FindTalkTarget(H))
		{
			const FString T = S->GetTalkPrompt(Ct);
			if (!T.IsEmpty())
			{
				const float W = TextW(T, FontS, ScS) + 30.f * UI;
				Panel(SW * 0.5f - W * 0.5f, PY - LHs - 6.f * UI, W, LHs + 8.f * UI, 0.8f);
				Txt(T, SW * 0.5f, PY - LHs - 2.f * UI, FontS, ScS, TPUI::C(255, 246, 220), 1);
				FVector2D P;
				const float Top = 90.f * Ct->GetActorScale3D().Z + 40.f;
				if (ToScreen(Ct->GetActorLocation() + FVector(0.f, 0.f, Top), P))
				{
					Disc(P.X, P.Y, 12.f * UI, 12.f * UI, TPUI::C(20, 16, 24, 0.85f), 20);
					Txt(TEXT("G"), P.X, P.Y - LineH(FontS, ScS * 0.9f) * 0.5f, FontS, ScS * 0.9f, TPUI::GoldBright(), 1, false);
				}
			}
		}

	// подсказка в начале игры
	if (HintTime > 0.f)
	{
		const float A = FMath::Clamp(HintTime / 1.5f, 0.f, 1.f);
		Txt(TEXT("F1 — управление · G — говорить · Tab — альбом · Esc/P — пауза"), SW - 22.f * UI, SH - 34.f * UI - 11.f * UI - 2.f * LHs - 8.f * UI, FontS, ScS * 0.85f, TPUI::WithA(TPUI::GoldBright(), A), 2);
	}
}

void ATPHUD::DrawToast()
{
	if (ToastTime <= 0.f || ToastText.IsEmpty()) return;
	const UTPStorySubsystem* S = Story();
	const bool bRace = S && S->IsRaceActive();
	const float A = FMath::Clamp(ToastTime / 0.4f, 0.f, 1.f);
	const float MaxW = FMath::Min(760.f * UI, SW * 0.6f);
	const TArray<FString> Lines = Wrap(ToastText, FontS, ScS, MaxW);
	const float LHs = LineH(FontS, ScS);
	float W = 0.f;
	for (const FString& L : Lines) W = FMath::Max(W, TextW(L, FontS, ScS));
	W += 32.f * UI;
	const float H = Lines.Num() * LHs + 12.f * UI;
	const float Y = bRace ? 12.f * UI + 3.f * LHs + LineH(FontL, ScL) + 34.f * UI : SH * 0.17f;
	Panel(SW * 0.5f - W * 0.5f, Y, W, H, 0.82f * A);
	for (int32 I = 0; I < Lines.Num(); ++I)
		Txt(Lines[I], SW * 0.5f, Y + 6.f * UI + I * LHs, FontS, ScS, TPUI::WithA(TPUI::C(255, 236, 200), A), 1);
}

void ATPHUD::DrawFade()
{
	if (DamageFlash > 0.f)
	{
		const float A = FMath::Clamp(DamageFlash / 0.45f, 0.f, 1.f) * 0.35f;
		const float E = 70.f * UI;
		Rect(0.f, 0.f, SW, E, TPUI::C(160, 0, 0, A));
		Rect(0.f, SH - E, SW, E, TPUI::C(160, 0, 0, A));
		Rect(0.f, E, E, SH - 2.f * E, TPUI::C(160, 0, 0, A));
		Rect(SW - E, E, E, SH - 2.f * E, TPUI::C(160, 0, 0, A));
	}
	if (FadeTime > 0.f) Rect(0.f, 0.f, SW, SH, FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(FadeTime / 1.0f, 0.f, 1.f)));
}

/* ======================= миникарта ======================= */

void ATPHUD::DrawMinimap()
{
	ATPCharacter* H = Hero();
	if (!H || !PlayerOwner) return;
	const UTPStorySubsystem* S = Story();
	const UTPIdentitySubsystem* Ids = Identities();
	const float R = 96.f * UI;
	const float CX = 24.f * UI + R, CY = SH - 24.f * UI - R;
	const float Range = 3600.f;          // см на радиус карты
	const float K = R / Range;
	const FVector HL = H->GetActorLocation();
	const float CamYaw = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw : PlayerOwner->GetControlRotation().Yaw;
	const float CY_ = FMath::Cos(FMath::DegreesToRadians(CamYaw)), SY_ = FMath::Sin(FMath::DegreesToRadians(CamYaw));

	// мир → карта: «вперёд по камере» — вверх, «вправо» — вправо
	auto ToMap = [&](const FVector& W, bool bClamp, FVector2D& Out) -> bool
	{
		const FVector D = W - HL;
		const float Fwd = D.X * CY_ + D.Y * SY_;
		const float Rt = -D.X * SY_ + D.Y * CY_;
		FVector2D P(Rt * K, -Fwd * K);
		const float L = P.Size();
		const float Lim = R - 6.f * UI;
		if (L > Lim)
		{
			if (!bClamp) return false;
			P *= Lim / L;
		}
		Out = FVector2D(CX, CY) + P;
		return true;
	};

	Disc(CX, CY, R + 4.f * UI, R + 4.f * UI, TPUI::C(214, 176, 90, 0.9f), 40);
	Disc(CX, CY, R, R, TPUI::C(24, 22, 30, 0.88f), 40);
	Ring(CX, CY, R * 0.5f, FMath::Max(1.f, UI), TPUI::C(214, 176, 90, 0.18f), 32);
	// север (+X мира) — буква «С» на краю
	{
		FVector2D N;
		if (ToMap(HL + FVector(Range * 2.f, 0.f, 0.f), true, N))
			Txt(TEXT("С"), N.X, N.Y - LineH(FontS, ScS * 0.75f) * 0.5f, FontS, ScS * 0.75f, TPUI::C(214, 176, 90, 0.8f), 1);
	}

	// жители
	if (S && Ids)
	{
		for (const TWeakObjectPtr<ATPCitizen>& Wk : S->GetCitizens())
		{
			const ATPCitizen* Ct = Wk.Get();
			if (!Ct) continue;
			const FTPIdentity& D = Ids->Get(Ct->GetIdentityIndex());
			FVector2D P;
			if (D.Role == ETPCitizenRole::Runner)
			{
				if (Ct->IsRacing()) continue;
				const bool bNews = S->RunnerHasNews(Ct);
				if (ToMap(Ct->GetActorLocation(), bNews, P)) Diamond(P.X, P.Y, (bNews ? 7.f : 5.f) * UI, bNews ? TPUI::C(255, 214, 70) : TPUI::C(170, 170, 180));
			}
			else if (D.Role == ETPCitizenRole::Painter)
			{
				if (ToMap(Ct->GetActorLocation(), S->GetChapter() == 0, P)) Disc(P.X, P.Y, 4.5f * UI, 4.5f * UI, TPUI::C(214, 120, 200), 12);
			}
			else if (!Ct->IsRacing() && ToMap(Ct->GetActorLocation(), false, P))
				Disc(P.X, P.Y, 2.2f * UI, 2.2f * UI, Ct->IsDown() ? TPUI::C(255, 140, 120, 0.9f) : TPUI::C(220, 214, 200, 0.55f), 8);
		}
	}

	// стража
#if TP_HUD_HAS_GUARD
	for (const TWeakObjectPtr<AActor>& Wk : Guards)
	{
		const ATPGuard* G = Cast<ATPGuard>(Wk.Get());
		if (!G) continue;
		FVector2D P;
		const bool bChase = G->State == ETPGuardState::Chase;
		if (!ToMap(G->GetActorLocation(), bChase, P)) continue;
		FLinearColor Col = TPUI::C(90, 130, 230);
		if (G->IsKO()) Col = TPUI::C(90, 90, 100, 0.6f);
		else if (bChase) Col = TPUI::C(255, 50, 50);
		else if (G->State == ETPGuardState::Suspicious || G->State == ETPGuardState::Search || G->Alert > 0.15f) Col = TPUI::C(255, 200, 60);
		Disc(P.X, P.Y, 3.4f * UI, 3.4f * UI, Col, 10);
	}
#endif

	// гонка: ворота, флажки, вор
	if (S && S->IsRaceActive())
	{
		TArray<FVector> Marks;
		S->GetRaceMarkers(Marks);
		FVector Tgt;
		const bool bHasTgt = S->GetRaceTarget(Tgt);
		for (const FVector& M : Marks)
		{
			FVector2D P;
			const bool bMain = bHasTgt && M.Equals(Tgt, 1.f);
			if (ToMap(M, true, P)) Diamond(P.X, P.Y, (bMain ? 7.f : 5.f) * UI, bMain ? TPUI::C(255, 214, 70) : TPUI::C(255, 140, 90, 0.8f));
		}
	}

	// герой: стрелка по направлению взгляда относительно камеры
	{
		const float A = FMath::DegreesToRadians(H->GetActorRotation().Yaw - CamYaw);
		const FVector2D F(FMath::Sin(A), -FMath::Cos(A)), Sd(-F.Y, F.X);
		const FVector2D O(CX, CY);
		const float L = 9.f * UI;
		Tri(O + F * L * 1.15f, O - F * L * 0.7f + Sd * L * 0.75f, O - F * L * 0.7f - Sd * L * 0.75f, TPUI::C(20, 16, 24));
		Tri(O + F * L, O - F * L * 0.55f + Sd * L * 0.6f, O - F * L * 0.55f - Sd * L * 0.6f, FLinearColor::White);
	}
}

/* ======================= отметки в мире ======================= */

void ATPHUD::DrawWorldMarkers()
{
	ATPCharacter* H = Hero();
	if (!H) return;
	const UTPStorySubsystem* S = Story();
	const UTPIdentitySubsystem* Ids = Identities();
	const FVector HL = H->GetActorLocation();
	const float LHs = LineH(FontS, ScS);

	if (S && Ids)
	{
		for (const TWeakObjectPtr<ATPCitizen>& Wk : S->GetCitizens())
		{
			const ATPCitizen* Ct = Wk.Get();
			if (!Ct) continue;
			const FVector L = Ct->GetActorLocation();
			const float Dist = FVector::Dist(L, HL);
			const float Top = 90.f * Ct->GetActorScale3D().Z;
			const FTPIdentity& D = Ids->Get(Ct->GetIdentityIndex());
			FVector2D P;

			// бегуны Лиги — ромб над головой, художник — сиреневая точка
			if (D.Role == ETPCitizenRole::Runner && !Ct->IsRacing() && Dist < 7000.f)
			{
				const bool bNews = S->RunnerHasNews(Ct);
				const float Bob = 6.f * FMath::Sin((float)RealTime * 3.f + D.RunnerIndex);
				if (ToScreen(L + FVector(0.f, 0.f, Top + 70.f + Bob), P))
					Diamond(P.X, P.Y, (bNews ? 10.f : 7.f) * UI, bNews ? TPUI::C(255, 214, 70) : TPUI::C(170, 170, 180, 0.9f));
			}
			else if (D.Role == ETPCitizenRole::Painter && Dist < 5000.f && ToScreen(L + FVector(0.f, 0.f, Top + 60.f), P))
			{
				Disc(P.X, P.Y, 7.f * UI, 7.f * UI, TPUI::C(30, 20, 30, 0.85f), 16);
				Disc(P.X, P.Y, 5.f * UI, 5.f * UI, TPUI::C(214, 120, 200), 16);
			}

			// реплика над головой
			const float BA = Ct->GetBarkAlpha();
			if (BA <= 0.f || Ct->GetBark().IsEmpty() || Dist > 2600.f) continue;
			if (!ToScreen(L + FVector(0.f, 0.f, Top + 38.f), P)) continue;
			if (P.X < -200.f || P.X > SW + 200.f || P.Y < 0.f || P.Y > SH) continue;
			const float Sc = ScS * FMath::Lerp(1.f, 0.8f, FMath::Clamp(Dist / 2600.f, 0.f, 1.f));
			const float LH = LineH(FontS, Sc);
			const TArray<FString> Lines = Wrap(Ct->GetBark(), FontS, Sc, 300.f * UI);
			const int32 NL = FMath::Min(Lines.Num(), 3);
			float W = 0.f;
			for (int32 I = 0; I < NL; ++I) W = FMath::Max(W, TextW(Lines[I], FontS, Sc));
			W += 16.f * UI;
			const float BH = NL * LH + 8.f * UI;
			const float BX = P.X - W * 0.5f, BY = P.Y - BH - 8.f * UI;
			const float A = 0.92f * BA;
			Rect(BX + 2.f * UI, BY + 2.f * UI, W, BH, FLinearColor(0.f, 0.f, 0.f, 0.3f * BA));
			Rect(BX, BY, W, BH, TPUI::C(250, 246, 236, A));
			Tri(FVector2D(P.X - 6.f * UI, BY + BH), FVector2D(P.X + 6.f * UI, BY + BH), FVector2D(P.X, BY + BH + 8.f * UI), TPUI::C(250, 246, 236, A));
			for (int32 I = 0; I < NL; ++I)
				Txt(Lines[I], P.X, BY + 4.f * UI + I * LH, FontS, Sc, TPUI::WithA(TPUI::Ink(), BA), 1, false);
		}
	}

	// стража: тревога, замах (контратака), прицел мушкетёра, «добить»
#if TP_HUD_HAS_GUARD
	for (const TWeakObjectPtr<AActor>& Wk : Guards)
	{
		const ATPGuard* G = Cast<ATPGuard>(Wk.Get());
		if (!G || G->IsKO()) continue;
		const float Dist = FVector::Dist(G->GetActorLocation(), HL);
		if (Dist > 5000.f) continue;
		const bool bChase = G->State == ETPGuardState::Chase;
		const bool bSusp = G->State == ETPGuardState::Suspicious || G->State == ETPGuardState::Search;
		const bool bTele = G->Combat && G->Combat->IsTelegraphing();
		const bool bOpen = G->Combat && G->Combat->IsOpenForFinisher();
		const float Aim = G->bMusketeer ? G->GetAimProgress() : 0.f;
		if (G->Alert < 0.04f && !bChase && !bSusp && !bTele && !bOpen && Aim <= 0.f) continue;
		FVector2D P;
		if (!ToScreen(G->GetActorLocation() + FVector(0.f, 0.f, 140.f), P)) continue;
		if (P.X < 0.f || P.X > SW || P.Y < 0.f || P.Y > SH) continue;

		const float A = bChase ? 1.f : FMath::Clamp(G->Alert, 0.f, 1.f);
		const FLinearColor Col = TPUI::Mix(TPUI::C(255, 220, 70), TPUI::C(255, 50, 40), A);
		Disc(P.X, P.Y, 10.f * UI, 10.f * UI, TPUI::C(20, 16, 24, 0.85f), 20);
		Disc(P.X, P.Y, (3.f + 6.f * A) * UI, (3.f + 6.f * A) * UI, Col, 20);
		if (bChase || bSusp)
			Txt(bChase ? TEXT("!") : TEXT("?"), P.X, P.Y - LineH(FontS, ScS) * 0.5f, FontS, ScS, FLinearColor::White, 1);

		if (bChase && Dist < 1400.f && G->Combat)
		{
			const float BW = 44.f * UI, BH = 5.f * UI;
			Rect(P.X - BW * 0.5f - UI, P.Y + 13.f * UI, BW + 2.f * UI, BH + 2.f * UI, FLinearColor(0.f, 0.f, 0.f, 0.75f));
			Rect(P.X - BW * 0.5f, P.Y + 14.f * UI, BW * FMath::Clamp(G->Combat->GetHealthFraction(), 0.f, 1.f), BH, bOpen ? TPUI::C(255, 230, 120) : TPUI::C(220, 60, 60));
		}
		if (bTele && Dist < 1000.f)
		{
			const float Pu = 1.f + 0.25f * FMath::Sin((float)RealTime * 30.f);
			const float RR = 15.f * UI * Pu;
			Disc(P.X, P.Y - 30.f * UI, RR, RR, TPUI::C(150, 10, 10, 0.85f), 24);
			Ring(P.X, P.Y - 30.f * UI, RR, 2.f * UI, TPUI::C(255, 230, 200), 24);
			Txt(TEXT("E"), P.X, P.Y - 30.f * UI - LineH(FontM, ScM) * 0.5f, FontM, ScM, FLinearColor::White, 1);
		}
		else if (bOpen && Dist < 1000.f)
			Txt(TEXT("ЛКМ — добить"), P.X, P.Y - 34.f * UI, FontS, ScS, TPUI::C(255, 230, 120), 1);
		if (Aim > 0.3f)
		{
			const float K = FMath::Clamp((Aim - 0.3f) / 0.7f, 0.f, 1.f);
			Ring(P.X, P.Y - 30.f * UI, (24.f - 14.f * K) * UI, 2.f * UI, TPUI::C(255, 170, 40), 28);
			Disc(P.X, P.Y - 30.f * UI, 2.5f * UI, 2.5f * UI, TPUI::C(255, 170, 40), 8);
			if (K > 0.5f) Txt(TEXT("Alt — уклон!"), P.X, P.Y - 62.f * UI, FontS, ScS * 0.9f, TPUI::C(255, 210, 120), 1);
		}
	}
#endif
	(void)LHs;
}

/* ======================= диалог и выбор ======================= */

int32 ATPHUD::FindSpeaker(const FString& Speaker) const
{
	const UTPIdentitySubsystem* Ids = Identities();
	if (!Ids || Speaker.IsEmpty()) return -1;
	if (Speaker.StartsWith(TEXT("Мэтр"))) return Ids->GetPainter();
	for (int32 I = 0; I < Ids->Num(); ++I)
	{
		const FTPIdentity& D = Ids->Get(I);
		if (D.Name == Speaker || Speaker.StartsWith(D.Name + TEXT(" ·"))) return I;
		if (D.Role == ETPCitizenRole::Runner && !D.Nick.IsEmpty() && D.Nick.Contains(Speaker)) return I;
	}
	return -1;
}

void ATPHUD::DrawDialog()
{
	if (!Dialog.IsValidIndex(DialogIndex)) return;
	FString Speaker, Text = Dialog[DialogIndex];
	int32 Bar = INDEX_NONE;
	if (Text.FindChar(TEXT('|'), Bar))
	{
		Speaker = Text.Left(Bar);
		Text = Text.Mid(Bar + 1);
	}
	const int32 Who = FindSpeaker(Speaker);
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM);
	const float X = FMath::Max(40.f * UI, SW * 0.5f - 560.f * UI), W = SW - 2.f * X, Pad = 16.f * UI;
	const float PortW = Who >= 0 ? 110.f * UI : 0.f;
	const float TextX = X + Pad + (PortW > 0.f ? PortW + Pad : 0.f);
	const float TextW_ = X + W - Pad - TextX;
	const TArray<FString> Lines = Wrap(Text, FontM, ScM, TextW_);
	const float Body = FMath::Max(Lines.Num() * LHm, PortW > 0.f ? PortW * 1.18f - LHs : 0.f);
	const float H = Pad + LHs + 6.f * UI + Body + LHs + Pad;
	const float Y = SH - H - 70.f * UI;
	Panel(X, Y, W, H, 0.9f);
	if (Who >= 0) DrawPortrait(Who, X + Pad, Y + Pad, PortW, PortW * 1.18f, 2);
	if (!Speaker.IsEmpty()) Txt(Speaker, TextX, Y + Pad * 0.7f, FontS, ScS, TPUI::GoldBright());

	// «печатная машинка»: открываем текст по буквам
	int32 Left = FMath::FloorToInt(DialogReveal);
	for (int32 I = 0; I < Lines.Num(); ++I)
	{
		if (Left <= 0) break;
		const FString Shown = Lines[I].Left(Left);
		Left -= Lines[I].Len() + 1;
		Txt(Shown, TextX, Y + Pad * 0.7f + LHs + 6.f * UI + I * LHm, FontM, ScM, TPUI::Paper());
	}
	const FString Hint = FString::Printf(TEXT("Пробел / G — дальше (%d/%d) · Esc — пропустить"), DialogIndex + 1, Dialog.Num());
	Txt(Hint, X + W - Pad, Y + H - Pad * 0.6f - LHs, FontS, ScS * 0.85f, TPUI::WithA(TPUI::Gold(), 0.6f + 0.4f * FMath::Abs(FMath::Sin((float)RealTime * 2.5f))), 2);
}

void ATPHUD::DrawChoice()
{
	Rect(0.f, 0.f, SW, SH, FLinearColor(0.f, 0.f, 0.f, 0.45f));
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM);
	const float W = FMath::Min(720.f * UI, SW - 40.f * UI), Pad = 16.f * UI;
	const float RowH = LHm + LHs + 10.f * UI;
	const float H = Pad + LHm + 12.f * UI + ChoiceItems.Num() * RowH + LHs + Pad;
	const float X = SW * 0.5f - W * 0.5f, Y = SH * 0.5f - H * 0.5f;
	Panel(X, Y, W, H, 0.94f);
	Txt(ChoiceTitle, X + Pad, Y + Pad * 0.7f, FontM, ScM, TPUI::GoldBright());
	for (int32 I = 0; I < ChoiceItems.Num(); ++I)
	{
		const float RY = Y + Pad + LHm + 12.f * UI + I * RowH;
		const bool bOn = ChoiceEnabled.IsValidIndex(I) && ChoiceEnabled[I];
		if (I == ChoiceSel)
		{
			Rect(X + 6.f * UI, RY - 3.f * UI, W - 12.f * UI, RowH - 2.f * UI, TPUI::C(214, 176, 90, 0.28f));
			const float TY = RY + LHm * 0.5f;
			Tri(FVector2D(X + 12.f * UI, TY - 6.f * UI), FVector2D(X + 12.f * UI, TY + 6.f * UI), FVector2D(X + 21.f * UI, TY), TPUI::C(255, 226, 150));
		}
		Txt(ChoiceItems[I], X + 30.f * UI, RY, FontM, ScM, bOn ? (I == ChoiceSel ? TPUI::C(255, 250, 236) : TPUI::C(236, 228, 214)) : TPUI::C(130, 124, 120));
		if (ChoiceDetails.IsValidIndex(I) && !ChoiceDetails[I].IsEmpty())
			Txt(ChoiceDetails[I], X + 30.f * UI, RY + LHm, FontS, ScS * 0.9f, bOn ? TPUI::C(200, 190, 176) : TPUI::C(120, 114, 110));
	}
	Txt(TEXT("W/S — выбор · Пробел / G — начать · Esc — назад"), SW * 0.5f, Y + H - Pad * 0.6f - LHs, FontS, ScS * 0.9f, TPUI::Gold(), 1);
}

/* ======================= справка и пауза ======================= */

void ATPHUD::DrawHelp()
{
	Rect(0.f, 0.f, SW, SH, FLinearColor(0.f, 0.f, 0.f, 0.6f));
	const float LHs = LineH(FontS, ScS), LHl = LineH(FontL, ScL);
	const int32 NK = (int32)UE_ARRAY_COUNT(TPUI::HelpKeys), NT = (int32)UE_ARRAY_COUNT(TPUI::HelpTips);
	const float W = FMath::Min(960.f * UI, SW - 40.f * UI), Pad = 22.f * UI;
	const float TipW = W - 2.f * Pad;
	float TipsH = 0.f;
	for (int32 I = 0; I < NT; ++I) TipsH += Wrap(TPUI::HelpTips[I], FontS, ScS, TipW).Num() * LHs + 4.f * UI;
	const float H = Pad + LHl + 10.f * UI + NK * (LHs + 4.f * UI) + 12.f * UI + TipsH + LHs + Pad;
	const float X = SW * 0.5f - W * 0.5f, Y = FMath::Max(10.f * UI, SH * 0.5f - H * 0.5f);
	Panel(X, Y, W, H, 0.95f);
	Txt(TEXT("Управление"), SW * 0.5f, Y + Pad * 0.6f, FontL, ScL, TPUI::GoldBright(), 1);
	float RY = Y + Pad * 0.6f + LHl + 10.f * UI;
	const float KeyW = 190.f * UI;
	for (int32 I = 0; I < NK; ++I)
	{
		Txt(TPUI::HelpKeys[I][0], X + Pad + KeyW - 14.f * UI, RY, FontS, ScS, TPUI::GoldBright(), 2);
		Txt(TPUI::HelpKeys[I][1], X + Pad + KeyW, RY, FontS, ScS, TPUI::Paper());
		RY += LHs + 4.f * UI;
	}
	RY += 6.f * UI;
	Rect(X + Pad, RY, W - 2.f * Pad, FMath::Max(1.f, UI), TPUI::WithA(TPUI::Gold(), 0.5f));
	RY += 6.f * UI;
	for (int32 I = 0; I < NT; ++I) RY += TxtWrapped(TPUI::HelpTips[I], X + Pad, RY, TipW, FontS, ScS, TPUI::Dim()) + 4.f * UI;
	Txt(TEXT("F1 / Esc — закрыть"), SW * 0.5f, Y + H - Pad * 0.5f - LHs, FontS, ScS, TPUI::Gold(), 1);
}

void ATPHUD::DrawPause()
{
	Rect(0.f, 0.f, SW, SH, FLinearColor(0.f, 0.f, 0.f, 0.55f));
	const UTPStorySubsystem* S = Story();
	TArray<FString> Items; TArray<int32> Ids;
	GetPauseItems(Items, Ids);
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM), LHl = LineH(FontL, ScL);
	const float W = 520.f * UI, Pad = 20.f * UI, RowH = LHm + 10.f * UI;
	const float H = Pad + LHl + LHs * 2.f + 16.f * UI + Items.Num() * RowH + LHs + Pad;
	const float X = SW * 0.5f - W * 0.5f, Y = SH * 0.5f - H * 0.5f;
	Panel(X, Y, W, H, 0.95f);
	Txt(TEXT("Пауза"), SW * 0.5f, Y + Pad * 0.6f, FontL, ScL, TPUI::GoldBright(), 1);
	float RY = Y + Pad * 0.6f + LHl;
	if (S)
	{
		int32 Medals = 0, Golds = 0;
		for (int32 R = 0; R < UTPStorySubsystem::NumRaces; ++R) { Medals += S->GetMedal(R) > 0; Golds += S->GetMedal(R) == 3; }
		Txt(S->GetChapterTitle(), SW * 0.5f, RY, FontS, ScS, TPUI::Paper(), 1);
		Txt(FString::Printf(TEXT("Портреты %d/%d · знакомые %d · медали %d/%d (золото %d)"), S->CountAlbum(2), TPCitizens::Num, S->CountAlbum(1), Medals, UTPStorySubsystem::NumRaces, Golds),
			SW * 0.5f, RY + LHs, FontS, ScS * 0.9f, TPUI::Dim(), 1);
	}
	RY += LHs * 2.f + 16.f * UI;
	for (int32 I = 0; I < Items.Num(); ++I)
	{
		const float IY = RY + I * RowH;
		if (I == PauseSel)
		{
			Rect(X + 40.f * UI, IY - 3.f * UI, W - 80.f * UI, RowH - 2.f * UI, TPUI::C(214, 176, 90, 0.3f));
			const float TY = IY + LHm * 0.5f;
			Tri(FVector2D(X + 50.f * UI, TY - 7.f * UI), FVector2D(X + 50.f * UI, TY + 7.f * UI), FVector2D(X + 60.f * UI, TY), TPUI::C(255, 226, 150));
		}
		Txt(Items[I], SW * 0.5f, IY, FontM, ScM, I == PauseSel ? TPUI::C(255, 250, 236) : TPUI::C(214, 206, 196), 1);
	}
	Txt(TEXT("W/S — выбор · Пробел / Enter — выбрать · Esc — продолжить"), SW * 0.5f, Y + H - Pad * 0.6f - LHs, FontS, ScS * 0.9f, TPUI::Gold(), 1);
}

/* ======================= альбом «Лица квартала» ======================= */

void ATPHUD::GildedFrame(float X, float Y, float W, float H)
{
	const float T = 8.f * UI;
	const FLinearColor G1 = TPUI::C(196, 150, 60), G2 = TPUI::C(250, 216, 120), G3 = TPUI::C(120, 84, 30);
	Rect(X - T, Y - T, W + 2.f * T, T, G1);
	Rect(X - T, Y + H, W + 2.f * T, T, G1);
	Rect(X - T, Y, T, H, G1);
	Rect(X + W, Y, T, H, G1);
	Rect(X - T, Y - T, W + 2.f * T, 2.f * UI, G2);
	Rect(X - T, Y - T, 2.f * UI, H + 2.f * T, G2);
	Rect(X - 2.f * UI, Y - 2.f * UI, W + 4.f * UI, 2.f * UI, G3);
	Rect(X - 2.f * UI, Y - 2.f * UI, 2.f * UI, H + 4.f * UI, G3);
	Rect(X + W, Y - 2.f * UI, 2.f * UI, H + 4.f * UI, TPUI::WithA(G2, 0.8f));
	Rect(X - 2.f * UI, Y + H, W + 4.f * UI, 2.f * UI, TPUI::WithA(G2, 0.8f));
	for (int32 K = 0; K < 4; ++K)
	{
		const float CX = (K & 1) ? X + W + T * 0.5f : X - T * 0.5f, CY = (K & 2) ? Y + H + T * 0.5f : Y - T * 0.5f;
		Disc(CX, CY, 6.f * UI, 6.f * UI, G2, 12);
		Disc(CX, CY, 3.f * UI, 3.f * UI, G3, 10);
	}
}

void ATPHUD::DrawPortrait(int32 Index, float X, float Y, float W, float H, int32 ForceStatus)
{
	const UTPIdentitySubsystem* Ids = Identities();
	const UTPStorySubsystem* S = Story();
	if (!Ids) return;
	const FTPIdentity& D = Ids->Get(Index);
	const int32 St = ForceStatus >= 0 ? ForceStatus : (S ? S->GetAlbumStatus(Index) : 0);

	// тёплый фон полосами и свечение за головой
	const int32 Bands = 14;
	for (int32 I = 0; I < Bands; ++I)
		Rect(X, Y + H * I / Bands, W, H / Bands + 1.f, TPUI::Mix(TPUI::C(78, 66, 48), TPUI::C(28, 24, 20), (float)I / (Bands - 1)));
	Disc(X + W * 0.5f, Y + H * 0.38f, W * 0.44f, W * 0.44f, TPUI::C(140, 110, 66, 0.28f), 28);

	if (St == 0)
	{
		Rect(X, Y, W, H, TPUI::C(20, 16, 14, 0.5f));
		Txt(TEXT("?"), X + W * 0.5f, Y + H * 0.5f - LineH(FontL, ScL * 1.6f) * 0.5f, FontL, ScL * 1.6f, TPUI::C(180, 160, 120), 1);
		GildedFrame(X, Y, W, H);
		return;
	}

	FLinearColor Skin(D.Skin), Hair(D.Hair), Coat(D.Coat), Trim(D.Trim), Pants(D.Pants);
	if (St == 1)
	{
		// знакомы, но портрет не написан — лишь силуэт
		const FLinearColor Sil = TPUI::C(46, 40, 38);
		Skin = Hair = Coat = Trim = Pants = Sil;
	}
	const float CX = X + W * 0.5f;
	const float HeadR = W * 0.165f;
	const float HeadY = Y + H * 0.40f;
	const FLinearColor Dark = St == 2 ? TPUI::C(30, 26, 28) : TPUI::C(46, 40, 38);

	// длинные волосы / апостольник за головой
	if (D.bFemale)
	{
		if (D.Class == ETPSocialClass::Clergy) Disc(CX, HeadY + HeadR * 0.35f, HeadR * 1.3f, HeadR * 1.45f, St == 2 ? TPUI::C(250, 248, 248) : Hair, 28);
		else Disc(CX, HeadY + HeadR * 0.45f, HeadR * 1.12f, HeadR * 1.4f, Hair, 28);
	}
	// плечи: эллипс сверху и прямоугольник до низа рамки
	const float ShY = Y + H * 0.80f;
	Disc(CX, ShY, W * 0.40f, H * 0.11f, Coat, 32);
	Rect(CX - W * 0.40f, ShY, W * 0.80f, Y + H - ShY, Coat);
	// шея и ворот
	Rect(CX - HeadR * 0.38f, HeadY + HeadR * 0.7f, HeadR * 0.76f, ShY - (HeadY + HeadR * 0.7f) + 2.f, Skin);
	Tri(FVector2D(CX - HeadR * 0.9f, ShY - H * 0.07f), FVector2D(CX + HeadR * 0.9f, ShY - H * 0.07f), FVector2D(CX, ShY + H * 0.08f), Trim);
	if (D.Class == ETPSocialClass::Soldier || D.Role == ETPCitizenRole::Runner)
		Rect(CX - W * 0.40f, ShY + H * 0.07f, W * 0.80f, H * 0.035f, D.Class == ETPSocialClass::Soldier ? Trim : TPUI::C(200, 40, 48));
	// волосы сверху и лицо
	Disc(CX, HeadY - HeadR * 0.32f, HeadR * 0.98f, HeadR * 0.78f, Hair, 28);
	Disc(CX, HeadY + HeadR * 0.12f, HeadR * 0.80f, HeadR * 0.92f, Skin, 28);
	if (St == 2)
	{
		// глаза, брови, рот
		const FLinearColor Eye = TPUI::C(40, 30, 28);
		Disc(CX - HeadR * 0.32f, HeadY + HeadR * 0.05f, HeadR * 0.09f, HeadR * 0.11f, Eye, 10);
		Disc(CX + HeadR * 0.32f, HeadY + HeadR * 0.05f, HeadR * 0.09f, HeadR * 0.11f, Eye, 10);
		Rect(CX - HeadR * 0.45f, HeadY - HeadR * 0.17f, HeadR * 0.26f, HeadR * 0.06f, TPUI::Mix(Hair, Eye, 0.4f));
		Rect(CX + HeadR * 0.19f, HeadY - HeadR * 0.17f, HeadR * 0.26f, HeadR * 0.06f, TPUI::Mix(Hair, Eye, 0.4f));
		Rect(CX - HeadR * 0.2f, HeadY + HeadR * 0.55f, HeadR * 0.4f, HeadR * 0.07f, TPUI::C(150, 70, 66));
		Disc(CX - HeadR * 0.5f, HeadY + HeadR * 0.38f, HeadR * 0.14f, HeadR * 0.09f, TPUI::C(220, 120, 110, 0.35f), 10);
		Disc(CX + HeadR * 0.5f, HeadY + HeadR * 0.38f, HeadR * 0.14f, HeadR * 0.09f, TPUI::C(220, 120, 110, 0.35f), 10);
	}
	// головной убор по роли и сословию
	const float TopY = HeadY - HeadR * 0.95f;
	if (D.Role == ETPCitizenRole::Painter)
		Disc(CX - HeadR * 0.2f, TopY + HeadR * 0.05f, HeadR * 1.05f, HeadR * 0.33f, St == 2 ? TPUI::C(70, 30, 60) : Dark, 24);   // берет
	else if (D.Role == ETPCitizenRole::Runner)
		Rect(CX - HeadR * 0.82f, HeadY - HeadR * 0.5f, HeadR * 1.64f, HeadR * 0.16f, St == 2 ? TPUI::C(200, 40, 48) : Dark);   // повязка Лиги
	else if (!D.bFemale && (D.Class == ETPSocialClass::Noble || D.Class == ETPSocialClass::Soldier || D.Class == ETPSocialClass::Shop || D.Class == ETPSocialClass::Artisan))
	{
		// треуголка (у солдат — с красной кокардой)
		Tri(FVector2D(CX - HeadR * 1.45f, TopY + HeadR * 0.22f), FVector2D(CX + HeadR * 1.45f, TopY + HeadR * 0.22f), FVector2D(CX, TopY - HeadR * 0.55f), Dark);
		Rect(CX - HeadR * 1.1f, TopY + HeadR * 0.1f, HeadR * 2.2f, HeadR * 0.2f, TPUI::Mix(Dark, Trim, D.Class == ETPSocialClass::Noble ? 0.5f : 0.f));
		if (D.Class == ETPSocialClass::Soldier && St == 2) Disc(CX + HeadR * 0.55f, TopY + HeadR * 0.05f, HeadR * 0.16f, HeadR * 0.16f, TPUI::C(190, 40, 50), 10);
	}
	else if (!D.bFemale && D.Class == ETPSocialClass::Clergy)
		Disc(CX, TopY + HeadR * 0.22f, HeadR * 0.6f, HeadR * 0.22f, Dark, 16);   // скуфья
	else if (D.Class == ETPSocialClass::Labor || D.Class == ETPSocialClass::Sailor || D.Class == ETPSocialClass::Servant)
		Disc(CX, TopY + HeadR * 0.2f, HeadR * 0.98f, HeadR * 0.38f, St == 2 ? (D.Class == ETPSocialClass::Sailor ? TPUI::C(170, 40, 40) : (D.bFemale ? Trim : Pants)) : Dark, 24);   // колпак, чепец
	else if (D.bFemale && D.Class == ETPSocialClass::Noble)
		Disc(CX, TopY - HeadR * 0.05f, HeadR * 0.75f, HeadR * 0.45f, St == 2 ? TPUI::C(236, 232, 226) : Dark, 24);   // напудренная причёска

	if (St == 2)
	{
		// мазки кистью — портрет «написан маслом»
		for (int32 K = 0; K < 10; ++K)
		{
			const uint32 Hh = UTPIdentitySubsystem::Hash(Index * 31 + K, 4021);
			const float SX = X + (Hh % 1000u) / 1000.f * W, SY = Y + ((Hh >> 10) % 1000u) / 1000.f * H;
			Rect(SX, SY, W * 0.12f, FMath::Max(1.f, 1.5f * UI), FLinearColor(1.f, 0.95f, 0.85f, 0.06f));
		}
	}
	else
	{
		Rect(X, Y + H * 0.82f, W, H * 0.18f, TPUI::C(20, 16, 14, 0.55f));
		Txt(TEXT("не написан"), X + W * 0.5f, Y + H * 0.86f, FontS, ScS * 0.85f, TPUI::C(200, 180, 140), 1);
	}
	GildedFrame(X, Y, W, H);
}

void ATPHUD::DrawAlbum()
{
	const UTPIdentitySubsystem* Ids = Identities();
	const UTPStorySubsystem* S = Story();
	// фон — тёмный градиент
	const int32 Bands = 16;
	for (int32 I = 0; I < Bands; ++I)
		Rect(0.f, SH * I / Bands, SW, SH / Bands + 1.f, TPUI::Mix(TPUI::C(44, 33, 30), TPUI::C(14, 11, 12), (float)I / (Bands - 1)));
	if (!Ids || !S)
	{
		Txt(TEXT("Альбом недоступен"), SW * 0.5f, SH * 0.5f, FontL, ScL, TPUI::Paper(), 1);
		return;
	}
	const int32 N = FMath::Min(Ids->Num(), TPCitizens::Num);
	AlbumSel = FMath::Clamp(AlbumSel, 0, FMath::Max(0, N - 1));
	const float M = 28.f * UI;
	const float LHs = LineH(FontS, ScS), LHm = LineH(FontM, ScM), LHl = LineH(FontL, ScL);

	Txt(TEXT("Лица квартала"), M, M * 0.6f, FontL, ScL, TPUI::GoldBright());
	Txt(FString::Printf(TEXT("Портреты: %d из %d · знакомые: %d"), S->CountAlbum(2), TPCitizens::Num, S->CountAlbum(1)), M, M * 0.6f + LHl, FontS, ScS, TPUI::C(220, 210, 196));

	// сетка 16 × 8
	const int32 Cols = 16, Rows = (N + Cols - 1) / Cols;
	const float GY = M * 0.6f + LHl + LHs + 14.f * UI;
	const float LegendH = 5.f * (LHs + 2.f * UI) + 16.f * UI;
	const float Cell = FMath::Max(8.f, FMath::Min((SW * 0.54f - M) / Cols, (SH - GY - LegendH - LHs - 2.f * M) / FMath::Max(1, Rows)));
	const float Box = Cell * 0.88f;
	for (int32 I = 0; I < N; ++I)
	{
		const float CX = M + (I % Cols) * Cell, CY = GY + (I / Cols) * Cell;
		const FTPIdentity& D = Ids->Get(I);
		const int32 St = S->GetAlbumStatus(I);
		const FLinearColor ClassCol = UTPIdentitySubsystem::ClassColor(D.Class);
		if (I == AlbumSel) Rect(CX - 3.f * UI, CY - 3.f * UI, Box + 6.f * UI, Box + 6.f * UI, TPUI::C(255, 226, 140));
		if (St == 2)
		{
			Rect(CX, CY, Box, Box, TPUI::C(214, 176, 90));
			Rect(CX + 1.5f * UI, CY + 1.5f * UI, Box - 3.f * UI, Box - 3.f * UI, TPUI::Mix(ClassCol, TPUI::C(40, 30, 30), 0.25f));
			Rect(CX + Box * 0.22f, CY + Box * 0.68f, Box * 0.56f, Box * 0.26f, FLinearColor(D.Coat));
			Disc(CX + Box * 0.5f, CY + Box * 0.46f, Box * 0.22f, Box * 0.24f, FLinearColor(D.Skin), 14);
			Disc(CX + Box * 0.5f, CY + Box * 0.28f, Box * 0.24f, Box * 0.13f, FLinearColor(D.Hair), 14);
		}
		else if (St == 1)
		{
			Rect(CX, CY, Box, Box, TPUI::Mix(ClassCol, TPUI::C(30, 26, 26), 0.55f));
			Disc(CX + Box * 0.5f, CY + Box * 0.45f, Box * 0.22f, Box * 0.24f, TPUI::C(60, 52, 50), 14);
			Rect(CX + Box * 0.22f, CY + Box * 0.68f, Box * 0.56f, Box * 0.26f, TPUI::C(60, 52, 50));
		}
		else
		{
			Rect(CX, CY, Box, Box, TPUI::C(36, 30, 30));
			Txt(TEXT("?"), CX + Box * 0.5f, CY + Box * 0.5f - LineH(FontS, ScS * 0.8f) * 0.5f, FontS, ScS * 0.8f, TPUI::C(80, 72, 70), 1, false);
		}
		// особые персонажи — маленький ромб в углу
		if (St > 0 && D.Role == ETPCitizenRole::Runner) Diamond(CX + Box - 4.f * UI, CY + 4.f * UI, 3.5f * UI, TPUI::C(255, 214, 70));
	}

	// легенда по сословиям
	{
		const float LY = GY + Rows * Cell + 12.f * UI;
		const float ColW = (Cols * Cell) * 0.5f;
		for (int32 Cl = 0; Cl < 10; ++Cl)
		{
			const float LX = M + (Cl % 2) * ColW, RowY = LY + (Cl / 2) * (LHs + 2.f * UI);
			const ETPSocialClass Klass = (ETPSocialClass)Cl;
			int32 Tot = 0, Got = 0;
			for (int32 I = 0; I < N; ++I) if (Ids->Get(I).Class == Klass) { ++Tot; Got += S->GetAlbumStatus(I) == 2; }
			Rect(LX, RowY + LHs * 0.2f, LHs * 0.6f, LHs * 0.6f, UTPIdentitySubsystem::ClassColor(Klass));
			Txt(FString::Printf(TEXT("%s %d/%d"), *UTPIdentitySubsystem::ClassName(Klass), Got, Tot), LX + LHs * 0.9f, RowY, FontS, ScS * 0.9f, TPUI::C(220, 210, 196));
		}
	}
	Txt(TEXT("WASD / стрелки — выбор · Q / E — предыдущий / следующий знакомый · Tab / Esc — закрыть"), M, SH - M * 0.6f - LHs, FontS, ScS * 0.9f, TPUI::C(200, 180, 140));

	// карточка выбранного жителя
	const int32 Id = AlbumSel;
	const FTPIdentity& D = Ids->Get(Id);
	const int32 St = S->GetAlbumStatus(Id);
	const float CardX = M + Cols * Cell + 44.f * UI;
	const float CardW = SW - CardX - M;
	if (CardW < 120.f * UI) return;
	const float PW = FMath::Min(CardW * 0.5f, SH * 0.36f), PH = PW * 1.18f;
	const float PX = CardX + 10.f * UI, PY = GY + 8.f * UI;
	DrawPortrait(Id, PX, PY, PW, PH);
	// рядом с портретом — номер и сословие
	const float SideX = PX + PW + 26.f * UI;
	if (SideX < CardX + CardW - 60.f * UI)
	{
		Txt(FString::Printf(TEXT("№ %d"), Id + 1), SideX, PY, FontM, ScM, TPUI::Dim());
		Txt(UTPIdentitySubsystem::ClassName(D.Class), SideX, PY + LHm, FontS, ScS, UTPIdentitySubsystem::ClassColor(D.Class));
		const TCHAR* StName = St == 2 ? TEXT("портрет написан") : St == 1 ? TEXT("знакомы") : TEXT("незнакомец");
		Txt(StName, SideX, PY + LHm + LHs, FontS, ScS, St == 2 ? TPUI::GoldBright() : TPUI::Dim());
		if (D.Role == ETPCitizenRole::Runner && St > 0)
			Txt(FString::Printf(TEXT("Лига крыш: «%s»"), *D.Nick), SideX, PY + LHm + 2.f * LHs, FontS, ScS, TPUI::C(255, 214, 70));
	}
	float TY = PY + PH + 22.f * UI;
	const float TW = CardW - 10.f * UI;
	const float Bottom = SH - M - LHs * 1.5f;
	if (St >= 1)
	{
		Txt(D.Name, CardX, TY, FontL, ScL * 0.9f, TPUI::C(250, 236, 200));
		TY += LineH(FontL, ScL * 0.9f) + 2.f * UI;
		TY += TxtWrapped(FString::Printf(TEXT("%d лет · %s · %s"), D.Age, D.bFemale ? TEXT("женщина") : TEXT("мужчина"), *D.Job), CardX, TY, TW, FontS, ScS, TPUI::C(220, 206, 180), 2) + 4.f * UI;
		if (St == 2)
		{
			Txt(FString::Printf(TEXT("Черты: %s, %s"), *D.Trait1, *D.Trait2), CardX, TY, FontS, ScS, TPUI::C(236, 196, 96));
			TY += LHs + 6.f * UI;
			const int32 MaxLines = FMath::Max(1, FMath::FloorToInt((Bottom - TY) / LHs));
			TxtWrapped(D.Bio, CardX, TY, TW, FontS, ScS, TPUI::C(236, 230, 220), MaxLines);
		}
		else
			TxtWrapped(TEXT("Знакомы, но портрет ещё не написан. Побеждайте в гонках Лиги: за каждую новую медаль Мэтр Огюстен пишет новые портреты."), CardX, TY, TW, FontS, ScS, TPUI::Dim(), 4);
	}
	else
	{
		Txt(TEXT("Незнакомец"), CardX, TY, FontL, ScL * 0.9f, TPUI::C(170, 160, 150));
		TY += LineH(FontL, ScL * 0.9f) + 4.f * UI;
		TxtWrapped(FString::Printf(TEXT("%s. Поговорите с жителями квартала (G), чтобы узнать имена; портреты — награды за гонки Лиги."), *UTPIdentitySubsystem::ClassName(D.Class)),
			CardX, TY, TW, FontS, ScS, TPUI::Dim(), 4);
	}
}
