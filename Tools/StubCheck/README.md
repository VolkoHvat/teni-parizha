# Стенд проверки (StubCheck)

В облачной среде нет Unreal Engine, поэтому код проверяется на **макете заголовков UE 4.27**. Стенд проверяет синтаксис и сигнатуры с помощью clang. Код при этом не компонуется и не запускается.

## Запуск
```
Tools/StubCheck/check.sh                  # всё: каждый .cpp, unity-сборка, AST-проверка, линтер
Tools/StubCheck/check.sh Source/TeniParizha/Private/TPGuard.cpp   # только эти файлы (+ их заголовки)
Tools/StubCheck/check.sh --unity          # только unity-сборка
Tools/StubCheck/check.sh --lint           # только линтер (+ конфиги, .uproject, Target/Build.cs)
Tools/StubCheck/check.sh --selftest       # самопроверка стенда
```
Последняя строка вывода — `ИТОГ: чисто` или `ИТОГ: есть ошибки`. Код возврата 0 означает, что ошибок нет. Полный прогон занимает около 15 с.

Что нужно: `clang++`, `python3`. Для AST-проверки нужен модуль `libclang` для Python. При необходимости `check.sh` сам ставит его командой `pip install libclang==18.1.1`. Если модуля нет, AST-шаг пропускается с предупреждением.

## Что проверяется
1. **Компиляция каждого .cpp** (`-std=c++14`, стандарт UE 4.27 по умолчанию; `WITH_EDITOR=1`, как в сборке Development Editor):
   - нет API из UE5: TObjectPtr, FVector3f, UE_PI, Enhanced Input, GetSkeletalMeshAsset, GetPlatformData, UTickableWorldSubsystem, AddInstance(…, bWorldSpace), конструктор FTexture2DMipMap(x, y, z) и т. п.;
   - FVector и FMath — на float. Поэтому `FMath::Max(double, float)` — ошибка, как в 4.27;
   - закрытые члены 4.27 недоступны: `RelativeLocation`, `ACharacter::Mesh`, `UStaticMeshComponent::StaticMesh`, `WalkableFloorAngle`, `AActor::bHidden`;
   - сигнатуры: SetTimer с делегатом или лямбдой без `bLoop` — ошибка; AddDynamic, BindAction и BindAxis проверяют тип функции; `Sort` у `TArray<T*>` передаёт в предикат ссылки, как в UE;
   - `UE_LOG` и `FString::Printf`: формат только `TEXT("…")`, а FString надо передавать как `*Str`, иначе — static_assert;
   - `TArray::RemoveAt(i, false)` — ошибка: второй параметр — это Count;
   - затенение имён (`-Werror=shadow`): в UE 4.27 с BuildSettingsVersion.V2 это ошибки C4456–C4459;
   - строгие include: неполный тип означает, что не хватает `#include`. В настоящем UE это иногда маскирует общий PCH, но лишний include безвреден.
2. **Unity-сборка**: все .cpp собираются в одну единицу трансляции с `WITH_EDITOR=0`, как в игровой сборке. Так ловятся одноимённые `static`-функции в разных файлах, «утёкшие» макросы и код, который работает только в редакторе.
3. **AST-проверка** (`tp_ast.py`, libclang):
   - параметр или локальная переменная в теле функции называется так же, как поле класса или его базы (MSVC C4458);
   - указатель на UObject (или контейнер с ним) в поле UCLASS/USTRUCT без `UPROPERTY()`: сборщик мусора удалит объект;
   - `AddDynamic`/`BindDynamic` на функцию без `UFUNCTION()`;
   - тело `BlueprintImplementableEvent` в .cpp.
4. **Линтер** (`lint.py`):
   - `.generated.h` подключён последним, и его имя совпадает с именем файла;
   - у UCLASS/USTRUCT есть GENERATED_BODY, у имён правильные префиксы;
   - `UENUM(BlueprintType) enum class … : uint8`;
   - у UPROPERTY нет типов, которых блюпринты 4.27 не знают (double, uint32 …), и вложенных контейнеров;
   - заголовки взяты только из модулей, перечисленных в `TeniParizha.Build.cs` (модуль каждого заголовка макета указан строкой `TP_STUB_MODULE`);
   - имена из UE5;
   - запреты проекта: объёмные облака и туман, трассировка лучей, Distance Field, SkyLight с пересъёмкой каждый кадр;
   - русские строки в файле без UTF-8 BOM;
   - `#pragma once`, LOCTEXT_NAMESPACE;
   - конфиги: DX11, нет параметров UE5, не больше 3 каскадов теней;
   - `.uproject` указывает на 4.27, в `Target.cs` стоит `BuildSettingsVersion.V2` и нет `IncludeOrderVersion`.

## Устройство
- `ue/` — макет заголовков по настоящим путям UE (`GameFramework/Character.h` и т. д.). Ядро лежит в `ue/TPStub/` (Core, Math, UObjectCore, Delegates). Функции только объявлены, тел нет: для `-fsyntax-only` этого достаточно. Шаблоны с телом проверяют типы аргументов.
- `gen_generated.py` заменяет UnrealHeaderTool. Для каждого `GENERATED_BODY()` он создаёт макрос с `Super`/`ThisClass`/`StaticClass()` и конструктором по умолчанию, если класс не объявил свой. Повторное подключение `.generated.h` даёт ошибку, как в UHT.
- `UPROPERTY(...)`/`UFUNCTION(...)` раскрываются в `__attribute__((annotate("tp_uproperty:…")))`. По этим аннотациям AST-проверка видит отражение.
- `selftest/good` — эталонный правильный код 4.27 с широким охватом API: он должен проходить чисто. `selftest/bad` — плохие примеры с метками `// ОЖИДАЕТСЯ: <текст>` (сообщение обязано появиться) и `// НЕ_ОЖИДАЕТСЯ: <текст>` (ложного срабатывания быть не должно).

## Как расширять макет
Если стенд пишет `no member named …` или `file not found` для API, который **точно есть в 4.27**:
1. Добавьте объявление в нужный заголовок `ue/…` с точной сигнатурой 4.27, включая параметры по умолчанию, `const` и `virtual`.
2. Новый заголовок кладите по настоящему пути и указывайте во второй строке `// TP_STUB_MODULE: <Модуль>`.
3. Добавьте вызов в `selftest/good/Private/TPGoodActor.cpp` и запустите `check.sh --selftest`.

Нельзя добавлять ничего из UE5 «для удобства»: смысл стенда в том, чтобы ловить именно это.

## Ограничения
- Проверяются только синтаксис и типы. Компоновка (нет тела у функции) и поведение во время работы не проверяются.
- Макет написан по памяти о 4.27, без исходников движка. Редкие члены могут отсутствовать: это ложная ошибка, и тогда макет нужно расширить. Возможны и неточности. Последнее слово остаётся за сборкой у владельца.
- MSVC местами мягче clang (двухфазный поиск имён в шаблонах). Стенд строже, но это безопасно.
- Предупреждения MSVC о преобразованиях (C4244 и т. п.) не воспроизводятся.
- `TCHAR` 16-битный (`-fshort-wchar`), как в Windows.
- Правило про BOM перестраховочное. Без ключа `/utf-8` MSVC читает файл без BOM в кодовой странице системы. Файл с BOM корректен в любом случае.
