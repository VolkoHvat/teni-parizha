// Тени Парижа — интерфейс: подсказки, здоровье, миникарта, диалоги, альбом портретов, меню паузы.
// Владелец модуля: агент «жители, сюжет и интерфейс». Рисуется через Canvas — без ассетов виджетов.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPHUD.generated.h"

UCLASS()
class TENIPARIZHA_API ATPHUD : public AHUD
{
	GENERATED_BODY()
public:
	ATPHUD();
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	/** всплывающее сообщение внизу экрана */
	UFUNCTION(BlueprintCallable, Category = "Интерфейс") void Toast(const FString& Text, float Seconds = 3.5f);
	/** реплики диалога «Имя|Текст»; вызывается сюжетом */
	UFUNCTION(BlueprintCallable, Category = "Интерфейс") void ShowDialog(const TArray<FString>& Lines);
	UFUNCTION(BlueprintPure, Category = "Интерфейс") bool IsDialogOpen() const { return DialogIndex >= 0; }

protected:
	FString ToastText; float ToastTime = 0.f;
	TArray<FString> Dialog; int32 DialogIndex = -1;
	bool bHelp = true, bAlbum = false, bPaused = false;
};
