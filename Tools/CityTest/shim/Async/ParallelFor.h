// Тени Парижа — замена Async/ParallelFor.h для проверки обычным g++: последовательный цикл.
#pragma once
#include "CoreMinimal.h"
template<class F> inline void ParallelFor(int32 Num, F Body, bool bForceSingleThread = false) { (void)bForceSingleThread; for (int32 I = 0; I < Num; ++I) Body(I); }
