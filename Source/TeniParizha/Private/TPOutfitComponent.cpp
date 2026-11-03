// Тени Парижа — одежда на скелете: построение UStaticMesh частей, кэш, крепление к костям.
#include "TPOutfitComponent.h"
#include "TeniParizha.h"
#include "TPTypes.h"
#include "TPTexGen.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"
#include "ReferenceSkeleton.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "TextureResource.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/Package.h"

namespace TPOutfitUE
{
	using namespace TPAnim;

	/** кэш: меши частей по наряду, кости и скелету; материалы ткани и гладкого (живут, пока на них ссылаются компоненты) */
	struct FOutfitCache
	{
		TMap<FString, TWeakObjectPtr<UStaticMesh>> Meshes;
		TMap<FString, int32> Tris;
		TWeakObjectPtr<UMaterialInterface> Mats[TPOutfit::S_Count];
		bool bTextured = false;
	};
	static FOutfitCache& OutfitCache()
	{
		static FOutfitCache C;
		return C;
	}

	static UTexture2D* MakeOutfitTexture(const TArray<TArray<FColor>>& Mips, int32 N, bool bSRGB, TextureCompressionSettings Compression, const TCHAR* BaseName)
	{
		if (Mips.Num() == 0 || N <= 0 || Mips[0].Num() != N * N) return nullptr;
		const FName Name = MakeUniqueObjectName(GetTransientPackage(), UTexture2D::StaticClass(), FName(BaseName));
		UTexture2D* Tex = UTexture2D::CreateTransient(N, N, PF_B8G8R8A8, Name);
		if (!Tex) return nullptr;
		Tex->SRGB = bSRGB;
		Tex->CompressionSettings = Compression;
		Tex->NeverStream = true;
		Tex->AddressX = TA_Wrap;
		Tex->AddressY = TA_Wrap;
		FTexturePlatformData* PD = Tex->PlatformData;
		if (!PD || PD->Mips.Num() == 0) return nullptr;
		{
			FTexture2DMipMap& Mip0 = PD->Mips[0];
			void* Data = Mip0.BulkData.Lock(LOCK_READ_WRITE);
			FMemory::Memcpy(Data, Mips[0].GetData(), (SIZE_T)N * N * sizeof(FColor));
			Mip0.BulkData.Unlock();
		}
		int32 S = N;
		for (int32 L = 1; L < Mips.Num(); ++L)
		{
			S = FMath::Max(1, S / 2);
			if (Mips[L].Num() != S * S) break;
			FTexture2DMipMap* Mip = new FTexture2DMipMap();
			Mip->SizeX = S;
			Mip->SizeY = S;
			Mip->SizeZ = 1;
			PD->Mips.Add(Mip);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			void* Data = Mip->BulkData.Realloc((int64)S * S * (int64)sizeof(FColor));
			FMemory::Memcpy(Data, Mips[L].GetData(), (SIZE_T)S * S * sizeof(FColor));
			Mip->BulkData.Unlock();
		}
		Tex->UpdateResource();
		return Tex;
	}

	/**
	 * Материалы: M_TPBase (Albedo × VertexColor × Tint) с белой тканью TPTexGen (256², переплетение) и гладким (4², шероховатость 0.5).
	 * Без M_TPBase — простой материал движка (цвет вершин теряется; задаётся средний цвет части, см. BuildPartMesh).
	 */
	static void GetOutfitMaterials(UMaterialInterface* Out[TPOutfit::S_Count], bool& bTextured)
	{
		FOutfitCache& C = OutfitCache();
		bool bValid = true;
		for (int32 S = 0; S < TPOutfit::S_Count; ++S) { bValid = bValid && C.Mats[S].IsValid(); }
		if (bValid)
		{
			for (int32 S = 0; S < TPOutfit::S_Count; ++S) { Out[S] = C.Mats[S].Get(); }
			bTextured = C.bTextured;
			return;
		}
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TPPaths::BaseMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
		C.bTextured = Base != nullptr;
		if (!Base)
		{
			UE_LOG(LogTeni, Warning, TEXT("Одежда: нет материала %s — запустите Content/Python/tp_setup.py. Пока одежда одноцветная по частям."), TPPaths::BaseMaterial);
		}
		for (int32 S = 0; S < TPOutfit::S_Count; ++S)
		{
			UMaterialInterface* M = nullptr;
			if (Base)
			{
				TPTexGen::FPalette Pal;
				Pal.C0 = TPTexGen::FRGB(0.92f, 0.92f, 0.92f);
				Pal.C1 = TPTexGen::FRGB(1.f, 1.f, 1.f);
				Pal.Var = 0.04f;
				TPTexGen::FSet Set;
				const bool bCloth = S == TPOutfit::S_Cloth;
				TPTexGen::Generate(Set, bCloth ? TPTexGen::EType::Cloth : TPTexGen::EType::Flat, bCloth ? 256 : 4, Pal, 1789u + (uint32)S, 0.5f, 0.f, true);
				UTexture2D* Alb = MakeOutfitTexture(Set.Albedo, Set.N, true, TC_Default, bCloth ? TEXT("T_TPOutfitCloth_Albedo") : TEXT("T_TPOutfitSmooth_Albedo"));
				UTexture2D* Nrm = MakeOutfitTexture(Set.Normal, Set.N, false, TC_Normalmap, bCloth ? TEXT("T_TPOutfitCloth_Normal") : TEXT("T_TPOutfitSmooth_Normal"));
				UTexture2D* Orm = MakeOutfitTexture(Set.ORM, Set.N, false, TC_Masks, bCloth ? TEXT("T_TPOutfitCloth_ORM") : TEXT("T_TPOutfitSmooth_ORM"));
				UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, GetTransientPackage());
				if (MID)
				{
					if (Alb) MID->SetTextureParameterValue(TEXT("Albedo"), Alb);
					if (Nrm) MID->SetTextureParameterValue(TEXT("Normal"), Nrm);
					if (Orm) MID->SetTextureParameterValue(TEXT("ORM"), Orm);
					MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor::White);
					MID->SetScalarParameterValue(TEXT("UVScale"), bCloth ? 6.f : 1.f);   // UV одежды — в метрах: переплетение ~1.7 мм на тексель
					MID->SetScalarParameterValue(TEXT("GroundGrime"), 0.f);
				}
				M = MID;
			}
			if (!M) { M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"), nullptr, LOAD_NoWarn | LOAD_Quiet); }
			if (!M) { M = UMaterial::GetDefaultMaterial(MD_Surface); }
			C.Mats[S] = M;
			Out[S] = M;
		}
		bTextured = C.bTextured;
	}

	/** средний линейный цвет секции (для запасного материала без цвета вершин) */
	static FLinearColor AverageColor(const TPOutfit::FPiece& P, int32 Sec)
	{
		FLinearColor Sum(0.f, 0.f, 0.f, 0.f);
		int32 N = 0;
		const TArray<int32>& Ix = P.Idx[Sec];
		for (int32 K = 0; K < Ix.Num(); K += 3)
		{
			Sum += FLinearColor(P.Col[Ix[K]]);
			++N;
		}
		return N > 0 ? Sum / (float)N : FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
	}

	/**
	 * UStaticMesh части из буферов построителя; вершины переводятся из пространства персонажа в пространство меша (MeshInv).
	 * Цвет вершин: как у модулей города — кладём FLinearColor(FColor(линейные байты)), сборка вернёт те же байты (ToFColor(sRGB)),
	 * шейдер прочтёт их как линейный цвет.
	 */
	static UStaticMesh* BuildPartMesh(const TPOutfit::FPiece& P, const FQuat& MeshInv, const FString& Name, UMaterialInterface* const Mats[TPOutfit::S_Count])
	{
		FMeshDescription MD;
		FStaticMeshAttributes Attr(MD);
		Attr.Register();
		TVertexAttributesRef<FVector> Positions = Attr.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector> Normals = Attr.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector> Tangents = Attr.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> Signs = Attr.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector4> Colors = Attr.GetVertexInstanceColors();
		TVertexInstanceAttributesRef<FVector2D> UVs = Attr.GetVertexInstanceUVs();
		TPolygonGroupAttributesRef<FName> Slots = Attr.GetPolygonGroupMaterialSlotNames();

		const FName MeshName = MakeUniqueObjectName(GetTransientPackage(), UStaticMesh::StaticClass(), FName(*Name));
		UStaticMesh* SM = NewObject<UStaticMesh>(GetTransientPackage(), MeshName, RF_Transient);
		if (!SM) return nullptr;

		const int32 NV = P.Pos.Num();
		MD.ReserveNewVertices(NV);
		MD.ReserveNewVertexInstances(NV);
		TArray<FVertexInstanceID> Inst;
		Inst.Reserve(NV);
		for (int32 I = 0; I < NV; ++I)
		{
			const FVertexID Vx = MD.CreateVertex();
			Positions[Vx] = MeshInv.RotateVector(P.Pos[I]);
			const FVertexInstanceID VI = MD.CreateVertexInstance(Vx);
			const FVector N = MeshInv.RotateVector(P.Nrm[I]);
			// касательная — горизонтальная (для переплетения ткани направление не важно)
			FVector T = FVector::CrossProduct(MeshInv.RotateVector(FVector(0.f, 0.f, 1.f)), N);
			if (T.SizeSquared() < 1e-4f) { T = FVector::CrossProduct(MeshInv.RotateVector(FVector(1.f, 0.f, 0.f)), N); }
			T.Normalize();
			Normals[VI] = N;
			Tangents[VI] = T;
			Signs[VI] = 1.f;
			UVs.Set(VI, 0, P.UV[I]);
			const FColor Lin = FLinearColor(P.Col[I]).ToFColor(false);
			Colors[VI] = FVector4(FLinearColor(Lin));
			Inst.Add(VI);
		}
		TArray<FVertexInstanceID> Tri;
		for (int32 S = 0; S < TPOutfit::S_Count; ++S)
		{
			const TArray<int32>& Ix = P.Idx[S];
			if (Ix.Num() == 0) continue;
			const FName Slot(S == TPOutfit::S_Cloth ? TEXT("Cloth") : TEXT("Smooth"));
			const FPolygonGroupID Group = MD.CreatePolygonGroup();
			Slots[Group] = Slot;
			SM->StaticMaterials.Add(FStaticMaterial(Mats[S], Slot, Slot));
			for (int32 K = 0; K + 2 < Ix.Num(); K += 3)
			{
				Tri.Reset(3);
				Tri.Add(Inst[Ix[K]]);
				Tri.Add(Inst[Ix[K + 1]]);
				Tri.Add(Inst[Ix[K + 2]]);
				MD.CreatePolygon(Group, Tri);
			}
		}
		if (SM->StaticMaterials.Num() == 0) return nullptr;
		TArray<const FMeshDescription*> Descs;
		Descs.Add(&MD);
		SM->BuildFromMeshDescriptions(Descs);
		if (!SM->HasValidRenderData())
		{
			UE_LOG(LogTeni, Warning, TEXT("Одежда: часть %s не собралась в UStaticMesh"), *Name);
			return nullptr;
		}
		return SM;
	}
}

UTPOutfitComponent::UTPOutfitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTPOutfitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearOutfit();
	Super::EndPlay(EndPlayReason);
}

void UTPOutfitComponent::ClearOutfit()
{
	for (UStaticMeshComponent* C : Parts)
	{
		if (IsValid(C)) { C->DestroyComponent(); }
	}
	Parts.Reset();
	if (IsValid(Target) && bHideBody) { Target->SetVisibility(true); }
	Target = nullptr;
	NumTriangles = 0;
}

bool UTPOutfitComponent::ApplyHeroOutfit(USkeletalMeshComponent* Body)
{
	return ApplyOutfit(Body, TPOutfit::Hero());
}

bool UTPOutfitComponent::ApplyGuardOutfit(USkeletalMeshComponent* Body)
{
	return ApplyOutfit(Body, TPOutfit::Guard());
}

bool UTPOutfitComponent::ApplyCitizenOutfit(USkeletalMeshComponent* Body, bool bFemale, int32 Variant)
{
	return ApplyOutfit(Body, bFemale ? TPOutfit::CitizenFemale(Variant) : TPOutfit::CitizenMale(Variant));
}

bool UTPOutfitComponent::ApplyOutfit(USkeletalMeshComponent* Body, const TPOutfit::FDesc& Desc)
{
	using namespace TPAnim;
	ClearOutfit();
	AActor* Owner = GetOwner();
	if (!IsValid(Body) || !Body->SkeletalMesh || !Owner) return false;

	// покой костей в пространстве меша (FReferenceSkeleton, сложенный к корню)
	const FReferenceSkeleton& Ref = Body->SkeletalMesh->RefSkeleton;
	const TArray<FTransform>& Pose = Ref.GetRefBonePose();
	FVector CompPos[B_Count];
	FQuat CompRot[B_Count];
	FTransform CompXf[B_Count];
	for (int32 B = 0; B < B_Count; ++B)
	{
		const int32 Idx = Ref.FindBoneIndex(FName(BoneName(B)));
		if (Idx == INDEX_NONE || !Pose.IsValidIndex(Idx))
		{
			UE_LOG(LogTeni, Warning, TEXT("Одежда: у меша %s нет кости %s — наряд не надет"), *Body->SkeletalMesh->GetName(), BoneName(B));
			return false;
		}
		FTransform CS = Pose[Idx];
		for (int32 P = Ref.GetParentIndex(Idx); P != INDEX_NONE; P = Ref.GetParentIndex(P))
		{
			CS = CS * Pose[P];
		}
		CompXf[B] = CS;
		CompPos[B] = CS.GetTranslation();
		CompRot[B] = CS.GetRotation();
	}
	// пространство персонажа = поворот меша относительно актёра (у манекена — −90° по рысканью), как в UTPAnimInstance
	const FQuat MeshRot = Body->GetRelativeRotation().Quaternion();
	const FQuat MeshInv = MeshRot.Inverse();
	const FTPRig Rig = MakeRigFromMesh(CompPos, CompRot, MeshRot);
	if (Rig.LegLen < 10.f || Rig.HipH < 10.f) return false;

	TPOutfit::FMesh Built;
	bool bBuilt = false;
	UMaterialInterface* Mats[TPOutfit::S_Count] = { nullptr, nullptr };
	bool bTextured = false;
	TPOutfitUE::GetOutfitMaterials(Mats, bTextured);

	// ключ кэша: наряд + скелет (рост, высота таза в мм и поворот меша в градусах — меш части уже в осях меша)
	const uint32 RigSig = (uint32)FMath::RoundToInt(Rig.LegLen * 10.f) * 7919u + (uint32)FMath::RoundToInt(Rig.RestPos[B_Pelvis].Z * 10.f)
		+ (uint32)(FMath::RoundToInt(Body->GetRelativeRotation().Yaw) + 360) * 104729u;
	const uint32 DescHash = TPOutfit::HashDesc(Desc);
	TPOutfitUE::FOutfitCache& Cache = TPOutfitUE::OutfitCache();

	Target = Body;
	for (int32 B = 0; B < B_Count; ++B)
	{
		const FString Key = FString::Printf(TEXT("%08x_%08x_%d"), DescHash, RigSig, B);
		UStaticMesh* SM = nullptr;
		if (const TWeakObjectPtr<UStaticMesh>* Found = Cache.Meshes.Find(Key)) { SM = Found->Get(); }
		int32 PieceTris = 0;
		if (!SM)
		{
			if (!bBuilt)
			{
				TPOutfit::Build(Rig, Desc, Built);
				bBuilt = true;
			}
			const TPOutfit::FPiece& P = Built.Pieces[B];
			if (P.IsEmpty()) continue;
			SM = TPOutfitUE::BuildPartMesh(P, MeshInv, FString::Printf(TEXT("SM_TPOutfit_%s"), BoneName(B)), Mats);
			if (!SM) continue;
			Cache.Meshes.Add(Key, SM);
			Cache.Tris.Add(Key, P.NumTris());
		}
		if (const int32* T = Cache.Tris.Find(Key)) { PieceTris = *T; }

		const FName CompName = MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), FName(*FString::Printf(TEXT("Outfit_%s"), BoneName(B))));
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner, CompName, RF_Transient);
		if (!C) continue;
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(SM);
		C->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false);
		C->SetCastShadow(bCastShadow);
		C->SetReceivesDecals(false);
		// без M_TPBase цвет вершин не виден — красим часть её средним цветом (параметр Color у BasicShapeMaterial)
		if (!bTextured)
		{
			if (!bBuilt)
			{
				TPOutfit::Build(Rig, Desc, Built);
				bBuilt = true;
			}
			const TPOutfit::FPiece& P = Built.Pieces[B];
			for (int32 S = 0, Slot = 0; S < TPOutfit::S_Count; ++S)
			{
				if (P.Idx[S].Num() == 0) continue;
				if (UMaterialInstanceDynamic* MID = C->CreateAndSetMaterialInstanceDynamic(Slot))
				{
					MID->SetVectorParameterValue(TEXT("Color"), TPOutfitUE::AverageColor(P, S));
				}
				++Slot;
			}
		}
		// часть построена в позе покоя; относительно кости — обратное к покою кости: в покое стоит на месте, дальше следует за костью
		const FName Bone(BoneName(B));
		C->SetupAttachment(Body, Bone);
		C->SetRelativeTransform(CompXf[B].Inverse());   // вершины меша части уже в осях меша (MeshInv)
		C->RegisterComponent();
		Owner->AddInstanceComponent(C);
		Parts.Add(C);
		NumTriangles += PieceTris;
	}

	if (Parts.Num() == 0)
	{
		Target = nullptr;
		return false;
	}
	if (bHideBody)
	{
		// манекен не рисуется, но кости обновляются — к ним прикреплены части одежды
		Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Body->SetVisibility(false, false);
	}
	UE_LOG(LogTeni, Log, TEXT("Одежда: %s — %d частей, %d треугольников"), *Owner->GetName(), Parts.Num(), NumTriangles);
	return true;
}
