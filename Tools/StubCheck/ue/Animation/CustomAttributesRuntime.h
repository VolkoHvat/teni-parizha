// Макет Animation/CustomAttributesRuntime.h (UE 4.26–4.27): FStackCustomAttributes (в UE5 — FStackAttributeContainer).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
struct FStackCustomAttributes
{
	FStackCustomAttributes();
	void Empty();
	void CopyFrom(const FStackCustomAttributes& Other);
};
