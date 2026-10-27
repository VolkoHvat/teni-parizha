// Тени Парижа — процедурный квартал: выгрузка геометрии TPCityBuilder в UProceduralMeshComponent и материалы
// из процедурных текстур TPTexGen. Владелец модуля: агент «город».
//
// Координаты: построитель работает в системе Lutèce (метры, Y вверх, правая тройка); здесь
// (x, y, z) → (x·100, z·100, y·100) см. Перестановка осей — отражение, переводящее правую тройку в левую, поэтому
// порядок индексов треугольников сохраняется (лицевые грани остаются лицевыми), нормали переводятся той же перестановкой.
#include "TPCityGenerator.h"
#include "TeniParizha.h"
#include "TPTypes.h"
#include "TPCityBuilder.h"
#include "TPTexGen.h"
#include "ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"
#include "HAL/PlatformTime.h"

const FName ATPCityGenerator::TagCityChunk(TEXT("TPCityChunk"));
const FName ATPCityGenerator::TagSoft(TEXT("TPSoft"));

namespace TPCityGenImpl
{
	/** вид материала Unreal для материала квартала */
	enum class EMatKind : uint8 { Opaque, Masked, Emissive };

	/** описание материала (порт таблицы lu_city_materials): линейные цвета палитры, разброс, шероховатость, металл, повтор UV */
	struct FMatDesc
	{
		TPTexGen::EType Tex;
		float C0[3];
		float C1[3];
		float Var;
		float Rough;
		float Metal;
		float UVScale;
		EMatKind Kind;
	};

	// порядок — как TPCity::EMat
	static const FMatDesc MatDescs[TPCity::CM_Num] = {
		{ TPTexGen::EType::StoneBlocks, { .62f, .56f, .45f }, { .74f, .68f, .56f }, .12f, .80f, 0.f, 1.f, EMatKind::Opaque },   // камень (известняк)
		{ TPTexGen::EType::Plaster,     { .70f, .66f, .58f }, { .80f, .76f, .68f }, .10f, .90f, 0.f, 1.f, EMatKind::Opaque },   // штукатурка
		{ TPTexGen::EType::Brick,       { .42f, .17f, .10f }, { .55f, .25f, .15f }, .15f, .85f, 0.f, 1.f, EMatKind::Opaque },   // кирпич
		{ TPTexGen::EType::Slate,       { .13f, .15f, .19f }, { .20f, .22f, .27f }, .20f, .50f, 0.f, 1.f, EMatKind::Opaque },   // шифер
		{ TPTexGen::EType::RoofTiles,   { .45f, .17f, .08f }, { .60f, .27f, .13f }, .20f, .70f, 0.f, 1.f, EMatKind::Opaque },   // черепица
		{ TPTexGen::EType::Wood,        { .35f, .23f, .14f }, { .50f, .34f, .20f }, .15f, .75f, 0.f, 1.f, EMatKind::Opaque },   // дерево
		{ TPTexGen::EType::Shutter,     { .80f, .80f, .80f }, { .90f, .90f, .90f }, .05f, .70f, 0.f, 1.f, EMatKind::Opaque },   // ставни (цвет в вершинах)
		{ TPTexGen::EType::Metal,       { .05f, .05f, .06f }, { .08f, .08f, .09f }, .10f, .45f, 1.f, 1.f, EMatKind::Opaque },   // кованое железо
		{ TPTexGen::EType::Flat,        { .90f, .90f, .90f }, { .90f, .90f, .90f }, 0.f,  .08f, .2f, 1.f, EMatKind::Opaque },   // стекло
		{ TPTexGen::EType::Cobble,      { .30f, .29f, .27f }, { .45f, .43f, .40f }, .20f, .80f, 0.f, .5f, EMatKind::Opaque },   // булыжник
		{ TPTexGen::EType::Flagstone,   { .50f, .47f, .42f }, { .62f, .58f, .52f }, .10f, .80f, 0.f, .35f, EMatKind::Opaque },  // плиты
		{ TPTexGen::EType::Cloth,       { .90f, .90f, .90f }, { 1.f,  1.f,  1.f  }, .05f, .95f, 0.f, 2.f, EMatKind::Masked },   // ткань (двусторонняя)
		{ TPTexGen::EType::Flat,        { 1.f,  1.f,  1.f  }, { 1.f,  1.f,  1.f  }, 0.f,  .60f, 0.f, 1.f, EMatKind::Opaque },   // крашеное дерево (переплёты)
		{ TPTexGen::EType::Dirt,        { .60f, .55f, .45f }, { .75f, .70f, .55f }, .20f, .95f, 0.f, 1.f, EMatKind::Opaque },   // земля / сено
		{ TPTexGen::EType::Leaves,      { .75f, .80f, .70f }, { 1.f,  1.f,  1.f  }, .15f, .70f, 0.f, 1.f, EMatKind::Masked },   // листва (цвет в вершинах)
		{ TPTexGen::EType::Wood,        { .25f, .20f, .16f }, { .35f, .28f, .22f }, .10f, .90f, 0.f, 2.f, EMatKind::Opaque },   // кора
		{ TPTexGen::EType::Flat,        { 1.f,  .80f, .50f }, { 1.f,  .80f, .50f }, 0.f,  .30f, 0.f, 1.f, EMatKind::Emissive }, // стекло фонаря
		{ TPTexGen::EType::Flat,        { .12f, .20f, .22f }, { .12f, .20f, .22f }, 0.f,  .05f, 0.f, 1.f, EMatKind::Opaque },   // вода
	};

	/** цвет свечения фонаря и сила при полной яркости */
	static const FLinearColor LampColor(1.f, .65f, .3f, 1.f);
	static constexpr float LampIntensity = 6.f;

	/** кэш материалов (общий для мира редактора и PIE; живут, пока на них ссылаются компоненты) */
	struct FMatCache
	{
		TArray<TWeakObjectPtr<UMaterialInterface>> Mats;
		TWeakObjectPtr<UMaterialInstanceDynamic> Lamp;
		bool bLampEmissive = false;
		bool bTextured = false;
		int32 TexSize = 0;
	};
	static FMatCache& Cache()
	{
		static FMatCache C;
		return C;
	}

	static UMaterialInterface* LoadMaterial(const TCHAR* Path)
	{
		return LoadObject<UMaterialInterface>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** текстура из готовых мип-уровней (B8G8R8A8, FColor совпадает с раскладкой формата) */
	static UTexture2D* MakeTexture(const TArray<TArray<FColor>>& Mips, int32 N, bool bSRGB, TextureCompressionSettings Compression, const FString& BaseName)
	{
		if (Mips.Num() == 0 || N <= 0 || Mips[0].Num() != N * N) return nullptr;
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), FName(*BaseName));
		UTexture2D* Tex = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8, Name);
		if (!Tex) return nullptr;
		Tex->SRGB = bSRGB;
		Tex->CompressionSettings = Compression;
		Tex->NeverStream = true;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
		FTexturePlatformData* PD = Tex->GetPlatformData();
		if (!PD || PD->Mips.Num() == 0) return nullptr;
		{
			FTexture2DMipMap& Mip0 = PD->Mips[0];
			void* Data = Mip0.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, Mips[0].GetData(), (SIZE_T)N * N * sizeof(FColor));
			Mip0.BulkData.Unlock();
		}
		// остальные мип-уровни (без них мостовая и фасады рябят вдали)
		int32 S = N;
		for (int32 L = 1; L < Mips.Num(); ++L)
		{
			S = FMath::Max(1, S / 2);
			if (Mips[L].Num() != S * S) break;
			FTexture2DMipMap* Mip = new FTexture2DMipMap(S, S, 1);   // так же создаёт нулевой уровень сам CreateTransient
			PD->Mips.Add(Mip);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			void* Data = Mip->BulkData.Realloc((int64)S * S * (int64)sizeof(FColor));
			FMemory::Memcpy(Data, Mips[L].GetData(), (SIZE_T)S * S * sizeof(FColor));
			Mip->BulkData.Unlock();
		}
		Tex->UpdateResource();
		return Tex;
	}

	/** материалы квартала по индексу TPCity::EMat; при отсутствии базового материала — запасной BasicShapeMaterial */
	static void GetMaterials(int32 TexSize, TArray<UMaterialInterface*>& Out)
	{
		FMatCache& C = Cache();
		Out.Reset();
		// базовый материал мог появиться после первой генерации (скрипт tp_setup.py) — тогда кэш с запасными материалами сбрасываем
		UMaterialInterface* Base = LoadMaterial(TPPaths::BaseMaterial);
		const bool bTextured = Base != nullptr;
		bool bValid = C.Mats.Num() == TPCity::CM_Num && C.TexSize == TexSize && C.bTextured == bTextured;
		for (int32 I = 0; bValid && I < C.Mats.Num(); ++I) bValid = C.Mats[I].IsValid();
		if (bValid)
		{
			for (const TWeakObjectPtr<UMaterialInterface>& M : C.Mats) Out.Add(M.Get());
			return;
		}

		const double T0 = FPlatformTime::Seconds();
		UMaterialInterface* Masked = LoadMaterial(TPPaths::BaseMaterialMasked);
		UMaterialInterface* Emissive = LoadMaterial(TPPaths::EmissiveMaterial);
		UMaterialInterface* Basic = LoadMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		UMaterialInterface* Fallback = Basic ? Basic : UMaterial::GetDefaultMaterial(MD_Surface);
		if (!Masked) Masked = Base;
		if (!bTextured)
		{
			UE_LOG(LogTeni, Warning, TEXT("Город: нет материала %s — запустите Content/Python/tp_setup.py. Пока используется простой материал без текстур."), TPPaths::BaseMaterial);
		}

		C.Mats.Reset();
		C.Lamp = nullptr;
		C.bLampEmissive = false;
		for (int32 I = 0; I < TPCity::CM_Num; ++I)
		{
			const FMatDesc& D = MatDescs[I];
			UMaterialInstanceDynamic* MID = nullptr;
			if (D.Kind == EMatKind::Emissive)
			{
				C.bLampEmissive = Emissive != nullptr;
				MID = UMaterialInstanceDynamic::Create(Emissive ? Emissive : Fallback, GetTransientPackage());
				if (MID)
				{
					MID->SetVectorParameterValue(TEXT("Color"), LampColor);
					MID->SetScalarParameterValue(TEXT("Intensity"), LampIntensity);
				}
				C.Lamp = MID;
			}
			else
			{
				TPTexGen::FPalette Pal;
				Pal.C0 = TPTexGen::FRGB(D.C0[0], D.C0[1], D.C0[2]);
				Pal.C1 = TPTexGen::FRGB(D.C1[0], D.C1[1], D.C1[2]);
				Pal.Var = D.Var;
				const bool bFlat = D.Tex == TPTexGen::EType::Flat;
				// без базового материала нужен только средний цвет — хватает маленькой текстуры
				const int32 N = bFlat ? 4 : (bTextured ? TexSize : 32);
				TPTexGen::FSet Set;
				TPTexGen::Generate(Set, D.Tex, N, Pal, 1000u + (uint32)I * 17u, D.Rough, D.Metal, bTextured);
				if (bTextured)
				{
					UMaterialInterface* Parent = D.Kind == EMatKind::Masked ? Masked : Base;
					const FString Prefix = FString::Printf(TEXT("T_TPCity%02d"), I);
					UTexture2D* Alb = MakeTexture(Set.Albedo, Set.N, true, TC_Default, Prefix + TEXT("_Albedo"));
					UTexture2D* Nrm = MakeTexture(Set.Normal, Set.N, false, TC_Normalmap, Prefix + TEXT("_Normal"));
					UTexture2D* Orm = MakeTexture(Set.ORM, Set.N, false, TC_Masks, Prefix + TEXT("_ORM"));
					MID = UMaterialInstanceDynamic::Create(Parent, GetTransientPackage());
					if (MID)
					{
						if (Alb) MID->SetTextureParameterValue(TEXT("Albedo"), Alb);
						if (Nrm) MID->SetTextureParameterValue(TEXT("Normal"), Nrm);
						if (Orm) MID->SetTextureParameterValue(TEXT("ORM"), Orm);
						MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor::White);
						MID->SetScalarParameterValue(TEXT("UVScale"), D.UVScale);
					}
				}
				else
				{
					MID = UMaterialInstanceDynamic::Create(Fallback, GetTransientPackage());
					if (MID) MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(Set.Average.R, Set.Average.G, Set.Average.B, 1.f));
				}
			}
			UMaterialInterface* Result = MID ? (UMaterialInterface*)MID : Fallback;
			C.Mats.Add(Result);
			Out.Add(Result);
		}
		C.TexSize = TexSize;
		C.bTextured = bTextured;
		UE_LOG(LogTeni, Log, TEXT("Город: материалы и текстуры %dx%d — %.0f мс"), TexSize, TexSize, (FPlatformTime::Seconds() - T0) * 1000.0);
	}

	/** Lutèce (м, Y вверх) → Unreal (см, Z вверх) */
	static FORCEINLINE FVector ToUE(const TPCity::FV3& P) { return FVector(P.X * 100.f, P.Z * 100.f, P.Y * 100.f); }
	static FORCEINLINE FVector ToUEDir(const TPCity::FV3& N) { return FVector(N.X, N.Z, N.Y); }

	/** касательные: TangentX = dP/dU, флаг переворота — если Cross(N, TangentX) смотрит против dP/dV (как в шейдерах UE) */
	static void ComputeTangents(const TArray<FVector>& P, const TArray<FVector>& N, const TArray<FVector2D>& UV, const TArray<int32>& Idx, TArray<FProcMeshTangent>& Out)
	{
		const int32 NV = P.Num();
		TArray<FVector> TU, TV;
		TU.Init(FVector::ZeroVector, NV);
		TV.Init(FVector::ZeroVector, NV);
		for (int32 K = 0; K + 2 < Idx.Num(); K += 3)
		{
			const int32 I0 = Idx[K], I1 = Idx[K + 1], I2 = Idx[K + 2];
			const FVector E1 = P[I1] - P[I0], E2 = P[I2] - P[I0];
			const double S1 = UV[I1].X - UV[I0].X, S2 = UV[I2].X - UV[I0].X;
			const double T1 = UV[I1].Y - UV[I0].Y, T2 = UV[I2].Y - UV[I0].Y;
			const double R = S1 * T2 - S2 * T1;
			if (FMath::Abs(R) < 1e-12) continue;
			const double InvR = 1.0 / R;
			const FVector SDir = (E1 * T2 - E2 * T1) * InvR;   // dP/dU
			const FVector TDir = (E2 * S1 - E1 * S2) * InvR;   // dP/dV
			TU[I0] += SDir; TU[I1] += SDir; TU[I2] += SDir;
			TV[I0] += TDir; TV[I1] += TDir; TV[I2] += TDir;
		}
		Out.SetNum(NV);
		for (int32 I = 0; I < NV; ++I)
		{
			const FVector& Nn = N[I];
			FVector T = TU[I] - Nn * FVector::DotProduct(Nn, TU[I]);
			if (!T.Normalize())
			{
				T = FVector::CrossProduct(FMath::Abs(Nn.Z) < 0.9 ? FVector(0.0, 0.0, 1.0) : FVector(1.0, 0.0, 0.0), Nn);
				if (!T.Normalize()) T = FVector(1.0, 0.0, 0.0);
			}
			const bool bFlipY = FVector::DotProduct(FVector::CrossProduct(Nn, T), TV[I]) < 0.0;
			Out[I] = FProcMeshTangent(T, bFlipY);
		}
	}

	/** таблица sRGB → линейный (как pow(c, 2.2) в шейдере Lutèce) для цвета вершин */
	struct FSrgbToLinear
	{
		float V[256];
		FSrgbToLinear() { for (int32 I = 0; I < 256; ++I) V[I] = FMath::Pow((float)I / 255.f, 2.2f); }
	};
	static const FSrgbToLinear& SrgbTable()
	{
		static const FSrgbToLinear Table;
		return Table;
	}

	/** видимая секция: позиции, нормали, UV, цвет вершин (линейный множитель альбедо), касательные; без столкновений */
	static void AddVisualSection(UProceduralMeshComponent* Comp, int32 Section, const TPCity::FMeshBuf& M)
	{
		const int32 NV = M.Pos.Num();
		TArray<FVector> V, N;
		TArray<FVector2D> UV;
		TArray<FLinearColor> Col;
		TArray<FProcMeshTangent> Tan;
		V.SetNumUninitialized(NV);
		N.SetNumUninitialized(NV);
		UV.SetNumUninitialized(NV);
		Col.SetNumUninitialized(NV);
		const FSrgbToLinear& L = SrgbTable();
		for (int32 I = 0; I < NV; ++I)
		{
			V[I] = ToUE(M.Pos[I]);
			N[I] = ToUEDir(M.Nrm[I]);
			UV[I] = FVector2D(M.UV[I].X, M.UV[I].Y);
			const FColor& C = M.Col[I];
			Col[I] = FLinearColor(L.V[C.R], L.V[C.G], L.V[C.B], (float)C.A / 255.f);
		}
		ComputeTangents(V, N, UV, M.Idx, Tan);
		Comp->CreateMeshSection_LinearColor(Section, V, M.Idx, N, UV, Col, Tan, false);
	}

	/** невидимая секция столкновений (простые коробки и скаты, как lu_phys в оригинале) */
	static void AddCollisionSection(UProceduralMeshComponent* Comp, int32 Section, const TPCity::FMeshBuf& M)
	{
		TArray<FVector> V, N;
		V.SetNumUninitialized(M.Pos.Num());
		N.SetNumUninitialized(M.Pos.Num());
		for (int32 I = 0; I < M.Pos.Num(); ++I)
		{
			V[I] = ToUE(M.Pos[I]);
			N[I] = ToUEDir(M.Nrm[I]);
		}
		const TArray<FVector2D> NoUV;
		const TArray<FLinearColor> NoCol;
		const TArray<FProcMeshTangent> NoTan;
		Comp->CreateMeshSection_LinearColor(Section, V, M.Idx, N, NoUV, NoCol, NoTan, true);
		Comp->SetMeshSectionVisible(Section, false);
	}

	/** новый компонент квартала (не сохраняется в уровень: всё строится заново при загрузке/старте) */
	static UProceduralMeshComponent* NewCityComponent(AActor* Owner, const TCHAR* BaseName, FName Profile)
	{
		const FName Name = MakeUniqueObjectName(Owner, UProceduralMeshComponent::StaticClass(), FName(BaseName));
		UProceduralMeshComponent* Comp = NewObject<UProceduralMeshComponent>(Owner, Name, RF_Transient);
		Comp->bUseComplexAsSimpleCollision = true;
		Comp->bUseAsyncCooking = false;   // столкновения нужны сразу: герой стоит на мостовой в первом же кадре
		Comp->SetMobility(EComponentMobility::Static);
		Comp->SetCollisionProfileName(Profile);
		Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Comp->SetCastShadow(true);
		Comp->ComponentTags.Add(ATPCityGenerator::TagCityChunk);
		return Comp;
	}
}

ATPCityGenerator::ATPCityGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;
#if WITH_EDITORONLY_DATA
	// не перестраивать квартал на каждом кадре перетаскивания в редакторе
	bRunConstructionScriptOnDrag = false;
#endif
}

void ATPCityGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (!bGenerateOnConstruction) return;
	const UWorld* W = GetWorld();
	// в игре квартал строится в BeginPlay (или явным Generate из режима игры) — не дважды
	if (W && W->IsGameWorld()) return;
	const int32 TexN = FMath::Clamp((int32)FMath::RoundUpToPowerOfTwo((uint32)FMath::Max(TextureSize, 16)), 128, 1024);
	if (HasCity() && GeneratedSeed == Seed && GeneratedTexSize == TexN)
	{
		UpdateWorldPoints();   // актор просто передвинули — геометрия едет вместе с ним
		return;
	}
	Generate();
}

void ATPCityGenerator::BeginPlay()
{
	Super::BeginPlay();
	if (!HasCity()) Generate();
	else UpdateWorldPoints();
}

bool ATPCityGenerator::HasCity() const
{
	if (Chunks.Num() == 0) return false;
	for (const TObjectPtr<UProceduralMeshComponent>& C : Chunks)
	{
		if (!IsValid(C) || !C->IsRegistered()) return false;
	}
	return true;
}

void ATPCityGenerator::ClearCity()
{
	// все компоненты квартала, в т. ч. оставшиеся от прошлых генераций или скопированные вместе с актором
	TInlineComponentArray<UProceduralMeshComponent*> Comps(this);
	for (UProceduralMeshComponent* C : Comps)
	{
		if (IsValid(C) && C->ComponentHasTag(TagCityChunk)) C->DestroyComponent();
	}
	for (const TObjectPtr<UProceduralMeshComponent>& C : Chunks)
	{
		if (IsValid(C)) C->DestroyComponent();
	}
	Chunks.Reset();
	NumTriangles = 0;
	NumBuildings = 0;
	GeneratedSeed = MIN_int32;
	GeneratedTexSize = 0;
}

void ATPCityGenerator::Generate()
{
	using namespace TPCityGenImpl;
	const double T0 = FPlatformTime::Seconds();
	ClearCity();

	// 1. геометрия (в метрах Lutèce)
	TPCity::FBuilder B;
	B.BuildLevel(Seed);
	const double T1 = FPlatformTime::Seconds();

	// 2. материалы
	const int32 TexN = FMath::Clamp((int32)FMath::RoundUpToPowerOfTwo((uint32)FMath::Max(TextureSize, 16)), 128, 1024);
	TArray<UMaterialInterface*> Mats;
	GetMaterials(TexN, Mats);
	const double T2 = FPlatformTime::Seconds();

	// 3. ячейки: секция на материал + невидимая секция столкновений
	static const FName ClimbProfile(TEXT("TPClimbable"));
	const int32 NumChunks = B.CW * B.CH;
	for (int32 Ci = 0; Ci < NumChunks; ++Ci)
	{
		bool bAny = !B.Coll[Ci].IsEmpty();
		for (int32 Mi = 0; Mi < TPCity::CM_Num && !bAny; ++Mi) bAny = !B.Bufs[Ci * TPCity::CM_Num + Mi].IsEmpty();
		if (!bAny) continue;
		const FString Name = FString::Printf(TEXT("CityChunk_%d_%d"), Ci % B.CW, Ci / B.CW);
		UProceduralMeshComponent* Comp = NewCityComponent(this, *Name, ClimbProfile);
		int32 Section = 0;
		for (int32 Mi = 0; Mi < TPCity::CM_Num; ++Mi)
		{
			const TPCity::FMeshBuf& M = B.Bufs[Ci * TPCity::CM_Num + Mi];
			if (M.IsEmpty()) continue;
			AddVisualSection(Comp, Section, M);
			if (Mats.IsValidIndex(Mi)) Comp->SetMaterial(Section, Mats[Mi]);
			++Section;
		}
		if (!B.Coll[Ci].IsEmpty()) AddCollisionSection(Comp, Section, B.Coll[Ci]);
		Comp->SetupAttachment(Root);
		Comp->RegisterComponent();
		Chunks.Add(Comp);
	}

	// 4. мягкие предметы (телеги с сеном) — отдельный компонент с тегом TPSoft
	{
		bool bAny = !B.SoftColl.IsEmpty();
		for (int32 Mi = 0; Mi < TPCity::CM_Num && !bAny; ++Mi) bAny = !B.SoftBufs[Mi].IsEmpty();
		if (bAny)
		{
			UProceduralMeshComponent* Comp = NewCityComponent(this, TEXT("CitySoft"), UCollisionProfile::BlockAll_ProfileName);
			Comp->ComponentTags.Add(TagSoft);
			int32 Section = 0;
			for (int32 Mi = 0; Mi < TPCity::CM_Num; ++Mi)
			{
				const TPCity::FMeshBuf& M = B.SoftBufs[Mi];
				if (M.IsEmpty()) continue;
				AddVisualSection(Comp, Section, M);
				if (Mats.IsValidIndex(Mi)) Comp->SetMaterial(Section, Mats[Mi]);
				++Section;
			}
			if (!B.SoftColl.IsEmpty()) AddCollisionSection(Comp, Section, B.SoftColl);
			Comp->SetupAttachment(Root);
			Comp->RegisterComponent();
			Chunks.Add(Comp);
		}
	}

	// 5. точки для игры (локально, см), затем в мир
	LocalWalk.Reset(); LocalLamps.Reset(); LocalHay.Reset();
	for (const TPCity::FV3& P : B.WalkPts) LocalWalk.Add(ToUE(P));
	for (const TPCity::FV3& P : B.LampPts) LocalLamps.Add(ToUE(P));
	for (const TPCity::FV3& P : B.HayPts) LocalHay.Add(ToUE(P));
	LocalStart = ToUE(B.PlayerStart);
	UpdateWorldPoints();

	NumTriangles = B.CountTriangles();
	NumBuildings = B.NumBuildings;
	GeneratedSeed = Seed;
	GeneratedTexSize = TexN;
	const double T3 = FPlatformTime::Seconds();
	UE_LOG(LogTeni, Log, TEXT("Город: %d домов, %d треугольников, %d компонентов, %d фонарей, %d точек прогулок; геометрия %.0f мс, материалы %.0f мс, загрузка в меши %.0f мс"),
		NumBuildings, NumTriangles, Chunks.Num(), LampPoints.Num(), WalkPoints.Num(), (T1 - T0) * 1000.0, (T2 - T1) * 1000.0, (T3 - T2) * 1000.0);
}

void ATPCityGenerator::UpdateWorldPoints()
{
	const FTransform& X = GetActorTransform();
	if (LocalWalk.Num() == 0 && LocalLamps.Num() == 0) return;   // ещё не строили — оставляем сохранённые в уровне точки
	WalkPoints.Reset(LocalWalk.Num());
	LampPoints.Reset(LocalLamps.Num());
	HayPoints.Reset(LocalHay.Num());
	for (const FVector& P : LocalWalk) WalkPoints.Add(X.TransformPosition(P));
	for (const FVector& P : LocalLamps) LampPoints.Add(X.TransformPosition(P));
	for (const FVector& P : LocalHay) HayPoints.Add(X.TransformPosition(P));
	PlayerStartPoint = X.TransformPosition(LocalStart);
}

FVector ATPCityGenerator::GetPlayerStart() const
{
	return PlayerStartPoint;
}

void ATPCityGenerator::SetLampGlow(float Level)
{
	using namespace TPCityGenImpl;
	FMatCache& C = Cache();
	UMaterialInstanceDynamic* MID = C.Lamp.Get();
	if (!MID) return;
	const float K = FMath::Clamp(Level, 0.f, 1.f);
	if (C.bLampEmissive)
	{
		MID->SetScalarParameterValue(TEXT("Intensity"), LampIntensity * K);
	}
	else
	{
		// запасной материал без свечения: просто светлее/темнее
		MID->SetVectorParameterValue(TEXT("Color"), FLinearColor::LerpUsingHSV(FLinearColor(.25f, .2f, .15f), LampColor, K));
	}
}
