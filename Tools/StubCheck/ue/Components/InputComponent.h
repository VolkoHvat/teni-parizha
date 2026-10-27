// Макет Components/InputComponent.h (UE 4.27): классический ввод — BindAxis/BindAction по именам из DefaultInput.ini.
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
DECLARE_DELEGATE(FInputActionHandlerSignature);
DECLARE_DELEGATE_OneParam(FInputActionHandlerWithKeySignature, FKey);
DECLARE_DELEGATE_OneParam(FInputAxisHandlerSignature, float);
DECLARE_DELEGATE_OneParam(FInputVectorAxisHandlerSignature, FVector);
DECLARE_DELEGATE_TwoParams(FInputTouchHandlerSignature, ETouchIndex::Type, FVector);
struct FInputBinding { uint8 bConsumeInput : 1; uint8 bExecuteWhenPaused : 1; };
struct FInputActionUnifiedDelegate { bool IsBound() const; void Unbind(); };
struct FInputActionBinding : public FInputBinding { TEnumAsByte<EInputEvent> KeyEvent; FInputActionUnifiedDelegate ActionDelegate; FInputActionBinding(); FInputActionBinding(const FName InActionName, const enum EInputEvent InKeyEvent); FName GetActionName() const; bool IsPaired() const; int32 GetHandle() const; };
struct FInputKeyBinding : public FInputBinding { FInputKeyBinding(); };
struct FInputAxisBinding : public FInputBinding { FName AxisName; FInputAxisHandlerSignature AxisDelegate; float AxisValue; FInputAxisBinding(); FInputAxisBinding(const FName InAxisName); };
struct FInputAxisKeyBinding : public FInputBinding { float AxisValue; FKey AxisKey; FInputAxisHandlerSignature AxisDelegate; };
struct FInputVectorAxisBinding : public FInputBinding { FVector AxisValue; FKey AxisKey; FInputVectorAxisHandlerSignature AxisDelegate; };
struct FInputTouchBinding : public FInputBinding {};
struct FInputChord { FKey Key; uint32 bShift : 1; uint32 bCtrl : 1; uint32 bAlt : 1; uint32 bCmd : 1; FInputChord(); FInputChord(const FKey InKey, const bool bInShift = false, const bool bInCtrl = false, const bool bInAlt = false, const bool bInCmd = false); };
class UInputComponent : public UActorComponent
{
public:
	typedef UActorComponent Super; typedef UInputComponent ThisClass; static UClass* StaticClass();
	UInputComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	TArray<FInputKeyBinding> KeyBindings; TArray<FInputTouchBinding> TouchBindings; TArray<FInputAxisBinding> AxisBindings;
	TArray<FInputAxisKeyBinding> AxisKeyBindings; TArray<FInputVectorAxisBinding> VectorAxisBindings;
	int32 Priority; uint8 bBlockInput : 1;
	template<class UserClass> FInputActionBinding& BindAction(const FName ActionName, const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)()>::Type Func);
	template<class UserClass> FInputActionBinding& BindAction(const FName ActionName, const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)(FKey)>::Type Func);
	template<class DelegateType, class UserClass, typename... VarTypes> FInputActionBinding& BindAction(const FName ActionName, const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)(VarTypes...)>::Type Func, VarTypes... Vars);
	FInputActionBinding& AddActionBinding(FInputActionBinding Binding);
	void ClearActionBindings(); void RemoveActionBinding(const int32 BindingIndex); void RemoveActionBinding(FName ActionName, EInputEvent KeyEvent); void RemoveActionBindingForHandle(const int32 Handle);
	FInputActionBinding& GetActionBinding(const int32 BindingIndex) const; int32 GetNumActionBindings() const;
	template<class UserClass> FInputAxisBinding& BindAxis(const FName AxisName, UserClass* Object, typename TTPIdentity<void (UserClass::*)(float)>::Type Func);
	FInputAxisBinding& BindAxis(const FName AxisName);
	template<class UserClass> FInputAxisKeyBinding& BindAxisKey(const FKey AxisKey, UserClass* Object, typename TTPIdentity<void (UserClass::*)(float)>::Type Func);
	template<class UserClass> FInputVectorAxisBinding& BindVectorAxis(const FKey AxisKey, UserClass* Object, typename TTPIdentity<void (UserClass::*)(FVector)>::Type Func);
	template<class UserClass> FInputKeyBinding& BindKey(const FKey Key, const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)()>::Type Func);
	template<class UserClass> FInputKeyBinding& BindKey(const FInputChord Chord, const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)()>::Type Func);
	template<class UserClass> FInputTouchBinding& BindTouch(const EInputEvent KeyEvent, UserClass* Object, typename TTPIdentity<void (UserClass::*)(ETouchIndex::Type, FVector)>::Type Func);
	float GetAxisValue(const FName AxisName) const; float GetAxisKeyValue(const FKey AxisKey) const; FVector GetVectorAxisValue(const FKey AxisKey) const;
	bool HasBindings() const; void ClearBindingValues();
};
