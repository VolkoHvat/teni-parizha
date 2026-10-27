// Макет ядра UE 4.27 (модуль Core) для проверки синтаксиса. Только объявления: код не линкуется.
// Сигнатуры — как в UE 4.27; всё, чего нет в 4.27 (UE5), намеренно отсутствует.
#pragma once
// TP_STUB_MODULE: Core

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <type_traits>
#include <utility>
#include <initializer_list>
#include <new>

// ---------- базовые типы ----------
typedef uint8_t uint8; typedef uint16_t uint16; typedef uint32_t uint32; typedef unsigned long long uint64;
typedef int8_t int8; typedef int16_t int16; typedef int32_t int32; typedef long long int64;
typedef wchar_t TCHAR; typedef wchar_t WIDECHAR; typedef char ANSICHAR; typedef char UTF8CHAR; typedef uint16 UCS2CHAR;
typedef uint64 SIZE_T; typedef int64 SSIZE_T; typedef uint64 UPTRINT; typedef int64 PTRINT;
typedef int32 TYPE_OF_NULL;

#define TEXT(x) L##x
#define FORCEINLINE inline
#define FORCENOINLINE
#define RESTRICT
#define CONSTEXPR constexpr
#define UE_NODISCARD
#define DLLEXPORT
#define DLLIMPORT
#define PURE_VIRTUAL(func, ...) { __VA_ARGS__ }
#define UE_DEPRECATED(Version, Msg) [[deprecated(Msg)]]
#define DEPRECATED(Version, Msg) [[deprecated(Msg)]]
#define PRAGMA_DISABLE_DEPRECATION_WARNINGS
#define PRAGMA_ENABLE_DEPRECATION_WARNINGS
#define PRAGMA_DISABLE_OPTIMIZATION
#define PRAGMA_ENABLE_OPTIMIZATION
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#define UE_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define ARRAY_COUNT(a) UE_ARRAY_COUNT(a)
#define STRUCT_OFFSET(s, m) offsetof(s, m)
#define ENUM_CLASS_FLAGS(E) \
	inline E& operator|=(E& L, E R) { return L = (E)((__underlying_type(E))L | (__underlying_type(E))R); } \
	inline E& operator&=(E& L, E R) { return L = (E)((__underlying_type(E))L & (__underlying_type(E))R); } \
	inline constexpr E operator|(E L, E R) { return (E)((__underlying_type(E))L | (__underlying_type(E))R); } \
	inline constexpr E operator&(E L, E R) { return (E)((__underlying_type(E))L & (__underlying_type(E))R); } \
	inline constexpr bool operator!(E V) { return !(__underlying_type(E))V; } \
	inline constexpr E operator~(E V) { return (E)~(__underlying_type(E))V; }

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 1
#endif
#define PLATFORM_LINUX 0
#define PLATFORM_MAC 0
#define PLATFORM_ANDROID 0
#define PLATFORM_IOS 0
#define PLATFORM_DESKTOP 1
#define PLATFORM_LITTLE_ENDIAN 1
#ifndef WITH_EDITOR
#define WITH_EDITOR 1
#endif
#ifndef WITH_EDITORONLY_DATA
#define WITH_EDITORONLY_DATA WITH_EDITOR
#endif
#ifndef UE_BUILD_SHIPPING
#define UE_BUILD_SHIPPING 0
#endif
#define UE_BUILD_DEBUG 0
#define UE_BUILD_DEVELOPMENT 1
#define UE_BUILD_TEST 0
#define UE_SERVER 0
#define UE_GAME (!WITH_EDITOR)
#define UE_EDITOR WITH_EDITOR
#define DO_CHECK 1
#define NO_LOGGING 0
#define ENGINE_MAJOR_VERSION 4
#define ENGINE_MINOR_VERSION 27
#define ENGINE_PATCH_VERSION 2
#define WITH_ENGINE 1
#define WITH_PHYSX 1
#define WITH_CHAOS 0
#define CPP 1
#define STATS 1

// ---------- проверки ----------
#define check(expr) ((void)(expr))
#define checkf(expr, fmt, ...) ((void)(expr))
#define checkSlow(expr) ((void)(expr))
#define checkNoEntry() ((void)0)
#define checkNoReentry() ((void)0)
#define verify(expr) ((void)(expr))
#define verifyf(expr, fmt, ...) ((void)(expr))
#define ensure(expr) (!!(expr))
#define ensureMsgf(expr, fmt, ...) (!!(expr))
#define ensureAlways(expr) (!!(expr))
#define ensureAlwaysMsgf(expr, fmt, ...) (!!(expr))
#define unimplemented() ((void)0)
#define static_assert_tp static_assert

// ---------- шаблонные утилиты ----------
template<typename T> T& TPStubRef(); // только объявление: для проверки типов в теле шаблонов
template<typename T> FORCEINLINE typename std::remove_reference<T>::type&& MoveTemp(T&& Obj) { return static_cast<typename std::remove_reference<T>::type&&>(Obj); }
template<typename T> FORCEINLINE T CopyTemp(const T& Val) { return Val; }
template<typename T> FORCEINLINE T&& Forward(typename std::remove_reference<T>::type& Obj) { return static_cast<T&&>(Obj); }
template<typename T> FORCEINLINE void Swap(T& A, T& B) { T Tmp = MoveTemp(A); A = MoveTemp(B); B = MoveTemp(Tmp); }
template<typename T> FORCEINLINE void Exchange(T& A, T& B) { Swap(A, B); }
template<typename T, typename U> FORCEINLINE T ReturnOther(U&&);
template<typename T> struct TRemoveReference { typedef typename std::remove_reference<T>::type Type; };
template<typename T> struct TRemovePointer { typedef typename std::remove_pointer<T>::type Type; };
template<typename T> struct TDecay { typedef typename std::decay<T>::type Type; };
template<typename T> struct TIsPointer { enum { Value = std::is_pointer<T>::value }; };
template<typename T> struct TIsEnum { enum { Value = std::is_enum<T>::value }; };
template<typename T> struct TIsArithmetic { enum { Value = std::is_arithmetic<T>::value }; };
template<typename A, typename B> struct TIsSame { enum { Value = std::is_same<A, B>::value }; };
template<typename From, typename To> struct TPointerIsConvertibleFromTo { enum { Value = std::is_convertible<From*, To*>::value }; };
template<typename T> struct TIsTriviallyDestructible { enum { Value = std::is_trivially_destructible<T>::value }; };
template<typename T> struct TIsPODType { enum { Value = std::is_pod<T>::value }; };
template<typename T> struct TNumericLimits;
template<> struct TNumericLimits<float> { static constexpr float Min() { return 1.175494351e-38F; } static constexpr float Max() { return 3.402823466e+38F; } static constexpr float Lowest() { return -Max(); } };
template<> struct TNumericLimits<double> { static constexpr double Min() { return 2.2250738585072014e-308; } static constexpr double Max() { return 1.7976931348623158e+308; } static constexpr double Lowest() { return -Max(); } };
template<> struct TNumericLimits<int32> { static constexpr int32 Min() { return (int32)0x80000000; } static constexpr int32 Max() { return 0x7fffffff; } static constexpr int32 Lowest() { return Min(); } };
template<> struct TNumericLimits<uint32> { static constexpr uint32 Min() { return 0; } static constexpr uint32 Max() { return 0xffffffffu; } static constexpr uint32 Lowest() { return 0; } };
template<> struct TNumericLimits<uint8> { static constexpr uint8 Min() { return 0; } static constexpr uint8 Max() { return 0xff; } static constexpr uint8 Lowest() { return 0; } };
template<> struct TNumericLimits<int64> { static constexpr int64 Min() { return (-9223372036854775807LL - 1); } static constexpr int64 Max() { return 9223372036854775807LL; } static constexpr int64 Lowest() { return Min(); } };
template<> struct TNumericLimits<uint16> { static constexpr uint16 Min() { return 0; } static constexpr uint16 Max() { return 0xffff; } static constexpr uint16 Lowest() { return 0; } };
template<> struct TNumericLimits<int16> { static constexpr int16 Min() { return -32768; } static constexpr int16 Max() { return 32767; } static constexpr int16 Lowest() { return Min(); } };

#define MIN_uint8 ((uint8)0x00)
#define MIN_uint16 ((uint16)0x0000)
#define MIN_uint32 ((uint32)0x00000000)
#define MIN_int8 ((int8)-128)
#define MIN_int16 ((int16)-32768)
#define MIN_int32 ((int32)0x80000000)
#define MIN_int64 ((int64)(-9223372036854775807LL - 1))
#define MAX_uint8 ((uint8)0xff)
#define MAX_uint16 ((uint16)0xffff)
#define MAX_uint32 ((uint32)0xffffffff)
#define MAX_uint64 ((uint64)0xffffffffffffffffULL)
#define MAX_int8 ((int8)0x7f)
#define MAX_int16 ((int16)0x7fff)
#define MAX_int32 ((int32)0x7fffffff)
#define MAX_int64 ((int64)0x7fffffffffffffffLL)
#define MIN_flt (1.175494351e-38F)
#define MAX_flt (3.402823466e+38F)
#define MIN_dbl (2.2250738585072014e-308)
#define MAX_dbl (1.7976931348623158e+308)

enum { INDEX_NONE = -1 };
enum EForceInit { ForceInit, ForceInitToZero };
enum ENoInit { NoInit };
enum EInPlace { InPlace };
namespace ESearchCase { enum Type { CaseSensitive, IgnoreCase }; }
namespace ESearchDir { enum Type { FromStart, FromEnd }; }
namespace EAllowShrinking { }

// Проверка аргументов printf-подобных функций (как TIsValidVariadicFunctionArg в 4.27)
template<typename T> struct TTPValidVariadicArg { enum { Value = std::is_arithmetic<T>::value || std::is_pointer<T>::value || std::is_enum<T>::value || std::is_same<T, std::nullptr_t>::value }; };
template<typename... Ts> struct TTPAllValidVariadic;
template<> struct TTPAllValidVariadic<> { enum { Value = 1 }; };
template<typename T, typename... Ts> struct TTPAllValidVariadic<T, Ts...> { enum { Value = TTPValidVariadicArg<typename std::decay<T>::type>::Value && TTPAllValidVariadic<Ts...>::Value }; };
template<typename T> struct TTPIsTCharArray { enum { Value = 0 }; };
template<unsigned N> struct TTPIsTCharArray<TCHAR[N]> { enum { Value = 1 }; };
template<unsigned N> struct TTPIsTCharArray<const TCHAR[N]> { enum { Value = 1 }; };
template<typename T> struct TTPIsTCharArray<T&> : TTPIsTCharArray<T> {};

// ---------- память ----------
struct FMemory
{
	static void* Malloc(SIZE_T Count, uint32 Alignment = 0);
	static void* Realloc(void* Original, SIZE_T Count, uint32 Alignment = 0);
	static void Free(void* Original);
	static void* Memcpy(void* Dest, const void* Src, SIZE_T Count);
	static void* Memmove(void* Dest, const void* Src, SIZE_T Count);
	static void* Memset(void* Dest, uint8 Char, SIZE_T Count);
	static void* Memzero(void* Dest, SIZE_T Count);
	static int32 Memcmp(const void* Buf1, const void* Buf2, SIZE_T Count);
	template<class T> static void Memzero(T& Src) { Memzero(&Src, sizeof(T)); }
	template<class T> static void Memset(T& Src, uint8 ValueToSet) { Memset(&Src, ValueToSet, sizeof(T)); }
};

// ---------- сортировка (Sort у TArray указателей разыменовывает элементы, как в UE) ----------
template<typename T, typename P> struct TTPDerefPred
{
	const P& Pred;
	bool operator()(const T& A, const T& B) const { return Pred(A, B); }
};
template<typename T, typename P> struct TTPDerefPred<T*, P>
{
	const P& Pred;
	bool operator()(T* A, T* B) const { return Pred(*A, *B); }
};
struct FTPLessDefault { template<typename T> bool operator()(const T& A, const T& B) const { return A < B; } };
template<typename T> struct TLess { bool operator()(const T& A, const T& B) const { return A < B; } };
template<typename T> struct TGreater { bool operator()(const T& A, const T& B) const { return B < A; } };

class FDefaultAllocator {};
class FDefaultSetAllocator {};
template<int N> class TInlineAllocator {};
template<int N> class TFixedAllocator {};

template<typename T> struct TArrayView;

// ---------- TArray ----------
template<typename InElementType, typename Allocator = FDefaultAllocator>
class TArray
{
public:
	typedef InElementType ElementType;
	typedef int32 SizeType;
	TArray();
	TArray(std::initializer_list<InElementType> Init);
	TArray(const InElementType* Ptr, int32 Count);
	TArray(const TArray& Other);
	TArray(TArray&& Other);
	template<typename OtherAlloc> explicit TArray(const TArray<InElementType, OtherAlloc>& Other);
	~TArray();
	TArray& operator=(const TArray& Other);
	TArray& operator=(TArray&& Other);
	TArray& operator=(std::initializer_list<InElementType> Init);

	int32 Num() const;
	int32 Max() const;
	int32 GetSlack() const;
	SIZE_T GetAllocatedSize() const;
	static constexpr SIZE_T GetTypeSize() { return sizeof(InElementType); }
	bool IsValidIndex(int32 Index) const;
	InElementType* GetData();
	const InElementType* GetData() const;
	InElementType& operator[](int32 Index);
	const InElementType& operator[](int32 Index) const;
	InElementType& Last(int32 IndexFromTheEnd = 0);
	const InElementType& Last(int32 IndexFromTheEnd = 0) const;
	InElementType& Top();
	const InElementType& Top() const;
	InElementType Pop(bool bAllowShrinking = true);
	void Push(const InElementType& Item);
	void Push(InElementType&& Item);

	int32 Add(const InElementType& Item);
	int32 Add(InElementType&& Item);
	InElementType& Add_GetRef(const InElementType& Item);
	InElementType& Add_GetRef(InElementType&& Item);
	int32 AddUnique(const InElementType& Item);
	int32 AddUnique(InElementType&& Item);
	template<typename... ArgsType> int32 Emplace(ArgsType&&... Args);
	template<typename... ArgsType> InElementType& Emplace_GetRef(ArgsType&&... Args);
	template<typename... ArgsType> void EmplaceAt(int32 Index, ArgsType&&... Args);
	int32 AddDefaulted();
	int32 AddDefaulted(int32 Count);
	InElementType& AddDefaulted_GetRef();
	int32 AddZeroed(int32 Count = 1);
	InElementType& AddZeroed_GetRef();
	int32 AddUninitialized(int32 Count = 1);
	int32 Insert(const InElementType& Item, int32 Index);
	int32 Insert(InElementType&& Item, int32 Index);
	int32 Insert(const TArray& Items, int32 InIndex);
	void InsertDefaulted(int32 Index, int32 Count = 1);
	void InsertZeroed(int32 Index, int32 Count = 1);
	template<typename OtherAlloc> void Append(const TArray<InElementType, OtherAlloc>& Source);
	template<typename OtherAlloc> void Append(TArray<InElementType, OtherAlloc>&& Source);
	void Append(const InElementType* Ptr, int32 Count);
	void Append(std::initializer_list<InElementType> InitList);
	TArray& operator+=(const TArray& Other);
	TArray& operator+=(TArray&& Other);

	int32 Remove(const InElementType& Item);
	int32 RemoveSingle(const InElementType& Item);
	int32 RemoveSwap(const InElementType& Item, bool bAllowShrinking = true);
	int32 RemoveSingleSwap(const InElementType& Item, bool bAllowShrinking = true);
	void RemoveAt(int32 Index);
	void RemoveAt(int32 Index, int32 Count, bool bAllowShrinking = true);
	template<typename CountType> void RemoveAt(int32 Index, CountType Count, bool bAllowShrinking = true) { static_assert(!std::is_same<CountType, bool>::value, "TArray::RemoveAt: второй параметр — Count, а не bAllowShrinking"); }
	void RemoveAtSwap(int32 Index);
	void RemoveAtSwap(int32 Index, int32 Count, bool bAllowShrinking = true);
	template<typename Predicate> int32 RemoveAll(const Predicate& Pred) { int32 N = 0; if (Pred(TPStubRef<const InElementType>())) ++N; return N; }
	template<typename Predicate> void RemoveAllSwap(const Predicate& Pred, bool bAllowShrinking = true) { (void)Pred(TPStubRef<const InElementType>()); }
	void Empty(int32 Slack = 0);
	void Reset(int32 NewSize = 0);
	void Shrink();
	void Reserve(int32 Number);
	void SetNum(int32 NewNum, bool bAllowShrinking = true);
	void SetNumZeroed(int32 NewNum, bool bAllowShrinking = true);
	void SetNumUninitialized(int32 NewNum, bool bAllowShrinking = true);
	void SetNumUnsafeInternal(int32 NewNum);
	void Init(const InElementType& Element, int32 Number);
	void Swap(int32 A, int32 B);
	void SwapMemory(int32 A, int32 B);

	template<typename ComparisonType> bool Contains(const ComparisonType& Item) const { (void)(TPStubRef<const InElementType>() == Item); return false; }
	template<typename Predicate> bool ContainsByPredicate(Predicate Pred) const { return Pred(TPStubRef<const InElementType>()); }
	int32 Find(const InElementType& Item) const;
	bool Find(const InElementType& Item, int32& Index) const;
	int32 FindLast(const InElementType& Item) const;
	bool FindLast(const InElementType& Item, int32& Index) const;
	template<typename KeyType> int32 IndexOfByKey(const KeyType& Key) const { (void)(TPStubRef<const InElementType>() == Key); return INDEX_NONE; }
	template<typename Predicate> int32 IndexOfByPredicate(Predicate Pred) const { (void)Pred(TPStubRef<const InElementType>()); return INDEX_NONE; }
	template<typename KeyType> InElementType* FindByKey(const KeyType& Key) { (void)(TPStubRef<const InElementType>() == Key); return nullptr; }
	template<typename KeyType> const InElementType* FindByKey(const KeyType& Key) const { (void)(TPStubRef<const InElementType>() == Key); return nullptr; }
	template<typename Predicate> InElementType* FindByPredicate(Predicate Pred) { (void)Pred(TPStubRef<InElementType>()); return nullptr; }
	template<typename Predicate> const InElementType* FindByPredicate(Predicate Pred) const { (void)Pred(TPStubRef<const InElementType>()); return nullptr; }
	template<typename Predicate> int32 FindLastByPredicate(Predicate Pred) const { (void)Pred(TPStubRef<const InElementType>()); return INDEX_NONE; }
	template<typename Predicate> TArray FilterByPredicate(Predicate Pred) const { (void)Pred(TPStubRef<const InElementType>()); return TArray(); }

	void Sort() { Sort(FTPLessDefault()); }
	template<typename PredicateType> void Sort(const PredicateType& Pred) { TTPDerefPred<InElementType, PredicateType> D{Pred}; (void)D(TPStubRef<InElementType>(), TPStubRef<InElementType>()); }
	void StableSort() { Sort(); }
	template<typename PredicateType> void StableSort(const PredicateType& Pred) { Sort(Pred); }
	template<typename PredicateType> void HeapSort(const PredicateType& Pred) { Sort(Pred); }
	template<typename PredicateType> void Heapify(const PredicateType& Pred) { Sort(Pred); }
	template<typename PredicateType> void HeapPush(const InElementType& Item, const PredicateType& Pred) { Sort(Pred); }
	template<typename PredicateType> void HeapPop(InElementType& OutItem, const PredicateType& Pred, bool bAllowShrinking = true) { Sort(Pred); }
	template<typename PredicateType> void HeapPopDiscard(const PredicateType& Pred, bool bAllowShrinking = true) { Sort(Pred); }
	const InElementType& HeapTop() const;

	bool operator==(const TArray& Other) const;
	bool operator!=(const TArray& Other) const;

	InElementType* begin();
	InElementType* end();
	const InElementType* begin() const;
	const InElementType* end() const;

	class TIterator { public: InElementType& operator*() const; InElementType* operator->() const; TIterator& operator++(); explicit operator bool() const; int32 GetIndex() const; void RemoveCurrent(); void Reset(); };
	class TConstIterator { public: const InElementType& operator*() const; const InElementType* operator->() const; TConstIterator& operator++(); explicit operator bool() const; int32 GetIndex() const; };
	TIterator CreateIterator();
	TConstIterator CreateConstIterator() const;
};
template<typename T, typename A> SIZE_T GetTypeHash(const TArray<T, A>&);

template<typename T, int N> using TTPInlineArray = TArray<T, TInlineAllocator<N>>;

// ---------- TArrayView ----------
template<typename T>
struct TArrayView
{
	TArrayView();
	template<typename A> TArrayView(const TArray<typename std::remove_const<T>::type, A>& Arr);
	TArrayView(T* Data, int32 Num);
	TArrayView(std::initializer_list<typename std::remove_const<T>::type> List);
	int32 Num() const;
	T& operator[](int32 I) const;
	T* GetData() const;
	T* begin() const; T* end() const;
	bool IsValidIndex(int32 I) const;
};
template<typename T> TArrayView<T> MakeArrayView(T* Data, int32 Num);
template<typename T, typename A> TArrayView<T> MakeArrayView(TArray<T, A>& Arr);
template<typename T, typename A> TArrayView<const T> MakeArrayView(const TArray<T, A>& Arr);

// ---------- TTuple / TPair ----------
template<typename KeyType, typename ValueType>
struct TPair
{
	KeyType Key; ValueType Value;
	TPair() = default;
	TPair(const KeyType& K, const ValueType& V) : Key(K), Value(V) {}
	template<typename K2, typename V2> TPair(K2&& K, V2&& V) : Key(Forward<K2>(K)), Value(Forward<V2>(V)) {}
	bool operator==(const TPair& O) const;
	bool operator<(const TPair& O) const;
	template<int I> auto& Get();
};
template<typename... Types> struct TTuple;
template<typename A> struct TTuple<A> { A V0; TTuple(); TTuple(const A& a); template<int I> A& Get(); template<int I> const A& Get() const; };
template<typename A, typename B> struct TTuple<A, B> { A Key; B Value; TTuple(); TTuple(const A& a, const B& b); template<int I> auto& Get(); template<int I> const auto& Get() const; };
template<typename A, typename B, typename C> struct TTuple<A, B, C> { TTuple(); TTuple(const A&, const B&, const C&); template<int I> auto& Get(); template<int I> const auto& Get() const; };
template<typename A, typename B, typename C, typename D> struct TTuple<A, B, C, D> { TTuple(); TTuple(const A&, const B&, const C&, const D&); template<int I> auto& Get(); template<int I> const auto& Get() const; };
template<typename... Types> TTuple<typename std::decay<Types>::type...> MakeTuple(Types&&... Args);
template<typename... Types> TTuple<Types&...> Tie(Types&... Args);
template<typename K, typename V> TPair<typename std::decay<K>::type, typename std::decay<V>::type> MakeTPair(K&&, V&&);

// ---------- хеши ----------
FORCEINLINE uint32 GetTypeHash(const uint8 A) { return A; }
FORCEINLINE uint32 GetTypeHash(const int8 A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(const uint16 A) { return A; }
FORCEINLINE uint32 GetTypeHash(const int16 A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(const int32 A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(const uint32 A) { return A; }
FORCEINLINE uint32 GetTypeHash(const uint64 A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(const int64 A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(float A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(double A) { return (uint32)A; }
FORCEINLINE uint32 GetTypeHash(const void* A) { return (uint32)(UPTRINT)A; }
FORCEINLINE uint32 GetTypeHash(const TCHAR* A) { return 0; }
template<typename E> FORCEINLINE typename std::enable_if<std::is_enum<E>::value, uint32>::type GetTypeHash(E A) { return (uint32)A; }
template<typename K, typename V> uint32 GetTypeHash(const TPair<K, V>& P) { return GetTypeHash(P.Key) ^ GetTypeHash(P.Value); }
FORCEINLINE uint32 HashCombine(uint32 A, uint32 C) { return A ^ C; }
FORCEINLINE uint32 PointerHash(const void* Key, uint32 C = 0) { return 0; }

// ---------- TSet / TMap ----------
template<typename T> struct TTPKeyArg { typedef const T& Type; };
template<typename T> struct TTPKeyArg<T*> { typedef const T* Type; };
template<typename InElementType, typename KeyFuncs = void, typename Allocator = FDefaultSetAllocator>
class TSet
{
public:
	typedef InElementType ElementType;
	TSet(); TSet(std::initializer_list<InElementType> L); TSet(const TArray<InElementType>& A);
	struct FSetElementId { bool IsValidId() const; int32 AsInteger() const; };
	FSetElementId Add(const InElementType& Item, bool* bIsAlreadyInSetPtr = nullptr) { (void)GetTypeHash(Item); (void)(Item == Item); return FSetElementId(); }
	FSetElementId Add(InElementType&& Item, bool* bIsAlreadyInSetPtr = nullptr) { (void)GetTypeHash(Item); return FSetElementId(); }
	template<typename... A> FSetElementId Emplace(A&&... Args);
	void Append(const TArray<InElementType>& InElements);
	void Append(const TSet& Other);
	int32 Remove(typename TTPKeyArg<InElementType>::Type Key);
	bool Contains(typename TTPKeyArg<InElementType>::Type Key) const { (void)GetTypeHash(Key); return false; }
	const InElementType* Find(typename TTPKeyArg<InElementType>::Type Key) const;
	InElementType* Find(typename TTPKeyArg<InElementType>::Type Key);
	int32 Num() const;
	void Empty(int32 ExpectedNumElements = 0);
	void Reset();
	void Reserve(int32 Number);
	TArray<InElementType> Array() const;
	TSet Intersect(const TSet& Other) const;
	TSet Union(const TSet& Other) const;
	TSet Difference(const TSet& Other) const;
	bool Includes(const TSet& Other) const;
	template<typename Predicate> void Sort(const Predicate& Pred);
	InElementType* begin(); InElementType* end();
	const InElementType* begin() const; const InElementType* end() const;
	class TIterator { public: InElementType& operator*() const; InElementType* operator->() const; TIterator& operator++(); explicit operator bool() const; void RemoveCurrent(); };
	class TConstIterator { public: const InElementType& operator*() const; const InElementType* operator->() const; TConstIterator& operator++(); explicit operator bool() const; };
	TIterator CreateIterator(); TConstIterator CreateConstIterator() const;
};

template<typename KeyType, typename ValueType, typename SetAllocator = FDefaultSetAllocator, typename KeyFuncs = void>
class TMap
{
public:
	typedef TPair<KeyType, ValueType> ElementType;
	TMap(); TMap(std::initializer_list<TPair<KeyType, ValueType>> L);
	ValueType& Add(const KeyType& InKey, const ValueType& InValue) { (void)GetTypeHash(InKey); return TPStubRef<ValueType>(); }
	ValueType& Add(const KeyType& InKey, ValueType&& InValue) { (void)GetTypeHash(InKey); return TPStubRef<ValueType>(); }
	ValueType& Add(KeyType&& InKey, const ValueType& InValue) { (void)GetTypeHash(InKey); return TPStubRef<ValueType>(); }
	ValueType& Add(KeyType&& InKey, ValueType&& InValue) { (void)GetTypeHash(InKey); return TPStubRef<ValueType>(); }
	ValueType& Add(const KeyType& InKey) { (void)GetTypeHash(InKey); return TPStubRef<ValueType>(); }
	ValueType& Add(const TPair<KeyType, ValueType>& InPair);
	template<typename K, typename V> ValueType& Emplace(K&& InKey, V&& InValue);
	template<typename K> ValueType& Emplace(K&& InKey);
	ValueType& FindOrAdd(const KeyType& Key) { (void)GetTypeHash(Key); return TPStubRef<ValueType>(); }
	ValueType& FindOrAdd(KeyType&& Key) { (void)GetTypeHash(Key); return TPStubRef<ValueType>(); }
	ValueType& FindOrAdd(const KeyType& Key, const ValueType& Value);
	ValueType* Find(typename TTPKeyArg<KeyType>::Type Key) { (void)GetTypeHash(Key); return nullptr; }
	const ValueType* Find(typename TTPKeyArg<KeyType>::Type Key) const { (void)GetTypeHash(Key); return nullptr; }
	ValueType FindRef(typename TTPKeyArg<KeyType>::Type Key) const { (void)GetTypeHash(Key); return TPStubRef<ValueType>(); }
	ValueType& FindChecked(typename TTPKeyArg<KeyType>::Type Key);
	const ValueType& FindChecked(typename TTPKeyArg<KeyType>::Type Key) const;
	const KeyType* FindKey(const ValueType& Value) const;
	ValueType FindAndRemoveChecked(const KeyType& Key);
	bool RemoveAndCopyValue(const KeyType& Key, ValueType& OutRemovedValue);
	bool Contains(typename TTPKeyArg<KeyType>::Type Key) const { (void)GetTypeHash(Key); return false; }
	int32 Remove(typename TTPKeyArg<KeyType>::Type Key);
	int32 Num() const;
	void Empty(int32 ExpectedNumElements = 0);
	void Reset();
	void Reserve(int32 Number);
	void Shrink();
	void Compact();
	ValueType& operator[](const KeyType& Key);
	const ValueType& operator[](const KeyType& Key) const;
	int32 GetKeys(TArray<KeyType>& OutKeys) const;
	int32 GetKeys(TSet<KeyType>& OutKeys) const;
	template<typename A> void GenerateKeyArray(TArray<KeyType, A>& OutArray) const;
	template<typename A> void GenerateValueArray(TArray<ValueType, A>& OutArray) const;
	template<typename Predicate> void KeySort(const Predicate& Pred) { (void)Pred(TPStubRef<const KeyType>(), TPStubRef<const KeyType>()); }
	template<typename Predicate> void ValueSort(const Predicate& Pred) { (void)Pred(TPStubRef<const ValueType>(), TPStubRef<const ValueType>()); }
	template<typename Predicate> TMap FilterByPredicate(Predicate Pred) const;
	void Append(const TMap& Other);
	TPair<KeyType, ValueType>* begin(); TPair<KeyType, ValueType>* end();
	const TPair<KeyType, ValueType>* begin() const; const TPair<KeyType, ValueType>* end() const;
	class TIterator { public: TPair<KeyType, ValueType>& operator*() const; TPair<KeyType, ValueType>* operator->() const; TIterator& operator++(); explicit operator bool() const; const KeyType& Key() const; ValueType& Value() const; void RemoveCurrent(); };
	class TConstIterator { public: const TPair<KeyType, ValueType>& operator*() const; const TPair<KeyType, ValueType>* operator->() const; TConstIterator& operator++(); explicit operator bool() const; const KeyType& Key() const; const ValueType& Value() const; };
	TIterator CreateIterator(); TConstIterator CreateConstIterator() const;
};
template<typename KeyType, typename ValueType>
class TMultiMap : public TMap<KeyType, ValueType>
{
public:
	void MultiFind(const KeyType& Key, TArray<ValueType>& OutValues, bool bMaintainOrder = false) const;
	ValueType& AddUnique(const KeyType& Key, const ValueType& Value);
	int32 RemoveSingle(const KeyType& Key, const ValueType& Value);
	int32 Num(const KeyType& Key) const;
	int32 Num() const;
};
template<typename T> class TQueue { public: bool Enqueue(const T& Item); bool Dequeue(T& OutItem); bool IsEmpty() const; bool Peek(T& OutItem) const; void Empty(); };

// ---------- TOptional ----------
template<typename T>
struct TOptional
{
	TOptional(); TOptional(const T& V); TOptional(T&& V);
	bool IsSet() const; explicit operator bool() const;
	T& GetValue(); const T& GetValue() const;
	const T& Get(const T& Default) const;
	T* GetPtrOrNull(); void Reset(); template<typename... A> void Emplace(A&&... Args);
	T* operator->(); const T* operator->() const; T& operator*(); const T& operator*() const;
};

// ---------- указатели ----------
template<typename T> struct TDefaultDelete { void operator()(T* P) const { delete P; } };
template<typename T, typename Deleter = TDefaultDelete<T>>
class TUniquePtr
{
public:
	TUniquePtr(); TUniquePtr(std::nullptr_t); explicit TUniquePtr(T* P);
	TUniquePtr(TUniquePtr&& O); template<typename U> TUniquePtr(TUniquePtr<U>&& O);
	TUniquePtr(const TUniquePtr&) = delete; TUniquePtr& operator=(const TUniquePtr&) = delete;
	TUniquePtr& operator=(TUniquePtr&& O); TUniquePtr& operator=(std::nullptr_t);
	~TUniquePtr();
	T* Get() const; T* Release(); void Reset(T* P = nullptr); bool IsValid() const; explicit operator bool() const;
	T* operator->() const; T& operator*() const;
};
template<typename T, typename... A> TUniquePtr<T> MakeUnique(A&&... Args) { return TUniquePtr<T>(new T(Forward<A>(Args)...)); }

namespace ESPMode { enum Type { NotThreadSafe = 0, Fast = 0, ThreadSafe = 1 }; }
template<class T, ESPMode::Type M = ESPMode::Fast> class TSharedRef;
template<class T, ESPMode::Type M = ESPMode::Fast> class TWeakPtr;
template<class T, ESPMode::Type M = ESPMode::Fast>
class TSharedPtr
{
public:
	TSharedPtr(); TSharedPtr(std::nullptr_t); template<typename U> explicit TSharedPtr(U* P);
	template<typename U> TSharedPtr(const TSharedPtr<U, M>& O); template<typename U> TSharedPtr(const TSharedRef<U, M>& O);
	T* Get() const; bool IsValid() const; explicit operator bool() const; void Reset();
	T* operator->() const; T& operator*() const; TSharedRef<T, M> ToSharedRef() const;
	bool operator==(const TSharedPtr& O) const; bool operator!=(const TSharedPtr& O) const;
};
template<class T, ESPMode::Type M>
class TSharedRef
{
public:
	template<typename U> TSharedRef(const TSharedRef<U, M>& O);
	T* operator->() const; T& operator*() const; T& Get() const; TSharedPtr<T, M> ToSharedPtr() const;
};
template<class T, ESPMode::Type M>
class TWeakPtr
{
public:
	TWeakPtr(); template<typename U> TWeakPtr(const TSharedPtr<U, M>& O); template<typename U> TWeakPtr(const TSharedRef<U, M>& O);
	TSharedPtr<T, M> Pin() const; bool IsValid() const; void Reset();
};
template<typename T, ESPMode::Type M = ESPMode::Fast, typename... A> TSharedRef<T, M> MakeShared(A&&... Args);
template<typename T, ESPMode::Type M = ESPMode::Fast, typename... A> TSharedRef<T, M> MakeShareable(T* P);
template<class T> class TSharedFromThis { public: TSharedRef<T> AsShared(); };

// ---------- TFunction ----------
template<typename FuncType> class TFunction;
template<typename Ret, typename... Params>
class TFunction<Ret(Params...)>
{
public:
	TFunction(); TFunction(std::nullptr_t);
	template<typename F, typename = typename std::enable_if<!std::is_same<typename std::decay<F>::type, TFunction>::value>::type>
	TFunction(F&& Func) { static_assert(std::is_convertible<decltype(Func(std::declval<Params>()...)), Ret>::value || std::is_void<Ret>::value, "TFunction: несовместимая сигнатура"); }
	TFunction(const TFunction&); TFunction(TFunction&&); TFunction& operator=(const TFunction&); TFunction& operator=(TFunction&&);
	Ret operator()(Params... Args) const;
	explicit operator bool() const; bool IsSet() const; void Reset();
};
template<typename FuncType> class TFunctionRef;
template<typename Ret, typename... Params>
class TFunctionRef<Ret(Params...)>
{
public:
	template<typename F, typename = typename std::enable_if<!std::is_same<typename std::decay<F>::type, TFunctionRef>::value>::type>
	TFunctionRef(F&& Func) { (void)sizeof(decltype(Func(std::declval<Params>()...))*); }
	Ret operator()(Params... Args) const;
};
template<typename FuncType> using TUniqueFunction = TFunction<FuncType>;

// ---------- строки ----------
class FString;
class FName;
class FText;

struct FCString
{
	static int32 Atoi(const TCHAR* S);
	static int64 Atoi64(const TCHAR* S);
	static float Atof(const TCHAR* S);
	static double Atod(const TCHAR* S);
	static bool ToBool(const TCHAR* S);
	static int32 Strlen(const TCHAR* S);
	static int32 Strcmp(const TCHAR* A, const TCHAR* B);
	static int32 Stricmp(const TCHAR* A, const TCHAR* B);
	static int32 Strncmp(const TCHAR* A, const TCHAR* B, SIZE_T Count);
	static TCHAR* Strcpy(TCHAR* Dest, SIZE_T DestCount, const TCHAR* Src);
	static const TCHAR* Strstr(const TCHAR* A, const TCHAR* B);
	static const TCHAR* Strchr(const TCHAR* A, TCHAR C);
	static bool IsNumeric(const TCHAR* S);
	template<typename... T> static int32 Sprintf(TCHAR* Dest, const TCHAR* Fmt, T... Args);
	template<typename... T> static int32 Snprintf(TCHAR* Dest, int32 DestSize, const TCHAR* Fmt, T... Args);
};
struct FCStringAnsi { static int32 Strlen(const ANSICHAR* S); static int32 Atoi(const ANSICHAR* S); static float Atof(const ANSICHAR* S); static int32 Strcmp(const ANSICHAR* A, const ANSICHAR* B); };
struct FChar
{
	static bool IsDigit(TCHAR C); static bool IsAlpha(TCHAR C); static bool IsAlnum(TCHAR C); static bool IsWhitespace(TCHAR C);
	static bool IsUpper(TCHAR C); static bool IsLower(TCHAR C); static TCHAR ToUpper(TCHAR C); static TCHAR ToLower(TCHAR C);
	static bool IsPunct(TCHAR C); static int32 ConvertCharDigitToInt(TCHAR C); static bool IsLinebreak(TCHAR C);
};

class FString
{
public:
	typedef TCHAR ElementType;
	FString(); FString(const FString&); FString(FString&&);
	FString(const TCHAR* Str);
	FString(const ANSICHAR* Str);
	FString(int32 InCount, const TCHAR* InSrc);
	explicit FString(const TArray<TCHAR>& Arr);
	FString& operator=(const FString&); FString& operator=(FString&&); FString& operator=(const TCHAR*);
	const TCHAR* operator*() const;
	TCHAR& operator[](int32 Index); const TCHAR& operator[](int32 Index) const;
	TArray<TCHAR>& GetCharArray(); const TArray<TCHAR>& GetCharArray() const;
	int32 Len() const; bool IsEmpty() const; void Empty(int32 Slack = 0); void Reset(int32 NewReservedSize = 0); void Reserve(int32 CharacterCount); void Shrink();
	bool IsValidIndex(int32 Index) const;
	FString& operator+=(const FString& Str); FString& operator+=(const TCHAR* Str); FString& operator+=(TCHAR Ch);
	FString& Append(const FString& Text); FString& Append(const TCHAR* Text, int32 Count); FString& Append(const TCHAR* Text); FString& AppendChar(TCHAR InChar);
	FString& AppendChars(const TCHAR* Array, int32 Count);
	void InsertAt(int32 Index, TCHAR Character); void InsertAt(int32 Index, const FString& Characters);
	void RemoveAt(int32 Index, int32 Count = 1, bool bAllowShrinking = true);
	bool RemoveFromStart(const FString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase);
	bool RemoveFromEnd(const FString& InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase);
	FString Left(int32 Count) const; FString LeftChop(int32 Count) const; FString Right(int32 Count) const; FString RightChop(int32 Count) const;
	FString Mid(int32 Start, int32 Count = MAX_int32) const;
	void LeftInline(int32 Count, bool bAllowShrinking = true); void LeftChopInline(int32 Count, bool bAllowShrinking = true);
	void RightInline(int32 Count, bool bAllowShrinking = true); void RightChopInline(int32 Count, bool bAllowShrinking = true);
	void MidInline(int32 Start, int32 Count = MAX_int32, bool bAllowShrinking = true);
	int32 Find(const TCHAR* SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart, int32 StartPosition = INDEX_NONE) const;
	int32 Find(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart, int32 StartPosition = INDEX_NONE) const;
	bool Contains(const TCHAR* SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart) const;
	bool Contains(const FString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart) const;
	bool FindChar(TCHAR InChar, int32& Index) const; bool FindLastChar(TCHAR InChar, int32& Index) const;
	bool StartsWith(const TCHAR* InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
	bool StartsWith(const FString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
	bool EndsWith(const TCHAR* InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
	bool EndsWith(const FString& InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
	bool Equals(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;
	int32 Compare(const FString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSensitive) const;
	bool Split(const FString& InS, FString* LeftS, FString* RightS, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart) const;
	int32 ParseIntoArray(TArray<FString>& OutArray, const TCHAR* pchDelim, bool InCullEmpty = true) const;
	int32 ParseIntoArrayLines(TArray<FString>& OutArray, bool InCullEmpty = true) const;
	int32 ParseIntoArrayWS(TArray<FString>& OutArray, const TCHAR* pchExtraDelim = nullptr, bool InCullEmpty = true) const;
	FString Replace(const TCHAR* From, const TCHAR* To, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;
	int32 ReplaceInline(const TCHAR* SearchText, const TCHAR* ReplacementText, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase);
	FString ToUpper() const; FString ToLower() const; void ToUpperInline(); void ToLowerInline();
	FString TrimStart() const; FString TrimEnd() const; FString TrimStartAndEnd() const;
	void TrimStartInline(); void TrimEndInline(); void TrimStartAndEndInline();
	FString TrimQuotes(bool* bQuotesRemoved = nullptr) const;
	FString Reverse() const; void ReverseString();
	FString LeftPad(int32 ChCount) const; FString RightPad(int32 ChCount) const;
	bool IsNumeric() const;
	FString ConvertTabsToSpaces(const int32 InSpacesPerTab);
	static FString Chr(TCHAR Ch);
	static FString ChrN(int32 NumCharacters, TCHAR Char);
	static FString FromInt(int32 Num);
	static FString SanitizeFloat(double InFloat, const int32 InMinFractionalDigits = 1);
	static FString FormatAsNumber(int32 InNumber);
	template<typename FmtType, typename... Types>
	static FString Printf(const FmtType& Fmt, Types... Args)
	{
		static_assert(TTPIsTCharArray<FmtType>::Value, "Строка формата FString::Printf должна быть массивом TCHAR (TEXT(\"...\"))");
		static_assert(TTPAllValidVariadic<Types...>::Value, "В FString::Printf переданы недопустимые аргументы (FString/FName/FText нужно передавать как *Str)");
		return FString();
	}
	static FString Format(const TCHAR* InFormatString, const TArray<struct FStringFormatArg>& InOrderedArguments);
	template<typename T> static FString Join(const T& Array, const TCHAR* Separator);
	static FString Join(const TArray<FString>& Array, const TCHAR* Separator);
	friend FString operator+(const FString& L, const FString& R);
	friend FString operator+(const FString& L, const TCHAR* R);
	friend FString operator+(const TCHAR* L, const FString& R);
	friend FString operator+(FString&& L, const FString& R);
	friend FString operator+(FString&& L, const TCHAR* R);
	friend bool operator==(const FString& L, const FString& R);
	friend bool operator==(const FString& L, const TCHAR* R);
	friend bool operator==(const TCHAR* L, const FString& R);
	friend bool operator!=(const FString& L, const FString& R);
	friend bool operator!=(const FString& L, const TCHAR* R);
	friend bool operator!=(const TCHAR* L, const FString& R);
	friend bool operator<(const FString& L, const FString& R);
	friend bool operator>(const FString& L, const FString& R);
	friend FString operator/(const FString& L, const FString& R);
	friend FString operator/(const FString& L, const TCHAR* R);
	friend FString& operator/=(FString& L, const TCHAR* R);
	friend FString& operator/=(FString& L, const FString& R);
	TCHAR* begin(); TCHAR* end(); const TCHAR* begin() const; const TCHAR* end() const;
};
uint32 GetTypeHash(const FString& S);
struct FStringFormatArg { FStringFormatArg(int32); FStringFormatArg(uint32); FStringFormatArg(int64); FStringFormatArg(float); FStringFormatArg(double); FStringFormatArg(const FString&); FStringFormatArg(const TCHAR*); };
typedef TArray<FStringFormatArg> FStringFormatOrderedArguments;
inline FString LexToString(int32 V); inline FString LexToString(float V); inline FString LexToString(double V); inline FString LexToString(bool V); inline FString LexToString(const FString& V); inline FString LexToString(int64 V); inline FString LexToString(uint8 V); inline FString LexToString(uint32 V);
void LexFromString(int32& Out, const TCHAR* S); void LexFromString(float& Out, const TCHAR* S); void LexFromString(bool& Out, const TCHAR* S);
struct FStringOutputDevice;
template<int N> class TStringBuilder { public: TStringBuilder& operator<<(const TCHAR*); TStringBuilder& operator<<(const FString&); TStringBuilder& Append(const TCHAR*); int32 Len() const; const TCHAR* ToString() const; const TCHAR* operator*() const; };
// конвертеры
class FTCHARToUTF8 { public: explicit FTCHARToUTF8(const TCHAR* S); const ANSICHAR* Get() const; int32 Length() const; };
class FUTF8ToTCHAR { public: explicit FUTF8ToTCHAR(const ANSICHAR* S); explicit FUTF8ToTCHAR(const ANSICHAR* S, int32 Len); const TCHAR* Get() const; int32 Length() const; };
#define TCHAR_TO_UTF8(s) (const ANSICHAR*)FTCHARToUTF8((const TCHAR*)s).Get()
#define UTF8_TO_TCHAR(s) (TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)s).Get()
#define TCHAR_TO_ANSI(s) (ANSICHAR*)FTCHARToUTF8((const TCHAR*)s).Get()
#define ANSI_TO_TCHAR(s) (TCHAR*)FUTF8ToTCHAR((const ANSICHAR*)s).Get()

// ---------- FName ----------
enum EName { NAME_None = 0 };
enum EFindName { FNAME_Find, FNAME_Add };
class FName
{
public:
	FName(); FName(EName N); FName(const TCHAR* Name, EFindName FindType = FNAME_Add); FName(const ANSICHAR* Name, EFindName FindType = FNAME_Add);
	explicit FName(const FString& Name, EFindName FindType = FNAME_Add);
	FName(const TCHAR* Name, int32 InNumber); FName(FName Other, int32 InNumber);
	FString ToString() const; void ToString(FString& Out) const; void AppendString(FString& Out) const;
	bool IsNone() const; bool IsValid() const; int32 GetNumber() const; void SetNumber(int32 N); FName GetComparisonIndex() const;
	bool IsEqual(const FName& Other) const; bool LexicalLess(const FName& Other) const; bool FastLess(const FName& Other) const;
	int32 Compare(const FName& Other) const;
	bool operator==(const FName& O) const; bool operator!=(const FName& O) const; bool operator==(EName O) const; bool operator!=(EName O) const;
	bool operator==(const TCHAR* O) const; bool operator!=(const TCHAR* O) const;
};
uint32 GetTypeHash(const FName& N);
struct FNameLexicalLess { bool operator()(const FName& A, const FName& B) const; };
struct FNameFastLess { bool operator()(const FName& A, const FName& B) const; };

// ---------- FText ----------
class FFormatArgumentValue;
class FText
{
public:
	FText(); FText(const FText&); FText(FText&&); FText& operator=(const FText&); FText& operator=(FText&&);
	static FText FromString(const FString& String); static FText FromString(FString&& String);
	static FText FromName(const FName& Val); static FText AsCultureInvariant(const FString& String);
	static const FText& GetEmpty();
	static FText AsNumber(int32 Val, const struct FNumberFormattingOptions* const Options = nullptr);
	static FText AsNumber(float Val, const struct FNumberFormattingOptions* const Options = nullptr);
	static FText AsNumber(double Val, const struct FNumberFormattingOptions* const Options = nullptr);
	static FText AsNumber(int64 Val, const struct FNumberFormattingOptions* const Options = nullptr);
	static FText AsPercent(float Val, const struct FNumberFormattingOptions* const Options = nullptr);
	template<typename... A> static FText Format(class FTextFormat Fmt, A&&... Args);
	static FText FormatOrdered(class FTextFormat Fmt, ...);
	static FText Join(const FText& Delimiter, const TArray<FText>& Args);
	const FString& ToString() const; bool IsEmpty() const; bool IsEmptyOrWhitespace() const; bool EqualTo(const FText& Other) const;
	FText ToUpper() const; FText ToLower() const;
};
class FTextFormat { public: FTextFormat(); FTextFormat(const FText& InText); static FTextFormat FromString(const FString& InString); };
class FFormatArgumentValue { public: FFormatArgumentValue(int32); FFormatArgumentValue(float); FFormatArgumentValue(double); FFormatArgumentValue(const FText&); FFormatArgumentValue(int64); FFormatArgumentValue(uint32); };
typedef TMap<FString, FFormatArgumentValue> FFormatNamedArguments;
typedef TArray<FFormatArgumentValue> FFormatOrderedArguments;
struct FNumberFormattingOptions { FNumberFormattingOptions(); FNumberFormattingOptions& SetMinimumFractionalDigits(int32); FNumberFormattingOptions& SetMaximumFractionalDigits(int32); FNumberFormattingOptions& SetUseGrouping(bool); int32 MinimumFractionalDigits; int32 MaximumFractionalDigits; bool UseGrouping; };
#define NSLOCTEXT(InNamespace, InKey, InTextLiteral) FText::FromString(FString(TEXT(InTextLiteral)))
#define LOCTEXT(InKey, InTextLiteral) (sizeof(LOCTEXT_NAMESPACE), FText::FromString(FString(TEXT(InTextLiteral))))
#define INVTEXT(InTextLiteral) FText::FromString(FString(TEXT(InTextLiteral)))

// ---------- журнал ----------
namespace ELogVerbosity { enum Type : uint8 { NoLogging = 0, Fatal, Error, Warning, Display, Log, Verbose, VeryVerbose, All = VeryVerbose, NumVerbosity, VerbosityMask = 0xf, SetColor = 0x40, BreakOnLog = 0x80 }; }
struct FLogCategoryBase { FName GetCategoryName() const; bool IsSuppressed(ELogVerbosity::Type V) const; };
#define DECLARE_LOG_CATEGORY_EXTERN(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
	extern struct FLogCategory##CategoryName : public FLogCategoryBase { enum { Def = ELogVerbosity::DefaultVerbosity, Comp = ELogVerbosity::CompileTimeVerbosity }; } CategoryName;
#define DEFINE_LOG_CATEGORY(CategoryName) FLogCategory##CategoryName CategoryName;
#define DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity) \
	static struct FLogCategory##CategoryName : public FLogCategoryBase { enum { Def = ELogVerbosity::DefaultVerbosity, Comp = ELogVerbosity::CompileTimeVerbosity }; } CategoryName;
#define DECLARE_LOG_CATEGORY_CLASS(CategoryName, DefaultVerbosity, CompileTimeVerbosity) DEFINE_LOG_CATEGORY_STATIC(CategoryName, DefaultVerbosity, CompileTimeVerbosity)
DECLARE_LOG_CATEGORY_EXTERN(LogTemp, Log, All);
template<typename FmtType, typename... Types> inline void TPStubCheckLogFormat(const FmtType& Fmt, Types... Args)
{
	static_assert(TTPIsTCharArray<FmtType>::Value, "Строка формата UE_LOG должна быть массивом TCHAR (TEXT(\"...\"))");
	static_assert(TTPAllValidVariadic<Types...>::Value, "В UE_LOG переданы недопустимые аргументы (FString/FName/FText нужно передавать как *Str)");
}
#define UE_LOG(CategoryName, Verbosity, Format, ...) do { (void)static_cast<const FLogCategoryBase&>(CategoryName); (void)ELogVerbosity::Verbosity; TPStubCheckLogFormat(Format, ##__VA_ARGS__); } while (0)
#define UE_CLOG(Condition, CategoryName, Verbosity, Format, ...) do { if (Condition) UE_LOG(CategoryName, Verbosity, Format, ##__VA_ARGS__); } while (0)

// ---------- статистика ----------
struct TStatId { TStatId(); bool IsValidStat() const; };
#define DECLARE_CYCLE_STAT(CounterName, StatId, GroupId) struct FStat_##StatId { static TStatId GetStatId(); }
#define DECLARE_STATS_GROUP(GroupDesc, GroupId, GroupCat)
#define SCOPE_CYCLE_COUNTER(Stat) ((void)0)
#define QUICK_SCOPE_CYCLE_COUNTER(Stat) ((void)0)
#define RETURN_QUICK_DECLARE_CYCLE_STAT(StatId, GroupId) return TStatId();
#define TRACE_CPUPROFILER_EVENT_SCOPE(Name) ((void)0)
#define CSV_SCOPED_TIMING_STAT(Cat, Stat) ((void)0)
#define GET_STATID(Stat) TStatId()
#define QUICK_USE_CYCLE_STAT(StatId, GroupId) TStatId()

// ---------- глобальные переменные ядра (CoreGlobals.h) ----------
extern uint64 GFrameCounter; extern uint32 GFrameNumber; extern bool GIsRunning; extern float GNearClippingPlane; extern bool GIsEditor; extern class FConfigCacheIni* GConfig; extern TCHAR GInternalProjectName[64];
bool IsEngineExitRequested();

// ---------- платформа / время ----------
struct FPlatformTime
{
	static double Seconds(); static uint32 Cycles(); static uint64 Cycles64(); static double GetSecondsPerCycle(); static float ToMilliseconds(uint32 Cycles); static double ToSeconds(uint32 Cycles);
	static void SystemTime(int32& Year, int32& Month, int32& DayOfWeek, int32& Day, int32& Hour, int32& Min, int32& Sec, int32& MSec);
};
struct FPlatformMisc { static int32 NumberOfCores(); static int32 NumberOfCoresIncludingHyperthreads(); static void RequestExit(bool Force); static FString GetEnvironmentVariable(const TCHAR* VariableName); static void LowLevelOutputDebugString(const TCHAR* Message); };
struct FPlatformProcess { static void Sleep(float Seconds); static const TCHAR* BaseDir(); };
struct FPlatformMath;
struct FPlatformAtomics { static int32 InterlockedIncrement(volatile int32* Value); static int32 InterlockedDecrement(volatile int32* Value); static int32 InterlockedAdd(volatile int32* Value, int32 Amount); };
template<typename T> class TAtomic { public: TAtomic(); TAtomic(T V); T Load() const; void Store(T V); operator T() const; T operator++(); T operator++(int); T operator--(); T operator+=(T); T operator=(T); T Exchange(T); };
class FCriticalSection { public: void Lock(); void Unlock(); };
class FScopeLock { public: explicit FScopeLock(FCriticalSection* Sync); ~FScopeLock(); };
struct FTimespan;
struct FDateTime { FDateTime(); FDateTime(int32 Year, int32 Month, int32 Day, int32 Hour = 0, int32 Minute = 0, int32 Second = 0, int32 Millisecond = 0); int32 GetDay() const; int32 GetMonth() const; int32 GetSecond() const; FTimespan operator-(const FDateTime& Other) const; bool operator<(const FDateTime& O) const; bool operator>(const FDateTime& O) const; bool operator==(const FDateTime& O) const; static FDateTime Now(); static FDateTime UtcNow(); FString ToString() const; FString ToString(const TCHAR* Format) const; int32 GetYear() const; int32 GetHour() const; int32 GetMinute() const; int64 GetTicks() const; };
struct FTimespan { FTimespan(); double GetTotalSeconds() const; double GetTotalMinutes() const; double GetTotalHours() const; double GetTotalMilliseconds() const; int32 GetSeconds() const; int32 GetMinutes() const; int32 GetHours() const; static FTimespan FromSeconds(double Seconds); };
struct FGuid { FGuid(); FGuid(uint32 A, uint32 B, uint32 C, uint32 D); static FGuid NewGuid(); bool IsValid() const; FString ToString() const; uint32 A, B, C, D; bool operator==(const FGuid&) const; bool operator!=(const FGuid&) const; };
uint32 GetTypeHash(const FGuid& G);
struct FPaths
{
	static FString ProjectDir(); static FString ProjectContentDir(); static FString ProjectSavedDir(); static FString ProjectConfigDir(); static FString EngineDir(); static FString ProjectLogDir();
	template<typename... PathTypes> static FString Combine(PathTypes&&... InPaths);
	static bool FileExists(const FString& InPath); static bool DirectoryExists(const FString& InPath);
	static FString GetBaseFilename(const FString& InPath, bool bRemovePath = true); static FString GetExtension(const FString& InPath, bool bIncludeDot = false); static FString GetPath(const FString& InPath); static FString GetCleanFilename(const FString& InPath);
	static void NormalizeFilename(FString& InPath); static FString ConvertRelativePathToFull(const FString& InPath);
};
struct FFileHelper
{
	enum class EHashOptions { None = 0, EnableVerify = 1, ErrorMissingHash = 2 };
	enum class EEncodingOptions { AutoDetect, ForceAnsi, ForceUnicode, ForceUTF8, ForceUTF8WithoutBOM };
	static bool LoadFileToString(FString& Result, const TCHAR* Filename, EHashOptions VerifyFlags = EHashOptions::None, uint32 ReadFlags = 0);
	static bool LoadFileToArray(TArray<uint8>& Result, const TCHAR* Filename, uint32 Flags = 0);
	static bool LoadFileToStringArray(TArray<FString>& Result, const TCHAR* Filename);
	static bool SaveStringToFile(const FString& String, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, class IFileManager* FileManager = nullptr, uint32 WriteFlags = 0);
	static bool SaveStringArrayToFile(const TArray<FString>& Lines, const TCHAR* Filename, EEncodingOptions EncodingOptions = EEncodingOptions::AutoDetect, class IFileManager* FileManager = nullptr, uint32 WriteFlags = 0);
	static bool SaveArrayToFile(const TArray<uint8>& Array, const TCHAR* Filename, class IFileManager* FileManager = nullptr, uint32 WriteFlags = 0);
};
class FArchive
{
public:
	virtual ~FArchive();
	bool IsLoading() const; bool IsSaving() const; bool IsError() const; int64 Tell(); int64 TotalSize(); void Seek(int64 InPos);
	virtual void Serialize(void* V, int64 Length);
	friend FArchive& operator<<(FArchive& Ar, int32& V); friend FArchive& operator<<(FArchive& Ar, uint32& V);
	friend FArchive& operator<<(FArchive& Ar, int8& V); friend FArchive& operator<<(FArchive& Ar, uint8& V);
	friend FArchive& operator<<(FArchive& Ar, int16& V); friend FArchive& operator<<(FArchive& Ar, uint16& V);
	friend FArchive& operator<<(FArchive& Ar, int64& V); friend FArchive& operator<<(FArchive& Ar, uint64& V);
	friend FArchive& operator<<(FArchive& Ar, float& V); friend FArchive& operator<<(FArchive& Ar, double& V);
	friend FArchive& operator<<(FArchive& Ar, bool& V); friend FArchive& operator<<(FArchive& Ar, FString& V);
	friend FArchive& operator<<(FArchive& Ar, FName& V); friend FArchive& operator<<(FArchive& Ar, FText& V);
	template<typename T, typename A> friend FArchive& operator<<(FArchive& Ar, TArray<T, A>& V) { return Ar; }
	template<typename K, typename V2> friend FArchive& operator<<(FArchive& Ar, TMap<K, V2>& V) { return Ar; }
};
class IFileManager
{
public:
	static IFileManager& Get();
	virtual bool Delete(const TCHAR* Filename, bool RequireExists = 0, bool EvenReadOnly = 0, bool Quiet = 0) = 0;
	virtual bool MakeDirectory(const TCHAR* Path, bool Tree = 0) = 0;
	virtual bool DirectoryExists(const TCHAR* InDirectory) = 0;
	virtual bool Move(const TCHAR* Dest, const TCHAR* Src, bool Replace = 1, bool EvenIfReadOnly = 0, bool Attributes = 0, bool bDoNotRetryOrError = 0) = 0;
	virtual int64 FileSize(const TCHAR* Filename) = 0;
	bool FileExists(const TCHAR* Filename);
	virtual void FindFiles(TArray<FString>& FileNames, const TCHAR* Filename, bool Files, bool Directories) = 0;
	virtual FArchive* CreateFileReader(const TCHAR* Filename, uint32 ReadFlags = 0) = 0;
	virtual FArchive* CreateFileWriter(const TCHAR* Filename, uint32 WriteFlags = 0) = 0;
};
class IPlatformFile { public: virtual ~IPlatformFile(); virtual bool FileExists(const TCHAR* Filename) = 0; virtual bool DeleteFile(const TCHAR* Filename) = 0; virtual bool DirectoryExists(const TCHAR* Directory) = 0; virtual bool CreateDirectoryTree(const TCHAR* Directory); virtual bool CreateDirectory(const TCHAR* Directory) = 0; virtual int64 FileSize(const TCHAR* Filename) = 0; };
class FPlatformFileManager { public: static FPlatformFileManager& Get(); IPlatformFile& GetPlatformFile(); };
class FMemoryWriter : public FArchive { public: FMemoryWriter(TArray<uint8>& InBytes, bool bIsPersistent = false, bool bSetOffset = false, const FName InArchiveName = NAME_None); };
class FMemoryReader : public FArchive { public: explicit FMemoryReader(const TArray<uint8>& InBytes, bool bIsPersistent = false); };
class FOutputDevice { public: virtual ~FOutputDevice(); virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) = 0; template<typename... T> void Logf(const TCHAR* Fmt, T... Args); };

// ---------- параллельность ----------
enum class EParallelForFlags { None = 0, ForceSingleThread = 1, Unbalanced = 2, PumpRenderingThread = 4, BackgroundPriority = 8 };
ENUM_CLASS_FLAGS(EParallelForFlags)
void ParallelFor(int32 Num, TFunctionRef<void(int32)> Body, bool bForceSingleThread = false, bool bPumpRenderingThread = false);
void ParallelFor(int32 Num, TFunctionRef<void(int32)> Body, EParallelForFlags Flags);
namespace ENamedThreads { enum Type { UnusedAnchor = -1, RHIThread, AudioThread, GameThread, ActualRenderingThread, AnyThread = 0xff, AnyBackgroundThreadNormalTask = 0x100, AnyHiPriThreadNormalTask = 0x200 }; }
bool IsInGameThread();
bool IsInRenderingThread();
template<typename F> void AsyncTask(ENamedThreads::Type Thread, F&& Function);
template<typename T> class TFuture { public: T Get() const; bool IsReady() const; bool IsValid() const; void Wait() const; };
template<> class TFuture<void> { public: void Get() const; bool IsReady() const; bool IsValid() const; void Wait() const; };
enum class EAsyncExecution { TaskGraph, TaskGraphMainThread, Thread, ThreadIfForkSafe, ThreadPool, LargeThreadPool };
template<typename F> auto Async(EAsyncExecution Execution, F&& Function, TUniqueFunction<void()> CompletionCallback = nullptr) -> TFuture<decltype(Function())>;

// ---------- консольные переменные ----------
enum EConsoleVariableFlags { ECVF_Default = 0x0, ECVF_Cheat = 0x1, ECVF_ReadOnly = 0x4, ECVF_Unregistered = 0x8, ECVF_RenderThreadSafe = 0x20, ECVF_Scalability = 0x40, ECVF_ScalabilityGroup = 0x80, ECVF_SetByConstructor = 0x0, ECVF_SetByScalability = 0x1000000, ECVF_SetByGameSetting = 0x2000000, ECVF_SetByProjectSetting = 0x3000000, ECVF_SetByCode = 0x0C000000, ECVF_SetByConsole = 0x0E000000 };
class IConsoleVariable;
class IConsoleObject { public: virtual ~IConsoleObject(); virtual IConsoleVariable* AsVariable(); };
class IConsoleVariable : public IConsoleObject
{
public:
	virtual void Set(const TCHAR* InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode) = 0;
	void Set(bool InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode); void Set(int32 InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode); void Set(float InValue, EConsoleVariableFlags SetBy = ECVF_SetByCode);
	virtual int32 GetInt() const = 0; virtual float GetFloat() const = 0; virtual FString GetString() const = 0; bool GetBool() const;
};
class IConsoleCommand : public IConsoleObject {};
DECLARE_LOG_CATEGORY_EXTERN(LogConsoleResponse, Log, All);
struct IConsoleManager
{
	static IConsoleManager& Get();
	virtual IConsoleVariable* FindConsoleVariable(const TCHAR* Name, bool bTrackFrequentCalls = true) const = 0;
	virtual IConsoleObject* FindConsoleObject(const TCHAR* Name, bool bTrackFrequentCalls = true) const = 0;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, int32 DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, float DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, bool DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	virtual IConsoleVariable* RegisterConsoleVariable(const TCHAR* Name, const FString& DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default) = 0;
	virtual void UnregisterConsoleObject(IConsoleObject* ConsoleObject, bool bKeepState = true) = 0;
};
template<class T>
class TAutoConsoleVariable
{
public:
	TAutoConsoleVariable(const TCHAR* Name, const T& DefaultValue, const TCHAR* Help, uint32 Flags = ECVF_Default);
	T GetValueOnGameThread() const; T GetValueOnRenderThread() const; T GetValueOnAnyThread(bool bForceGameThread = false) const;
	IConsoleVariable& operator*(); const IConsoleVariable& operator*() const; IConsoleVariable* operator->() const;
	IConsoleVariable* AsVariable();
};
class FAutoConsoleVariableRef { public: FAutoConsoleVariableRef(const TCHAR* Name, int32& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default); FAutoConsoleVariableRef(const TCHAR* Name, float& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default); FAutoConsoleVariableRef(const TCHAR* Name, bool& RefValue, const TCHAR* Help, uint32 Flags = ECVF_Default); };

// ---------- модули ----------
class IModuleInterface { public: virtual ~IModuleInterface(); virtual void StartupModule(); virtual void ShutdownModule(); virtual bool IsGameModule() const; };
class FDefaultModuleImpl : public IModuleInterface {};
class FDefaultGameModuleImpl : public FDefaultModuleImpl { public: virtual bool IsGameModule() const override; };
class FModuleManager { public: static FModuleManager& Get(); template<typename T> static T& LoadModuleChecked(const FName ModuleName); bool IsModuleLoaded(const FName InModuleName) const; };
#define IMPLEMENT_MODULE(ModuleImplClass, ModuleName) static_assert(sizeof(ModuleImplClass) > 0, "module"); extern "C" IModuleInterface* InitializeModule_##ModuleName();
#define IMPLEMENT_GAME_MODULE(ModuleImplClass, ModuleName) IMPLEMENT_MODULE(ModuleImplClass, ModuleName)
#define IMPLEMENT_PRIMARY_GAME_MODULE(ModuleImplClass, ModuleName, GameName) IMPLEMENT_MODULE(ModuleImplClass, ModuleName) static_assert(sizeof(GameName) > 1, "game name");

// ---------- делегаты ----------
#include "TPStub/Delegates.h"

// ---------- TEnumAsByte ----------
template<class TEnum>
class TEnumAsByte
{
public:
	typedef TEnum EnumType;
	TEnumAsByte(); TEnumAsByte(TEnum InValue); explicit TEnumAsByte(int32 InValue); explicit TEnumAsByte(uint8 InValue);
	TEnumAsByte& operator=(TEnum InValue);
	bool operator==(TEnum InValue) const; bool operator==(TEnumAsByte InValue) const;
	operator TEnum() const; TEnum GetValue() const;
};
template<class T> uint32 GetTypeHash(const TEnumAsByte<T>& E);
