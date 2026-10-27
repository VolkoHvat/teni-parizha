// Макет делегатов UE 4.27: проверка сигнатур при привязке (BindUObject, AddDynamic и т. д.).
#pragma once
// TP_STUB_MODULE: Core

class UObject;
template<typename T> struct TTPIdentity { typedef T Type; };

class FDelegateHandle
{
public:
	FDelegateHandle(); bool IsValid() const; void Reset();
	bool operator==(const FDelegateHandle& O) const; bool operator!=(const FDelegateHandle& O) const;
};

template<typename FuncType> class TDelegate;
template<typename Ret, typename... Args>
class TDelegate<Ret(Args...)>
{
public:
	TDelegate(); TDelegate(std::nullptr_t);
	template<class U, typename... V> static TDelegate CreateUObject(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> static TDelegate CreateUObject(const U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...) const>::Type F, V... Vars);
	template<class U, typename... V> static TDelegate CreateRaw(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> static TDelegate CreateRaw(const U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...) const>::Type F, V... Vars);
	template<class U, typename... V> static TDelegate CreateSP(const TSharedRef<U>& Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> static TDelegate CreateSP(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<typename... V> static TDelegate CreateStatic(typename TTPIdentity<Ret (*)(Args..., V...)>::Type F, V... Vars);
	template<typename F, typename... V> static TDelegate CreateLambda(F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); return TDelegate(); }
	template<class U, typename F, typename... V> static TDelegate CreateWeakLambda(U* Obj, F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); return TDelegate(); }
	template<class U, typename... V> static TDelegate CreateUFunction(U* Obj, const FName& FunctionName, V... Vars);

	template<class U, typename... V> void BindUObject(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> void BindUObject(const U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...) const>::Type F, V... Vars);
	template<class U, typename... V> void BindRaw(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> void BindRaw(const U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...) const>::Type F, V... Vars);
	template<class U, typename... V> void BindSP(U* Obj, typename TTPIdentity<Ret (U::*)(Args..., V...)>::Type F, V... Vars);
	template<typename... V> void BindStatic(typename TTPIdentity<Ret (*)(Args..., V...)>::Type F, V... Vars);
	template<typename F, typename... V> void BindLambda(F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); }
	template<class U, typename F, typename... V> void BindWeakLambda(U* Obj, F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); }
	template<class U, typename... V> void BindUFunction(U* Obj, const FName& FunctionName, V... Vars);

	Ret Execute(Args... A) const;
	template<typename R2 = Ret> typename std::enable_if<std::is_void<R2>::value, bool>::type ExecuteIfBound(Args... A) const { return true; }
	bool IsBound() const; void Unbind(); FDelegateHandle GetHandle() const;
	template<class U> bool IsBoundToObject(const U* Obj) const;
};
template<typename Ret, typename... Args> using TBaseDelegate = TDelegate<Ret(Args...)>;

template<typename FuncType> class TMulticastDelegate;
template<typename... Args>
class TMulticastDelegate<void(Args...)>
{
public:
	typedef TDelegate<void(Args...)> FDelegate;
	FDelegateHandle Add(const FDelegate& D);
	template<class U, typename... V> FDelegateHandle AddUObject(U* Obj, typename TTPIdentity<void (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> FDelegateHandle AddUObject(const U* Obj, typename TTPIdentity<void (U::*)(Args..., V...) const>::Type F, V... Vars);
	template<class U, typename... V> FDelegateHandle AddRaw(U* Obj, typename TTPIdentity<void (U::*)(Args..., V...)>::Type F, V... Vars);
	template<class U, typename... V> FDelegateHandle AddSP(U* Obj, typename TTPIdentity<void (U::*)(Args..., V...)>::Type F, V... Vars);
	template<typename... V> FDelegateHandle AddStatic(typename TTPIdentity<void (*)(Args..., V...)>::Type F, V... Vars);
	template<typename F, typename... V> FDelegateHandle AddLambda(F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); return FDelegateHandle(); }
	template<class U, typename F, typename... V> FDelegateHandle AddWeakLambda(U* Obj, F&& Functor, V... Vars) { (void)sizeof((Functor(std::declval<Args>()..., Vars...), 0)); return FDelegateHandle(); }
	template<class U, typename... V> FDelegateHandle AddUFunction(U* Obj, const FName& FunctionName, V... Vars);
	bool Remove(FDelegateHandle Handle);
	int32 RemoveAll(const void* InUserObject);
	void Clear();
	bool IsBound() const;
	bool IsBoundToObject(const void* InUserObject) const;
	void Broadcast(Args... A) const;
};
template<typename... Args> using TBaseMulticastDelegate = TMulticastDelegate<void(Args...)>;

// Динамические делегаты: функция должна быть UFUNCTION (проверяет линтер); сигнатура проверяется здесь.
class FScriptDelegate { public: void BindUFunction(UObject* InObject, const FName& InFunctionName); bool IsBound() const; void Unbind(); };
template<typename Ret, typename... Args>
class TTPDynamicDelegate
{
public:
	template<class U> void __Internal_BindDynamic(U* Obj, typename TTPIdentity<Ret (U::*)(Args...)>::Type F, FName FuncName);
	template<class U> void __Internal_BindDynamic(U* Obj, typename TTPIdentity<Ret (U::*)(Args...) const>::Type F, FName FuncName);
	void BindUFunction(UObject* InObject, const FName& InFunctionName);
	Ret Execute(Args... A) const;
	template<typename R2 = Ret> typename std::enable_if<std::is_void<R2>::value, bool>::type ExecuteIfBound(Args... A) const { return true; }
	bool IsBound() const; void Unbind(); void Clear();
	UObject* GetUObject() const; FName GetFunctionName() const;
};
template<typename... Args>
class TTPDynamicMulticastDelegate
{
public:
	template<class U> void __Internal_AddDynamic(U* Obj, typename TTPIdentity<void (U::*)(Args...)>::Type F, FName FuncName);
	template<class U> void __Internal_AddUniqueDynamic(U* Obj, typename TTPIdentity<void (U::*)(Args...)>::Type F, FName FuncName);
	template<class U> void __Internal_RemoveDynamic(U* Obj, typename TTPIdentity<void (U::*)(Args...)>::Type F, FName FuncName);
	template<class U> bool __Internal_IsAlreadyBound(U* Obj, typename TTPIdentity<void (U::*)(Args...)>::Type F, FName FuncName) const;
	void Add(const FScriptDelegate& D); void AddUnique(const FScriptDelegate& D); void Remove(const FScriptDelegate& D);
	void RemoveAll(const UObject* Object);
	void Clear();
	bool IsBound() const;
	bool Contains(const UObject* Obj, FName FunctionName) const;
	void Broadcast(Args... A) const;
};
#define STATIC_FUNCTION_FNAME(str) FName(str)
#define AddDynamic(UserObject, FuncName) __Internal_AddDynamic(UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)))
#define AddUniqueDynamic(UserObject, FuncName) __Internal_AddUniqueDynamic(UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)))
#define RemoveDynamic(UserObject, FuncName) __Internal_RemoveDynamic(UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)))
#define IsAlreadyBound(UserObject, FuncName) __Internal_IsAlreadyBound(UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)))
#define BindDynamic(UserObject, FuncName) __Internal_BindDynamic(UserObject, FuncName, STATIC_FUNCTION_FNAME(TEXT(#FuncName)))

#define TP_DECL_DELEGATE(Name, Ret, ...) typedef TDelegate<Ret(__VA_ARGS__)> Name
#define TP_DECL_MULTICAST(Name, ...) typedef TMulticastDelegate<void(__VA_ARGS__)> Name

#define DECLARE_DELEGATE(D) TP_DECL_DELEGATE(D, void)
#define DECLARE_DELEGATE_OneParam(D, T1) TP_DECL_DELEGATE(D, void, T1)
#define DECLARE_DELEGATE_TwoParams(D, T1, T2) TP_DECL_DELEGATE(D, void, T1, T2)
#define DECLARE_DELEGATE_ThreeParams(D, T1, T2, T3) TP_DECL_DELEGATE(D, void, T1, T2, T3)
#define DECLARE_DELEGATE_FourParams(D, T1, T2, T3, T4) TP_DECL_DELEGATE(D, void, T1, T2, T3, T4)
#define DECLARE_DELEGATE_FiveParams(D, T1, T2, T3, T4, T5) TP_DECL_DELEGATE(D, void, T1, T2, T3, T4, T5)
#define DECLARE_DELEGATE_RetVal(R, D) TP_DECL_DELEGATE(D, R)
#define DECLARE_DELEGATE_RetVal_OneParam(R, D, T1) TP_DECL_DELEGATE(D, R, T1)
#define DECLARE_DELEGATE_RetVal_TwoParams(R, D, T1, T2) TP_DECL_DELEGATE(D, R, T1, T2)
#define DECLARE_DELEGATE_RetVal_ThreeParams(R, D, T1, T2, T3) TP_DECL_DELEGATE(D, R, T1, T2, T3)
#define DECLARE_MULTICAST_DELEGATE(D) TP_DECL_MULTICAST(D)
#define DECLARE_MULTICAST_DELEGATE_OneParam(D, T1) TP_DECL_MULTICAST(D, T1)
#define DECLARE_MULTICAST_DELEGATE_TwoParams(D, T1, T2) TP_DECL_MULTICAST(D, T1, T2)
#define DECLARE_MULTICAST_DELEGATE_ThreeParams(D, T1, T2, T3) TP_DECL_MULTICAST(D, T1, T2, T3)
#define DECLARE_MULTICAST_DELEGATE_FourParams(D, T1, T2, T3, T4) TP_DECL_MULTICAST(D, T1, T2, T3, T4)
#define DECLARE_EVENT(Owner, D) TP_DECL_MULTICAST(D)
#define DECLARE_EVENT_OneParam(Owner, D, T1) TP_DECL_MULTICAST(D, T1)
#define DECLARE_EVENT_TwoParams(Owner, D, T1, T2) TP_DECL_MULTICAST(D, T1, T2)

#define DECLARE_DYNAMIC_DELEGATE(D) class D : public TTPDynamicDelegate<void> {}
#define DECLARE_DYNAMIC_DELEGATE_OneParam(D, T1, N1) class D : public TTPDynamicDelegate<void, T1> {}
#define DECLARE_DYNAMIC_DELEGATE_TwoParams(D, T1, N1, T2, N2) class D : public TTPDynamicDelegate<void, T1, T2> {}
#define DECLARE_DYNAMIC_DELEGATE_ThreeParams(D, T1, N1, T2, N2, T3, N3) class D : public TTPDynamicDelegate<void, T1, T2, T3> {}
#define DECLARE_DYNAMIC_DELEGATE_RetVal(R, D) class D : public TTPDynamicDelegate<R> {}
#define DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(R, D, T1, N1) class D : public TTPDynamicDelegate<R, T1> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE(D) class D : public TTPDynamicMulticastDelegate<> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(D, T1, N1) class D : public TTPDynamicMulticastDelegate<T1> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(D, T1, N1, T2, N2) class D : public TTPDynamicMulticastDelegate<T1, T2> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(D, T1, N1, T2, N2, T3, N3) class D : public TTPDynamicMulticastDelegate<T1, T2, T3> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(D, T1, N1, T2, N2, T3, N3, T4, N4) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(D, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4, T5> {}
#define DECLARE_DYNAMIC_MULTICAST_DELEGATE_SixParams(D, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4, T5, T6> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(D, Owner, Prop, T1, N1) class D : public TTPDynamicMulticastDelegate<T1> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(D, Owner, Prop, T1, N1, T2, N2) class D : public TTPDynamicMulticastDelegate<T1, T2> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_ThreeParams(D, Owner, Prop, T1, N1, T2, N2, T3, N3) class D : public TTPDynamicMulticastDelegate<T1, T2, T3> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(D, Owner, Prop, T1, N1, T2, N2, T3, N3, T4, N4) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SixParams(D, Owner, Prop, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5, T6, N6) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4, T5, T6> {}
#define DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams(D, Owner, Prop, T1, N1, T2, N2, T3, N3, T4, N4, T5, N5) class D : public TTPDynamicMulticastDelegate<T1, T2, T3, T4, T5> {}

DECLARE_DELEGATE(FSimpleDelegate);
DECLARE_MULTICAST_DELEGATE(FSimpleMulticastDelegate);
