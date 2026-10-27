#pragma once
// TP_STUB_MODULE: RenderCore
#include "CoreMinimal.h"
class FRenderResource { public: virtual ~FRenderResource(); virtual void InitRHI(); virtual void ReleaseRHI(); virtual FString GetFriendlyName() const; bool IsInitialized() const; };
class FTexture : public FRenderResource { public: virtual uint32 GetSizeX() const; virtual uint32 GetSizeY() const; virtual uint32 GetSizeZ() const; };
extern FTexture* GWhiteTexture; extern FTexture* GBlackTexture;
template<class ResourceType> class TGlobalResource : public ResourceType {};
void BeginInitResource(FRenderResource* Resource); void BeginReleaseResource(FRenderResource* Resource); void ReleaseResourceAndFlush(FRenderResource* Resource);
