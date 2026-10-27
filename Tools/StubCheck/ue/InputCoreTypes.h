// Макет InputCoreTypes.h (UE 4.27, модуль InputCore)
#pragma once
// TP_STUB_MODULE: InputCore
#include "CoreMinimal.h"
struct FKey
{
	FKey(); FKey(const FName InName); FKey(const TCHAR* InName); FKey(const ANSICHAR* InName);
	bool IsValid() const; bool IsModifierKey() const; bool IsGamepadKey() const; bool IsTouch() const; bool IsMouseButton() const;
	bool IsFloatAxis() const; bool IsVectorAxis() const; bool IsAxis1D() const; bool IsAxis2D() const; bool IsAxis3D() const; bool IsDigital() const; bool IsAnalog() const;
	bool IsBindableInBlueprints() const; bool ShouldUpdateAxisWithoutSamples() const; bool IsBindableToActions() const;
	FText GetDisplayName(bool bLongDisplayName = true) const; FString ToString() const; FName GetFName() const; FName GetMenuCategory() const;
	bool operator==(const FKey& Other) const; bool operator!=(const FKey& Other) const; bool operator<(const FKey& Other) const;
};
uint32 GetTypeHash(const FKey& Key);
struct EKeys
{
	static const FKey AnyKey, MouseX, MouseY, Mouse2D, MouseScrollUp, MouseScrollDown, MouseWheelAxis;
	static const FKey LeftMouseButton, RightMouseButton, MiddleMouseButton, ThumbMouseButton, ThumbMouseButton2;
	static const FKey BackSpace, Tab, Enter, Pause, CapsLock, Escape, SpaceBar, PageUp, PageDown, End, Home, Left, Up, Right, Down, Insert, Delete;
	static const FKey Zero, One, Two, Three, Four, Five, Six, Seven, Eight, Nine;
	static const FKey A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z;
	static const FKey NumPadZero, NumPadOne, NumPadTwo, NumPadThree, NumPadFour, NumPadFive, NumPadSix, NumPadSeven, NumPadEight, NumPadNine;
	static const FKey Multiply, Add, Subtract, Decimal, Divide;
	static const FKey F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12;
	static const FKey NumLock, ScrollLock, LeftShift, RightShift, LeftControl, RightControl, LeftAlt, RightAlt, LeftCommand, RightCommand;
	static const FKey Semicolon, Equals, Comma, Underscore, Hyphen, Period, Slash, Tilde, LeftBracket, Backslash, RightBracket, Apostrophe;
	static const FKey Ampersand, Asterix, Caret, Colon, Dollar, Exclamation, LeftParantheses, RightParantheses, Quote;
	static const FKey Gamepad_Left2D, Gamepad_LeftX, Gamepad_LeftY, Gamepad_Right2D, Gamepad_RightX, Gamepad_RightY, Gamepad_LeftTriggerAxis, Gamepad_RightTriggerAxis;
	static const FKey Gamepad_LeftThumbstick, Gamepad_RightThumbstick, Gamepad_Special_Left, Gamepad_Special_Left_X, Gamepad_Special_Left_Y, Gamepad_Special_Right;
	static const FKey Gamepad_FaceButton_Bottom, Gamepad_FaceButton_Right, Gamepad_FaceButton_Left, Gamepad_FaceButton_Top;
	static const FKey Gamepad_LeftShoulder, Gamepad_RightShoulder, Gamepad_LeftTrigger, Gamepad_RightTrigger;
	static const FKey Gamepad_DPad_Up, Gamepad_DPad_Down, Gamepad_DPad_Right, Gamepad_DPad_Left;
	static const FKey Gamepad_LeftStick_Up, Gamepad_LeftStick_Down, Gamepad_LeftStick_Right, Gamepad_LeftStick_Left;
	static const FKey Gamepad_RightStick_Up, Gamepad_RightStick_Down, Gamepad_RightStick_Right, Gamepad_RightStick_Left;
	static const FKey Invalid;
	static bool IsGamepadKey(FKey Key);
};
namespace ETouchIndex { enum Type { Touch1, Touch2, Touch3, Touch4, Touch5, Touch6, Touch7, Touch8, Touch9, Touch10, CursorPointerIndex, MAX_TOUCHES }; }
