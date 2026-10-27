// Макет Sound/SoundWaveProcedural.h (UE 4.27): процедурный звук (этапы 13–14).
#pragma once
// TP_STUB_MODULE: Engine
#include "CoreMinimal.h"
#include "Sound/SoundWave.h"
DECLARE_DELEGATE_TwoParams(FOnSoundWaveProceduralUnderflow, class USoundWave*, int32);
class USoundWaveProcedural : public USoundWave
{
public:
	typedef USoundWave Super; typedef USoundWaveProcedural ThisClass; static UClass* StaticClass();
	USoundWaveProcedural(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	FOnSoundWaveProceduralUnderflow OnSoundWaveProceduralUnderflow;
	void QueueAudio(const uint8* AudioData, const int32 BufferSize);
	void ResetAudio();
	int32 GetAvailableAudioByteCount();
	virtual int32 GeneratePCMData(uint8* PCMData, const int32 SamplesNeeded);
	int32 NumBufferUnderrunSamples; int32 NumSamplesToGeneratePerCallback;
};
