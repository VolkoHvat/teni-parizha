// Макет CoreUObject UE 4.27: UObject, UClass, приведение типов, слабые и мягкие указатели, макросы отражения.
#pragma once
// TP_STUB_MODULE: CoreUObject
#include "CoreMinimal.h"

// ---------- макросы отражения ----------
// UPROPERTY/UFUNCTION превращаются в аннотации clang, их читает AST-проверка (tp_ast.py).
#define UPROPERTY(...) __attribute__((annotate("tp_uproperty:" #__VA_ARGS__)))
#define UFUNCTION(...) __attribute__((annotate("tp_ufunction:" #__VA_ARGS__)))
#define UCLASS(...)
#define USTRUCT(...)
#define UENUM(...)
#define UINTERFACE(...)
#define UMETA(...)
#define UPARAM(...)
#define UDELEGATE(...)
#define TP_BODY_COMBINE_INNER(A, B, C, D) A##B##C##D
#define TP_BODY_COMBINE(A, B, C, D) TP_BODY_COMBINE_INNER(A, B, C, D)
#define GENERATED_BODY(...) TP_BODY_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)
#define GENERATED_UCLASS_BODY(...) TP_BODY_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY_LEGACY)
#define GENERATED_USTRUCT_BODY(...) TP_BODY_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY)
#define GENERATED_IINTERFACE_BODY(...) TP_BODY_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY_LEGACY)
#define GENERATED_UINTERFACE_BODY(...) TP_BODY_COMBINE(CURRENT_FILE_ID, _, __LINE__, _GENERATED_BODY_LEGACY)
#define RESULT_PARAM Z_Param__Result
#define RESULT_DECL void* const RESULT_PARAM
#define NO_API

class UObject; class UClass; class UStruct; class UScriptStruct; class UEnum; class UFunction; class UPackage; class UWorld; class FProperty;
class FObjectInitializer; struct FPropertyChangedEvent; class FReferenceCollector;

enum EObjectFlags
{
	RF_NoFlags = 0x0, RF_Public = 0x1, RF_Standalone = 0x2, RF_MarkAsNative = 0x4, RF_Transactional = 0x8, RF_ClassDefaultObject = 0x10,
	RF_ArchetypeObject = 0x20, RF_Transient = 0x40, RF_MarkAsRootSet = 0x80, RF_TagGarbageTemp = 0x100, RF_NeedInitialization = 0x200,
	RF_NeedLoad = 0x400, RF_KeepForCooker = 0x800, RF_NeedPostLoad = 0x1000, RF_NeedPostLoadSubobjects = 0x2000, RF_NewerVersionExists = 0x4000,
	RF_BeginDestroyed = 0x8000, RF_FinishDestroyed = 0x10000, RF_BeingRegenerated = 0x20000, RF_DefaultSubObject = 0x40000, RF_WasLoaded = 0x80000,
	RF_TextExportTransient = 0x100000, RF_LoadCompleted = 0x200000, RF_InheritableComponentTemplate = 0x400000, RF_DuplicateTransient = 0x800000,
	RF_StrongRefOnFrame = 0x1000000, RF_NonPIEDuplicateTransient = 0x2000000, RF_Dynamic = 0x4000000, RF_WillBeLoaded = 0x8000000
};
inline EObjectFlags operator|(EObjectFlags A, EObjectFlags B) { return (EObjectFlags)((uint32)A | (uint32)B); }
inline EObjectFlags operator&(EObjectFlags A, EObjectFlags B) { return (EObjectFlags)((uint32)A & (uint32)B); }
enum ELoadFlags { LOAD_None = 0, LOAD_Async = 1, LOAD_NoWarn = 0x2, LOAD_EditorOnly = 0x4, LOAD_ResolvingDeferredExports = 0x8, LOAD_Verify = 0x10, LOAD_NoVerify = 0x80, LOAD_Quiet = 0x2000, LOAD_FindIfFail = 0x4000 };
enum class EInternalObjectFlags : int32 { None = 0, PendingKill = 1 << 29, RootSet = 1 << 30, Unreachable = 1 << 28 };
namespace ERenameFlags { enum { REN_None = 0, REN_ForceNoResetLoaders = 1, REN_Test = 2, REN_DoNotDirty = 4, REN_DontCreateRedirectors = 0x10, REN_NonTransactional = 0x20 }; }
struct FVTableHelper { FVTableHelper(); };
class UPackageMap;

class FObjectInitializer
{
public:
	FObjectInitializer();
	static FObjectInitializer& Get();
	template<class TReturnType> TReturnType* CreateDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const;
	template<class TReturnType, class TClassToConstructByDefault> TReturnType* CreateDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const;
	template<class TReturnType> TReturnType* CreateOptionalDefaultSubobject(UObject* Outer, FName SubobjectName, bool bTransient = false) const;
	template<class T> FObjectInitializer const& SetDefaultSubobjectClass(FName SubobjectName) const;
	FObjectInitializer const& DoNotCreateDefaultSubobject(FName SubobjectName) const;
	UObject* GetObj() const; UClass* GetClass() const; UObject* GetArchetype() const;
};

class UObjectBase
{
public:
	virtual ~UObjectBase();
	UClass* GetClass() const;
	UObject* GetOuter() const;
	FName GetFName() const;
	uint32 GetUniqueID() const;
	EObjectFlags GetFlags() const;
	void AtomicallySetFlags(EObjectFlags FlagsToAdd); void AtomicallyClearFlags(EObjectFlags FlagsToClear);
	bool IsValidLowLevel() const; bool IsValidLowLevelFast(bool bRecursive = true) const;
};
class UObjectBaseUtility : public UObjectBase
{
public:
	void SetFlags(EObjectFlags NewFlags); void ClearFlags(EObjectFlags NewFlags);
	bool HasAnyFlags(EObjectFlags FlagsToCheck) const; bool HasAllFlags(EObjectFlags FlagsToCheck) const;
	void MarkPendingKill(); void ClearPendingKill(); bool IsPendingKill() const; bool IsPendingKillOrUnreachable() const; bool IsUnreachable() const;
	void AddToRoot(); void RemoveFromRoot(); bool IsRooted() const;
	bool IsTemplate(EObjectFlags TemplateTypes = (EObjectFlags)(RF_ArchetypeObject | RF_ClassDefaultObject)) const;
	UObject* GetTypedOuter(UClass* Target) const;
	template<typename T> T* GetTypedOuter() const;
	bool IsIn(const UObject* SomeOuter) const;
	UPackage* GetOutermost() const; UPackage* GetPackage() const; UObject* GetOutermostObject() const;
	bool MarkPackageDirty() const;
	FString GetFullName(const UObject* StopOuter = nullptr) const;
	FString GetPathName(const UObject* StopOuter = nullptr) const;
	FString GetName() const; void GetName(FString& ResultString) const;
	bool IsA(const UClass* SomeBase) const;
	template<class T> bool IsA() const;
	bool IsDefaultSubobject() const;
	bool IsSelected() const;
};

class UObject : public UObjectBaseUtility
{
public:
	typedef UObject ThisClass; typedef UObject Super;
	static UClass* StaticClass();
	UObject(); UObject(const FObjectInitializer& ObjectInitializer); UObject(FVTableHelper& Helper);
	virtual ~UObject();
	template<class TReturnType> TReturnType* CreateDefaultSubobject(FName SubobjectName, bool bTransient = false);
	template<class TReturnType> TReturnType* CreateOptionalDefaultSubobject(FName SubobjectName, bool bTransient = false);
	template<class TReturnType> TReturnType* CreateEditorOnlyDefaultSubobject(FName SubobjectName, bool bTransient = false);
	UObject* CreateDefaultSubobject(FName SubobjectFName, UClass* ReturnType, UClass* ClassToCreateByDefault, bool bIsRequired, bool bIsTransient);
	virtual void PostInitProperties();
	virtual void PostLoad();
	virtual void PostCDOContruct();
	virtual void BeginDestroy();
	virtual void FinishDestroy();
	virtual bool IsReadyForFinishDestroy();
	virtual void Serialize(FArchive& Ar);
	virtual void PostDuplicate(bool bDuplicateForPIE);
	virtual void PreSave(const class ITargetPlatform* TargetPlatform);
	virtual bool Modify(bool bAlwaysMarkDirty = true);
	virtual bool Rename(const TCHAR* NewName = nullptr, UObject* NewOuter = nullptr, uint32 Flags = 0);
	virtual UWorld* GetWorld() const;
	virtual bool IsSupportedForNetworking() const;
	virtual bool IsNameStableForNetworking() const;
	virtual FString GetDesc();
	virtual bool IsEditorOnly() const;
	virtual bool NeedsLoadForClient() const; virtual bool NeedsLoadForServer() const;
	virtual void GetResourceSizeEx(struct FResourceSizeEx& CumulativeResourceSize);
	virtual bool CallRemoteFunction(UFunction* Function, void* Parms, struct FOutParmRec* OutParms, struct FFrame* Stack);
	virtual int32 GetFunctionCallspace(UFunction* Function, struct FFrame* Stack);
	virtual void ProcessEvent(UFunction* Function, void* Parms);
#if WITH_EDITOR
	virtual void PreEditChange(FProperty* PropertyAboutToChange);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
	virtual bool CanEditChange(const FProperty* InProperty) const;
	virtual void PostEditUndo();
#endif
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	void ConditionalBeginDestroy(); void ConditionalPostLoad();
	bool IsBasedOnArchetype(const UObject* const SomeObject) const;
	UFunction* FindFunction(FName InName) const; UFunction* FindFunctionChecked(FName InName) const;
	void SaveConfig(uint64 Flags = 0x4000, const TCHAR* Filename = nullptr); void LoadConfig(UClass* ConfigClass = nullptr, const TCHAR* Filename = nullptr, uint32 PropagationFlags = 0, FProperty* PropertyToLoad = nullptr);
	void ReloadConfig(UClass* ConfigClass = nullptr, const TCHAR* Filename = nullptr, uint32 PropagationFlags = 0, FProperty* PropertyToLoad = nullptr);
	UObject* GetArchetype() const;
	bool CallFunctionByNameWithArguments(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor, bool bForceCallWithNonExec = false);
};

class UField : public UObject { public: typedef UObject Super; static UClass* StaticClass(); UField* Next; };
class UStruct : public UField
{
public:
	typedef UField Super; static UClass* StaticClass();
	bool IsChildOf(const UStruct* SomeBase) const;
	template<class T> bool IsChildOf() const;
	UStruct* GetSuperStruct() const;
	int32 GetPropertiesSize() const;
	FProperty* FindPropertyByName(FName InName) const;
};
class UScriptStruct : public UStruct { public: typedef UStruct Super; static UClass* StaticClass(); };
class UFunction : public UStruct { public: typedef UStruct Super; static UClass* StaticClass(); uint32 FunctionFlags; uint8 NumParms; uint16 ParmsSize; };
class UClass : public UStruct
{
public:
	typedef UStruct Super; static UClass* StaticClass();
	UObject* GetDefaultObject(bool bCreateIfNeeded = true) const;
	template<class T> T* GetDefaultObject() const;
	UClass* GetSuperClass() const;
	bool ImplementsInterface(const UClass* SomeInterface) const;
	bool HasAnyClassFlags(uint32 FlagsToCheck) const;
	FString GetDescription() const;
	UClass* GetAuthoritativeClass();
	bool IsFunctionImplementedInScript(FName InFunctionName) const;
	UFunction* FindFunctionByName(FName InName, int32 IncludeSuper = 1) const;
	uint32 ClassFlags;
	UClass* ClassWithin;
};
enum EClassFlags { CLASS_None = 0, CLASS_Abstract = 0x1, CLASS_Deprecated = 0x02000000, CLASS_Native = 0x10, CLASS_CompiledFromBlueprint = 0x40000, CLASS_NewerVersionExists = 0x80000000u };
class UEnum : public UField
{
public:
	typedef UField Super; static UClass* StaticClass();
	FName GetNameByIndex(int32 Index) const; FName GetNameByValue(int64 InValue) const;
	FString GetNameStringByIndex(int32 InIndex) const; FString GetNameStringByValue(int64 InValue) const;
	FText GetDisplayNameTextByIndex(int32 InIndex) const; FText GetDisplayNameTextByValue(int64 InValue) const;
	int64 GetValueByIndex(int32 Index) const; int64 GetValueByName(FName InName) const; int64 GetValueByNameString(const FString& SearchString) const;
	int32 GetIndexByValue(int64 InValue) const; int32 GetIndexByName(const FName InName) const;
	int32 NumEnums() const; int64 GetMaxEnumValue() const; bool IsValidEnumValue(int64 InValue) const;
	template<typename T> static FString GetValueAsString(const TCHAR* EnumPath, const T EnumValue);
	template<typename T> static FString GetValueAsString(const T EnumValue);
	template<typename T> static FText GetDisplayValueAsText(const T EnumValue);
	template<typename T> static FName GetValueAsName(const T EnumValue);
};
template<typename T> UEnum* StaticEnum();
template<typename T> struct TBaseStructure { static UScriptStruct* Get(); };
class UInterface : public UObject { public: typedef UObject Super; static UClass* StaticClass(); UInterface(const FObjectInitializer& OI = FObjectInitializer::Get()); };
class IInterface { public: virtual ~IInterface(); };
class UPackage : public UObject { public: typedef UObject Super; static UClass* StaticClass(); void SetDirtyFlag(bool bIsDirty); bool IsDirty() const; static bool SavePackage(UPackage* InOuter, UObject* Base, EObjectFlags TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error = nullptr, void* Conform = nullptr, bool bForceByteSwapping = false, bool bWarnOfLongFilename = true, uint32 SaveFlags = 0, const class ITargetPlatform* TargetPlatform = nullptr, const FDateTime& FinalTimeStamp = FDateTime(), bool bSlowTask = true); };
UPackage* GetTransientPackage();
UPackage* CreatePackage(const TCHAR* PackageName);
UPackage* CreatePackage(UObject* InOuter, const TCHAR* PackageName);

// ---------- приведение типов ----------
template<class To, class From> To* Cast(From* Src) { static_assert(sizeof(From) > 0 && sizeof(To) > 0, "Cast: неполный тип — нужен #include"); return nullptr; }
template<class To, class From> const To* Cast(const From* Src) { static_assert(sizeof(From) > 0 && sizeof(To) > 0, "Cast: неполный тип — нужен #include"); return nullptr; }
template<class To, class From> To* ExactCast(From* Src) { static_assert(sizeof(From) > 0 && sizeof(To) > 0, "ExactCast: неполный тип"); return nullptr; }
template<class To, class From> To* CastChecked(From* Src) { static_assert(sizeof(From) > 0 && sizeof(To) > 0, "CastChecked: неполный тип"); return nullptr; }
template<class To, class From> const To* CastChecked(const From* Src) { static_assert(sizeof(From) > 0 && sizeof(To) > 0, "CastChecked: неполный тип"); return nullptr; }
template<class T> class TWeakObjectPtr; template<class T> class TSubclassOf; template<class T> class TSoftObjectPtr;
template<class To, class From> To* Cast(const TWeakObjectPtr<From>& Src);
bool IsValid(const UObject* Test);
FString GetNameSafe(const UObject* Object);
FString GetPathNameSafe(const UObject* Object);
FString GetFullNameSafe(const UObject* Object);

// ---------- создание и загрузка ----------
template<class T> T* NewObject(UObject* Outer = (UObject*)GetTransientPackage()) { static_assert(sizeof(T) > 0, "NewObject: неполный тип — нужен #include"); return nullptr; }
template<class T> T* NewObject(UObject* Outer, FName Name, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, struct FObjectInstancingGraph* InInstanceGraph = nullptr) { static_assert(sizeof(T) > 0, "NewObject: неполный тип"); return nullptr; }
template<class T> T* NewObject(UObject* Outer, const UClass* Class, FName Name = NAME_None, EObjectFlags Flags = RF_NoFlags, UObject* Template = nullptr, bool bCopyTransientsFromClassDefaults = false, struct FObjectInstancingGraph* InInstanceGraph = nullptr, UPackage* InExternalPackage = nullptr) { static_assert(sizeof(T) > 0, "NewObject: неполный тип"); return nullptr; }
UObject* StaticConstructObject_Internal(const struct FStaticConstructObjectParameters& Params);
template<class T> T* LoadObject(UObject* Outer, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr);
template<class T> T* FindObject(UObject* Outer, const TCHAR* Name, bool ExactClass = false);
template<class T> T* FindObjectChecked(UObject* Outer, const TCHAR* Name, bool ExactClass = false);
template<class T> T* FindObjectFast(UObject* Outer, FName InName, bool ExactClass = false, bool AnyPackage = false);
template<class T> UClass* LoadClass(UObject* Outer, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr);
UObject* StaticLoadObject(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr, bool bAllowObjectReconciliation = true, const struct FLinkerInstancingContext* InstancingContext = nullptr);
UClass* StaticLoadClass(UClass* BaseClass, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr);
UObject* StaticFindObject(UClass* Class, UObject* InOuter, const TCHAR* Name, bool ExactClass = false);
UObject* StaticDuplicateObject(UObject const* SourceObject, UObject* DestOuter, const FName DestName = NAME_None, EObjectFlags FlagMask = (EObjectFlags)0xffffffff, UClass* DestClass = nullptr);
template<class T> T* DuplicateObject(T const* SourceObject, UObject* Outer, const FName Name = NAME_None);
template<class T> const T* GetDefault();
template<class T> const T* GetDefault(UClass* Class);
template<class T> T* GetMutableDefault();
template<class T> T* GetMutableDefault(UClass* Class);
FName MakeUniqueObjectName(UObject* Outer, const UClass* Class, FName BaseName = NAME_None);
void CollectGarbage(EObjectFlags KeepFlags, bool bPerformFullPurge = true);
extern class UEngine* GEngine;
#define ANY_PACKAGE ((UPackage*)-1)

// ---------- TSubclassOf ----------
template<class TClass>
class TSubclassOf
{
public:
	TSubclassOf(); TSubclassOf(UClass* From); template<class U, class = typename std::enable_if<std::is_convertible<U*, TClass*>::value>::type> TSubclassOf(const TSubclassOf<U>& From);
	TSubclassOf& operator=(UClass* From);
	UClass* operator*() const; UClass* Get() const; UClass* operator->() const; operator UClass*() const;
	TClass* GetDefaultObject() const;
};

// ---------- слабые и мягкие указатели ----------
struct FWeakObjectPtr { FWeakObjectPtr(); FWeakObjectPtr(const UObject* Object); UObject* Get() const; bool IsValid() const; void Reset(); };
template<class T>
class TWeakObjectPtr
{
public:
	TWeakObjectPtr(); TWeakObjectPtr(std::nullptr_t);
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type> TWeakObjectPtr(U* Object);
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type> TWeakObjectPtr(const TWeakObjectPtr<U>& Other);
	TWeakObjectPtr(const TWeakObjectPtr& Other);
	TWeakObjectPtr& operator=(const TWeakObjectPtr& Other);
	template<class U, class = typename std::enable_if<std::is_convertible<U*, T*>::value>::type> TWeakObjectPtr& operator=(U* Object);
	TWeakObjectPtr& operator=(std::nullptr_t);
	T* Get(bool bEvenIfPendingKill) const; T* Get() const;
	T* operator->() const; T& operator*() const;
	bool IsValid(bool bEvenIfPendingKill, bool bThreadsafeTest = false) const; bool IsValid() const;
	bool IsStale(bool bIncludingIfPendingKill = true, bool bThreadsafeTest = false) const;
	void Reset();
	explicit operator bool() const;
	bool operator==(const TWeakObjectPtr& O) const; bool operator!=(const TWeakObjectPtr& O) const;
	template<class U> bool operator==(const U* O) const; template<class U> bool operator!=(const U* O) const;
};
template<class T> uint32 GetTypeHash(const TWeakObjectPtr<T>& P);
template<class T> TWeakObjectPtr<T> MakeWeakObjectPtr(T* Object);
struct FSoftObjectPath
{
	FSoftObjectPath(); FSoftObjectPath(const FString& Path); FSoftObjectPath(const TCHAR* Path); FSoftObjectPath(const UObject* InObject);
	FString ToString() const; FString GetAssetPathString() const; FName GetAssetPathName() const; FString GetAssetName() const; FString GetLongPackageName() const;
	bool IsValid() const; bool IsNull() const; UObject* TryLoad() const; UObject* ResolveObject() const; void Reset();
	bool operator==(const FSoftObjectPath& O) const;
};
struct FSoftClassPath : public FSoftObjectPath { FSoftClassPath(); FSoftClassPath(const FString& Path); FSoftClassPath(const TCHAR* Path); template<class T> UClass* TryLoadClass() const; UClass* ResolveClass() const; };
struct FSoftObjectPtr { FSoftObjectPtr(); UObject* Get() const; };
template<class T>
class TSoftObjectPtr
{
public:
	TSoftObjectPtr(); TSoftObjectPtr(std::nullptr_t); TSoftObjectPtr(const FSoftObjectPath& Path); template<class U> TSoftObjectPtr(U* Obj);
	T* Get() const; T* LoadSynchronous() const; bool IsValid() const; bool IsNull() const; bool IsPending() const; void Reset();
	FSoftObjectPath ToSoftObjectPath() const; FString ToString() const; FString GetAssetName() const; FString GetLongPackageName() const;
	T* operator->() const; explicit operator bool() const;
};
template<class T>
class TSoftClassPtr
{
public:
	TSoftClassPtr(); TSoftClassPtr(const FSoftObjectPath& Path); TSoftClassPtr(UClass* Cls);
	UClass* Get() const; UClass* LoadSynchronous() const; bool IsValid() const; bool IsNull() const; bool IsPending() const; void Reset();
	FSoftObjectPath ToSoftObjectPath() const; FString ToString() const;
};
template<class T> class TLazyObjectPtr { public: T* Get() const; bool IsValid() const; };
template<class InterfaceType>
class TScriptInterface
{
public:
	TScriptInterface(); TScriptInterface(std::nullptr_t); template<class U> TScriptInterface(U* Obj);
	UObject* GetObject() const; InterfaceType* GetInterface() const; void SetObject(UObject* O); void SetInterface(InterfaceType* I);
	InterfaceType* operator->() const; InterfaceType& operator*() const; explicit operator bool() const;
};

// ---------- обход объектов ----------
template<class T>
class TObjectIterator
{
public:
	explicit TObjectIterator(EObjectFlags AdditionalExclusionFlags = RF_ClassDefaultObject, bool bIncludeDerivedClasses = true, EInternalObjectFlags InInternalExclusionFlags = EInternalObjectFlags::None);
	explicit operator bool() const; bool operator!() const;
	void operator++(); T* operator*() const; T* operator->() const;
};
class FObjectIterator : public TObjectIterator<UObject> { public: explicit FObjectIterator(UClass* InClass = UObject::StaticClass(), bool bOnlyGCedObjects = false, EObjectFlags AdditionalExclusionFlags = RF_NoFlags, EInternalObjectFlags InInternalExclusionFlags = EInternalObjectFlags::None); };

class FReferenceCollector
{
public:
	virtual ~FReferenceCollector();
	template<class UObjectType> void AddReferencedObject(UObjectType*& Object, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr);
	template<class UObjectType> void AddReferencedObjects(TArray<UObjectType*>& ObjectArray, const UObject* ReferencingObject = nullptr, const FProperty* ReferencingProperty = nullptr);
};
class FGCObject
{
public:
	FGCObject(); virtual ~FGCObject();
	virtual void AddReferencedObjects(FReferenceCollector& Collector) = 0;
	virtual FString GetReferencerName() const;
};

#if WITH_EDITOR
namespace EPropertyChangeType { enum Type { Unspecified = 1, ArrayAdd = 2, ArrayRemove = 4, ArrayClear = 8, ValueSet = 16, Duplicate = 32, Interactive = 64, Redirected = 128 }; }
struct FPropertyChangedEvent
{
	FProperty* Property; FProperty* MemberProperty; uint32 ChangeType;
	FName GetPropertyName() const; FName GetMemberPropertyName() const;
};
#define GET_MEMBER_NAME_CHECKED(ClassName, MemberName) ((void)sizeof(((ClassName*)0)->MemberName), FName(TEXT(#MemberName)))
#endif
#ifndef GET_MEMBER_NAME_CHECKED
#define GET_MEMBER_NAME_CHECKED(ClassName, MemberName) ((void)sizeof(((ClassName*)0)->MemberName), FName(TEXT(#MemberName)))
#endif
#define GET_MEMBER_NAME_STRING_CHECKED(ClassName, MemberName) ((void)sizeof(((ClassName*)0)->MemberName), FString(TEXT(#MemberName)))
#define GET_FUNCTION_NAME_CHECKED(ClassName, FunctionName) ((void)sizeof(&ClassName::FunctionName), FName(TEXT(#FunctionName)))
class FProperty { public: FName GetFName() const; FString GetName() const; };
struct FFrame;

// ---------- генерируемое тело (используется в *.generated.h) ----------
#define TP_GENERATED_UCLASS(Cls, Base) \
	public: typedef Base Super; typedef Cls ThisClass; static UClass* StaticClass(); static const TCHAR* StaticPackage(); \
	static constexpr uint32 StaticClassCastFlags() { return 0; } \
	private: Cls(Cls&&) = delete; Cls(const Cls&) = delete; \
	public: Cls(FVTableHelper& Helper);
#define TP_GENERATED_UCLASS_DEFCTOR(Cls) public: Cls(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
#define TP_GENERATED_USTRUCT(Cls) friend struct Z_Construct_UScriptStruct_##Cls##_Statics; static class UScriptStruct* StaticStruct();
#define TP_GENERATED_USTRUCT_DERIVED(Cls, Base) TP_GENERATED_USTRUCT(Cls) typedef Base Super;
#define TP_GENERATED_IINTERFACE(Cls, UCls) \
	public: typedef UCls UClassType; typedef Cls ThisClass; virtual UObject* _getUObject() const { return nullptr; } \
	protected: virtual ~Cls() {} public:
