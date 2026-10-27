#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
class APlayerController; class UGameViewportClient; class UWorld; class UGameInstance;
class UPlayer : public UObject { public: typedef UObject Super; typedef UPlayer ThisClass; static UClass* StaticClass(); UPlayer(const FObjectInitializer& OI = FObjectInitializer::Get()); APlayerController* PlayerController; APlayerController* GetPlayerController(const UWorld* const InWorld) const; };
class ULocalPlayer : public UPlayer
{
public:
	typedef UPlayer Super; typedef ULocalPlayer ThisClass; static UClass* StaticClass();
	ULocalPlayer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UGameViewportClient* ViewportClient; FVector2D Origin; FVector2D Size; FVector LastViewLocation;
	template<class TSubsystemClass> TSubsystemClass* GetSubsystem() const { static_assert(sizeof(TSubsystemClass) > 0, "GetSubsystem<T>: неполный тип"); return nullptr; }
	int32 GetControllerId() const; void SetControllerId(int32 NewControllerId);
	virtual FString GetNickname() const;
	UGameInstance* GetGameInstance() const;
	virtual UWorld* GetWorld() const override;
};
