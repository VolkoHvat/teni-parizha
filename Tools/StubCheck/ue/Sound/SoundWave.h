#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Sound/SoundBase.h"
class USoundWave : public USoundBase
{
public:
	typedef USoundBase Super; typedef USoundWave ThisClass; static UClass* StaticClass();
	USoundWave(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	float Volume; float Pitch; int32 NumChannels; uint8 bLooping : 1; uint8 bProcedural : 1;
	void SetSampleRate(uint32 InSampleRate);
	virtual bool IsLooping() override;
};
