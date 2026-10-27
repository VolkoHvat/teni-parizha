#include "TPBadTypes.h"
#include "Components/PrimitiveComponent.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTPBadSig, float, Amount);
static FTPBadSig GBadSig;
void ATPBadActor::Handler(float Amount) {}
static void TPHelper() {}
// ОЖИДАЕТСЯ: redefinition of 'TPHelper'
void TPBindBad(ATPBadActor* Actor)
{
	GBadSig.AddDynamic(Actor, &ATPBadActor::Handler);
	// ОЖИДАЕТСЯ: AddDynamic: функция ATPBadActor::Handler не помечена UFUNCTION()
	GBadSig.AddDynamic(Actor, &ATPBadActor::SetSpeed);
	TPHelper();
}
