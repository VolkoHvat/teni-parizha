#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "InputCoreTypes.h"
#include "Engine/EngineTypes.h"
struct FInputActionKeyMapping
{
	FName ActionName; uint8 bShift : 1; uint8 bCtrl : 1; uint8 bAlt : 1; uint8 bCmd : 1; FKey Key;
	FInputActionKeyMapping(const FName InActionName = NAME_None, const FKey InKey = EKeys::Invalid, const bool bInShift = false, const bool bInCtrl = false, const bool bInAlt = false, const bool bInCmd = false);
	bool operator==(const FInputActionKeyMapping& Other) const;
};
struct FInputAxisKeyMapping
{
	FName AxisName; float Scale; FKey Key;
	FInputAxisKeyMapping(const FName InAxisName = NAME_None, const FKey InKey = EKeys::Invalid, const float InScale = 1.f);
	bool operator==(const FInputAxisKeyMapping& Other) const;
};
struct FInputAxisConfigEntry; struct FInputAxisProperties { float DeadZone; float Sensitivity; float Exponent; uint8 bInvert : 1; };
class UPlayerInput : public UObject
{
public:
	typedef UObject Super; typedef UPlayerInput ThisClass; static UClass* StaticClass();
	UPlayerInput(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	bool IsPressed(FKey InKey) const; bool WasJustPressed(FKey InKey) const; bool WasJustReleased(FKey InKey) const;
	float GetTimeDown(FKey InKey) const; float GetKeyValue(FKey InKey) const; float GetRawKeyValue(FKey InKey) const; FVector GetProcessedVectorKeyValue(FKey InKey) const; FVector GetRawVectorKeyValue(FKey InKey) const;
	bool IsAltPressed() const; bool IsCtrlPressed() const; bool IsShiftPressed() const; bool IsCmdPressed() const;
	void AddActionMapping(const FInputActionKeyMapping& KeyMapping); void RemoveActionMapping(const FInputActionKeyMapping& KeyMapping);
	void AddAxisMapping(const FInputAxisKeyMapping& KeyMapping); void RemoveAxisMapping(const FInputAxisKeyMapping& KeyMapping);
	static void AddEngineDefinedActionMapping(const FInputActionKeyMapping& ActionMapping);
	static void AddEngineDefinedAxisMapping(const FInputAxisKeyMapping& AxisMapping);
	void ForceRebuildingKeyMaps(const bool bRestoreDefaults = false);
	const TArray<FInputActionKeyMapping>& GetKeysForAction(const FName ActionName) const;
	const TArray<FInputAxisKeyMapping>& GetKeysForAxis(const FName AxisName) const;
	virtual void FlushPressedKeys();
	bool InputKey(FKey Key, enum EInputEvent Event, float AmountDepressed, bool bGamepad);
	float GetMouseSensitivityX(); float GetMouseSensitivityY(); bool GetInvertAxisKey(const FKey AxisKey); bool GetInvertAxis(const FName AxisName);
	void SetMouseSensitivity(const float Sensitivity);
	TArray<FInputActionKeyMapping> ActionMappings; TArray<FInputAxisKeyMapping> AxisMappings;
};
