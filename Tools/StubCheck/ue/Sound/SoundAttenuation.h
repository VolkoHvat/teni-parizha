#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "UObject/Object.h"
struct FSoundAttenuationSettings { uint8 bAttenuate : 1; uint8 bSpatialize : 1; float FalloffDistance; FVector AttenuationShapeExtents; float ConeOffset; FSoundAttenuationSettings(); };
class USoundAttenuation : public UObject { public: typedef UObject Super; static UClass* StaticClass(); FSoundAttenuationSettings Attenuation; };
