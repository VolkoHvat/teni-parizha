#pragma once
// TP_STUB_MODULE: RenderCore
#include "CoreMinimal.h"
void FlushRenderingCommands(bool bFlushDeferredDeletes = false);
template<typename F> void ENQUEUE_RENDER_COMMAND_TPImpl(F&& Lambda);
struct FRHICommandListImmediate;
#define ENQUEUE_RENDER_COMMAND(Type) ENQUEUE_RENDER_COMMAND_TPImpl
