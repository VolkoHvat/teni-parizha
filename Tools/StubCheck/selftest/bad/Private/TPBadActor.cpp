// Плохие примеры: UE5-имена, неверные сигнатуры 4.27, ошибки, которые MSVC/UHT не пропустят.
#include "TPBadTypes.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "AIController.h"
// ОЖИДАЕТСЯ: из модуля AIModule, которого нет в TeniParizha.Build.cs

void ATPBadActor::SetSpeed(float NewSpeed) { Speed = NewSpeed; }
void ATPBadActor::OnBp() {}
// ОЖИДАЕТСЯ: у BlueprintImplementableEvent «OnBp» не должно быть тела

void ATPBadActor::BeginPlay()
{
	Super::BeginPlay();
	TObjectPtr<AActor> P;
	// ОЖИДАЕТСЯ: TObjectPtr — это UE5
	FVector3f V3(1.f, 2.f, 3.f);
	// ОЖИДАЕТСЯ: unknown type name 'FVector3f'
	const float A = UE_PI;
	// ОЖИДАЕТСЯ: use of undeclared identifier 'UE_PI'
	USkeletalMeshComponent* Mesh = FindComponentByClass<USkeletalMeshComponent>();
	Mesh->GetSkeletalMeshAsset();
	// ОЖИДАЕТСЯ: no member named 'GetSkeletalMeshAsset'
	UTexture2D* T = UTexture2D::CreateTransient(4, 4);
	T->GetPlatformData();
	// ОЖИДАЕТСЯ: no member named 'GetPlatformData'
	const double D = 2.0;
	const float M = FMath::Max(D, 1.f);
	// ОЖИДАЕТСЯ: no matching function for call to 'Max'
	FTimerHandle H;
	GetWorld()->GetTimerManager().SetTimer(H, FTimerDelegate::CreateUObject(this, &ATPBadActor::BeginPlay), 1.f);
	// ОЖИДАЕТСЯ: no matching member function for call to 'SetTimer'
	FString S = TEXT("x");
	UE_LOG(LogTemp, Log, TEXT("%s"), S);
	// ОЖИДАЕТСЯ: В UE_LOG переданы недопустимые аргументы
	FString Bad = FString::Printf("%d", 3);
	// ОЖИДАЕТСЯ: Строка формата FString::Printf должна быть массивом TCHAR
	UMaterialInstanceDynamic* Mid = nullptr;
	Mid->SetVectorParameterValue(TEXT("Color"), FVector(1.f, 0.f, 0.f));
	// ОЖИДАЕТСЯ: no viable conversion from 'FVector' to 'const FLinearColor'
	TArray<AActor*> Arr;
	Arr.Sort([](const AActor* L, const AActor* R) { return L < R; });
	// ОЖИДАЕТСЯ: no matching function for call to object of type
	TArray<ACharacter*> Chars;
	UGameplayStatics::GetAllActorsOfClass(this, ACharacter::StaticClass(), Chars);
	// ОЖИДАЕТСЯ: cannot bind to a value of unrelated type 'TArray<ACharacter *>'
	Arr.RemoveAt(0, false);
	// ОЖИДАЕТСЯ: второй параметр — Count, а не bAllowShrinking
	float Speed = 1.f;
	// ОЖИДАЕТСЯ: declaration shadows a field of 'ATPBadActor'
	Speed += A + M + V3.X;
}
