// Тени Парижа — анимация без AnimBP: свой прокси анимации считает позу в рабочем потоке.
// Локомоция — клипы манекена UE4 (стойка, шаг, бег, прыжок) со смешиванием по скорости и общей фазой;
// если клипов нет или они от другого скелета — процедурная походка (TPAnimLocomotion.h).
// Присед (в пакете Third Person нет клипов) — процедурный слой поверх клипов.
// Этап 7: позы паркура и боя (TPAnimActions.h) — слой действий поверх локомоции с маской «всё тело»/«верх»,
// подмена поз клипами (OverrideSequences, например из Mixamo), наклон в повороте, взгляд и IK стоп.
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstanceProxy.h"
#include "TPTraversalAnimInstance.h"
#include "TPAnimLocomotion.h"
#include "TPAnimActions.h"
#include "TPAnimInstance.generated.h"

class UAnimSequence;
class UTPAnimInstance;
class UTPCombatComponent;

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
	/** клипы-подмены поз действий в порядке TPAnim::EAction (null — процедурная поза) */
	UPROPERTY(Transient) TArray<UAnimSequence*> ActionClips;

	// слой действий (этап 7)
	TPAnim::FTPActionLayer Layer;
	TPAnim::FTPActionContext ActionCtx;
	int32 Action = TPAnim::A_None;
	float ActionU = 0.f;
	bool bLayerPose = false;   // Layer.Pose посчитана на текущем скелете
	// постобработка
	TPAnim::FTPFootIKInput FootIK;
	float Lean = 0.f;          // рад, + вправо
	float LookYaw = 0.f, LookPitch = 0.f, LookWeight = 0.f;

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
	/** накладывает процедурную позу с весами масок: нижняя (таз, ноги) и верхняя (позвоночник, голова, руки) */
	void ApplyProceduralMasked(FCompactPose& Pose, const int32* Idx, const TPAnim::FTPPoseCS& P, float WLower, float WUpper) const;
	/** подмена позы действия клипом: сэмплирует клип и смешивает по маскам; false — клипа нет */
	bool ApplyOverrideClip(FPoseContext& Output, const int32* Idx) const;
	/** 23 кости позы → пространство персонажа и обратно (для наклона, взгляда и IK стоп) */
	bool PoseToBones(const FCompactPose& Pose, const int32* Idx, TPAnim::FTPBonesCS& O) const;
	void BonesToPose(FCompactPose& Pose, const int32* Idx, const TPAnim::FTPBonesCS& O) const;
	/** наклон, взгляд и IK стоп поверх готовой позы */
	void PostProcess(FCompactPose& Pose, const int32* Idx) const;
};

/**
 * Класс анимации героя, стражи и жителей без AnimBP. Назначение: Mesh->SetAnimInstanceClass(UTPAnimInstance::StaticClass())
 * или UTPAnimSetup::ApplyAnimation. Переменные паркура (Speed, bInAir, State…) наследуются от UTPTraversalAnimInstance.
 * Позы паркура и боя — процедурные (TPAnimActions.h) по состоянию UTPTraversalComponent и таймерам UTPCombatComponent;
 * любую можно подменить клипом через OverrideSequences. Монтажи (слоты) этим классом не проигрываются.
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

	/**
	 * Подмена процедурных поз действий клипами. Ключ — имя действия: Hang, Climb, ClimbUp, Mantle, Vault, WallRun, Roll,
	 * Turn, Drop, Hop, LandHard, Guard, Attack1, Attack2, Attack3, Counter, Block, Dodge, WindUp, Strike, HitReact, Stagger.
	 * Клип должен быть на скелете этого меша (анимации Mixamo — после ретаргета на скелет SK_Mannequin);
	 * циклы (Hang, Climb, WallRun, Guard, Block) повторяются, остальные растягиваются на длительность действия.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Анимация|Клипы") TMap<FName, UAnimSequence*> OverrideSequences;

	/** IK стоп на неровной земле (две трассировки вниз за кадр) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Анимация|IK") bool bFootIK = true;
	/** поворот головы: в бою — к цели, иначе — по направлению камеры */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Анимация|IK") bool bLookAt = true;
	/** IK стоп и взгляд только у персонажа игрока (стража и жители экономят трассировки) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Анимация|IK") bool bIKOnlyForPlayer = true;
	/** наклон корпуса в повороте на бегу (доля от угла паркура) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Анимация|IK") float LeanScale = 1.f;

	/** клип по индексу TPAnim::ELocoClip */
	UAnimSequence* GetClip(int32 Clip) const;

	/* ---- данные кадра для прокси (игровой поток пишет, прокси читает в GatherInput) ---- */
	int32 FrameAction = TPAnim::A_None;
	float FrameActionU = 0.f;
	TPAnim::FTPActionContext FrameCtx;
	TPAnim::FTPFootIKInput FrameFootIK;
	float FrameLean = 0.f;
	float FrameLookYaw = 0.f, FrameLookPitch = 0.f, FrameLookWeight = 0.f;
	/** клипы-подмены по TPAnim::EAction (обновляются при изменении OverrideSequences) */
	UAnimSequence* GetActionClip(int32 ActionIndex) const;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** подгружает пустые клипы по путям пакета Third Person (без падения, если их нет) */
	void LoadDefaultClips();

	UPROPERTY(Transient) UTPCombatComponent* Combat = nullptr;
	bool bCombatSearched = false;

	// игровой поток: выбор действия и контекста, трассировки стоп, взгляд
	ETPTraversal PrevState = ETPTraversal::Ground;
	float StateClock = 0.f;
	float LoopPhase = 0.f;
	int32 LoopAction = TPAnim::A_None;
	TPAnim::FTPTimerTrack DodgeTrack, ReactTrack, StaggerTrack, SwingTrack, WindUpTrack;
	float FootOffsetS[2];      // задаются в NativeInitializeAnimation
	FVector FootNormalS[2];
	float FootIKWeightS = 0.f;
	float LookYawS = 0.f, LookPitchS = 0.f, LookWeightS = 0.f;

	/** выбор позы действия и её доли выполнения по паркуру и бою */
	void UpdateAction(float DeltaSeconds);
	/** мировая точка → пространство персонажа (корень меша, X вперёд) */
	FVector ToCharacterSpace(const FVector& World) const;
	FVector ToCharacterDir(const FVector& WorldDir) const;
	void UpdateFootIK(float DeltaSeconds);
	void UpdateLookAt(float DeltaSeconds);
	bool UseIK() const;
};
