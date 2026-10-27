// Макет UObject/ConstructorHelpers.h (UE 4.27): работает только в конструкторе; в проекте предпочтительна загрузка с запасными путями.
#pragma once
// TP_STUB_MODULE: CoreUObject
#include "CoreMinimal.h"
#include "UObject/Object.h"
namespace ConstructorHelpers
{
	template<class T> struct FObjectFinder { T* Object; FObjectFinder(const TCHAR* ObjectToFind, uint32 InLoadFlags = LOAD_None); bool Succeeded(); };
	template<class T> struct FObjectFinderOptional { FObjectFinderOptional(const TCHAR* InObjectToFind, uint32 InLoadFlags = LOAD_None); T* Get(); bool Succeeded(); };
	template<class T> struct FClassFinder { TSubclassOf<T> Class; FClassFinder(const TCHAR* ClassToFind); bool Succeeded(); };
}
