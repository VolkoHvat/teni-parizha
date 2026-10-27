#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "GameFramework/PlayerInput.h"
class UInputSettings : public UObject
{
public:
	typedef UObject Super; typedef UInputSettings ThisClass; static UClass* StaticClass();
	UInputSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	static UInputSettings* GetInputSettings();
	void AddActionMapping(const FInputActionKeyMapping& KeyMapping, bool bForceRebuildKeymaps = true);
	void RemoveActionMapping(const FInputActionKeyMapping& KeyMapping, bool bForceRebuildKeymaps = true);
	void AddAxisMapping(const FInputAxisKeyMapping& KeyMapping, bool bForceRebuildKeymaps = true);
	void RemoveAxisMapping(const FInputAxisKeyMapping& KeyMapping, bool bForceRebuildKeymaps = true);
	void GetActionMappingByName(const FName InActionName, TArray<FInputActionKeyMapping>& OutMappings) const;
	void GetAxisMappingByName(const FName InAxisName, TArray<FInputAxisKeyMapping>& OutMappings) const;
	const TArray<FInputActionKeyMapping>& GetActionMappings() const; const TArray<FInputAxisKeyMapping>& GetAxisMappings() const;
	void GetActionNames(TArray<FName>& ActionNames) const; void GetAxisNames(TArray<FName>& AxisNames) const;
	bool DoesActionExist(const FName InActionName); bool DoesAxisExist(const FName InAxisName);
	void ForceRebuildKeymaps(); void SaveKeyMappings();
	uint8 bAltEnterTogglesFullscreen : 1; uint8 bF11TogglesFullscreen : 1; uint8 bUseMouseForTouch : 1; uint8 bEnableMouseSmoothing : 1; uint8 bEnableFOVScaling : 1;
	float DoubleClickTime; float FOVScale;
};
