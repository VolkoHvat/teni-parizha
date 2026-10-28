// Тени Парижа — анимация без AnimBP: свой прокси анимации считает позу в рабочем потоке.
// Локомоция — клипы манекена UE4 (стойка, шаг, бег, прыжок) со смешиванием по скорости и общей фазой;
// если клипов нет или они от другого скелета — процедурная походка (TPAnimLocomotion.h).
// Присед (в пакете Third Person нет клипов) — процедурный слой поверх клипов.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"
#include "TPTraversalAnimInstance.h"
#include "TPAnimLocomotion.h"
#include "TPAnimInstance.generated.h"

class UAnimSequence;
class UTPAnimInstance;

/**
 * Прокси: GatherInput (игровой поток) копирует входы из UTPAnimInstance, Update и Evaluate
 * (рабочий поток) не трогают UObject, кроме чтения клипов UAnimSequence.
 */
USTRUCT()
struct TENIPARIZHA_API FTPAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FTPAnimInstanceProxy() {}
	FTPAnimInstanceProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	/** включена ли процедурная походка (для отладки и HUD) */
	bool IsProcedural() const { return bProcedural; }

	/** игровой поток: забирает входы кадра из экземпляра (вызывается из NativeUpdateAnimation, без запаздывания на кадр) */
	void GatherInput(const UTPAnimInstance* Inst);

protected:
	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void Update(float DeltaSeconds) override;
	virtual bool Evaluate(FPoseContext& Output) override;

private:
	/** клипы в порядке TPAnim::ELocoClip (копия указателей из экземпляра; живут благодаря его UPROPERTY) */
	UPROPERTY(Transient) TArray<UAnimSequence*> Clips;

	FName BoneFNames[TPAnim::B_Count];
	TPAnim::FTPLocoParams Params;
	TPAnim::FTPLocoInput Input;
	TPAnim::FTPLocoState Loco;
	TPAnim::FTPRig Rig;
	FQuat MeshRot = FQuat::Identity;   // поворот меша относительно актёра
	FQuat RigMeshRot = FQuat::Identity;
	bool bClipsReady = false;    // все клипы есть и от скелета этого меша
	bool bForceProcedural = false;
	bool bProcedural = false;    // итог: процедурная поза вместо клипов
	bool bRigBuilt = false;
	bool bRigCompatible = false; // в меше есть все 23 кости и родители совпадают

	/** индексы костей TPAnim::EBone в позе текущего LOD (INDEX_NONE — кость отсечена LOD) */
	void MapBones(const FCompactPose& Pose, int32* OutIdx) const;
	void BuildRig(const FCompactPose& Pose, const int32* Idx);
	/** накладывает процедурную позу на кости из Idx с весом Weight (1 — заменить) */
	void ApplyProcedural(FCompactPose& Pose, const int32* Idx, const TPAnim::FTPPoseCS& P, float Weight) const;
	/** смешивает клипы по весам в Output; false — ни одного клипа не сэмплировано */
	bool SampleClips(FPoseContext& Output, const TPAnim::FTPLocoOutput& W) const;
};

/**
 * Класс анимации героя, стражи и жителей без AnimBP. Назначение: Mesh->SetAnimInstanceClass(UTPAnimInstance::StaticClass())
 * или UTPAnimSetup::ApplyAnimation. Переменные паркура (Speed, bInAir, State…) наследуются от UTPTraversalAnimInstance.
 * Ограничение этапа 6: монтажи (слоты) этим классом не проигрываются — позы паркура и боя добавляет этап 7.
 */
UCLASS(Blueprintable)
class TENIPARIZHA_API UTPAnimInstance : public UTPTraversalAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** клипы локомоции; пустые подгружаются из пакета Third Person по запасным путям */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* IdleAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* WalkAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* RunAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* JumpStartAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* JumpLoopAnim = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация|Клипы") UAnimSequence* JumpEndAnim = nullptr;

	/** скорость, под которую снят клип шага, см/с */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация") float WalkAnimSpeed = 150.f;
	/** скорость клипа бега, см/с */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация") float RunAnimSpeed = 375.f;
	/** наибольшее ускорение клипа бега (спринт) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация") float MaxPlayRate = 1.7f;
	/** всегда процедурная походка (даже если клипы есть) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Анимация") bool bForceProcedural = false;

	/** клип по индексу TPAnim::ELocoClip */
	UAnimSequence* GetClip(int32 Clip) const;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** подгружает пустые клипы по путям пакета Third Person (без падения, если их нет) */
	void LoadDefaultClips();
};
