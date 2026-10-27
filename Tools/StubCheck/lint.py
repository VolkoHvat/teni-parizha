#!/usr/bin/env python3
# Линтер «Тени Парижа» для UE 4.27: то, чего не поймает компилятор на макете.
# Уровни: ОШИБКА — сломает сборку/запуск в UE 4.27 или нарушает правила проекта; ПРЕДУПРЕЖДЕНИЕ — стоит исправить.
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gen_generated import strip_comments  # noqa: E402

# Имена из UE5, которых нет в 4.27 (или есть, но запрещены правилами проекта).
UE5_NAMES = [
    (r'\bTObjectPtr\b', 'TObjectPtr — это UE5; в 4.27 сырой указатель под UPROPERTY()'),
    (r'\bMakeObjectPtr\b|\bObjectPtrDecay\b|\bTObjectPtrWrapTypeOf\b', 'API TObjectPtr — это UE5'),
    (r'\bFVector3[fd]\b|\bFVector2[fd]\b|\bFRotator3[fd]\b|\bFQuat4[fd]\b|\bFTransform3[fd]\b|\bFMatrix44[fd]\b|\bFBox3[fd]\b', 'типы LWC (FVector3f и т. п.) — это UE5; в 4.27 FVector на float'),
    (r'\bFVector::FReal\b|\bFReal\b', 'FReal — это UE5'),
    (r'\bUE::Math::', 'пространство UE::Math — это UE5'),
    (r'\bUE_(?:PI|HALF_PI|TWO_PI|INV_PI|SMALL_NUMBER|KINDA_SMALL_NUMBER|BIG_NUMBER|DOUBLE_PI|DOUBLE_SMALL_NUMBER|DELTA)\b', 'константы UE_PI/UE_SMALL_NUMBER — это UE5; в 4.27 PI, SMALL_NUMBER, KINDA_SMALL_NUMBER'),
    (r'\bUEnhancedInput\w*|\bUInputAction\b|\bUInputMappingContext\b|\bFInputActionValue\b|\bFInputActionInstance\b|\bETriggerEvent\b|\bEInputActionValueType\b|\bUInputModifier\w*|\bUInputTrigger\w*|\bFEnhancedActionKeyMapping\b|\bIEnhancedInputSubsystemInterface\b',
     'Enhanced Input запрещён в проекте: классический ввод BindAxis/BindAction и Config/DefaultInput.ini'),
    (r'\bGetSkeletalMeshAsset\b|\bSetSkeletalMeshAsset\b', 'GetSkeletalMeshAsset/SetSkeletalMeshAsset — это UE5.1; в 4.27 член SkeletalMesh и SetSkeletalMesh()'),
    (r'\bSetLeaderPoseComponent\b|\bLeaderPoseComponent\b|\bGetFollowerPoseComponents\b', 'Leader/Follower — это UE5.1; в 4.27 SetMasterPoseComponent'),
    (r'\bGetSkinnedAsset\b|\bSetSkinnedAsset\w*\b', 'SkinnedAsset — это UE5.1'),
    (r'->GetPlatformData\s*\(|\.GetPlatformData\s*\(|\bSetPlatformData\s*\(', 'GetPlatformData() — это UE5; в 4.27 открытый член PlatformData'),
    (r'\bUTickableWorldSubsystem\b', 'UTickableWorldSubsystem — это UE5; в 4.27 UWorldSubsystem + FTickableGameObject (Tickable.h)'),
    (r'\bNativeThreadSafeUpdateAnimation\b|\bBlueprintThreadSafeUpdateAnimation\b', 'ThreadSafeUpdateAnimation — это UE5'),
    (r'\bDoesSupportWorldType\b', 'UWorldSubsystem::DoesSupportWorldType — это UE5; в 4.27 ShouldCreateSubsystem'),
    (r'\bHitObjectHandle\b|\bFActorInstanceHandle\b', 'HitObjectHandle — это UE5; в 4.27 Hit.GetActor()/Hit.Actor'),
    (r'\bIsValidChecked\b|\bMarkAsGarbage\b|\bIsGarbage\b|\bClearGarbage\b', 'MarkAsGarbage/IsValidChecked — это UE5; в 4.27 MarkPendingKill/IsPendingKill/IsValid'),
    (r'\bUE_LOGFMT\b|\bUE_ASSUME\b', 'макрос из UE5'),
    (r'\bGetResource\s*\(\s*\)', 'UTexture::GetResource() — это UE5; в 4.27 член Resource'),
    (r'\bFTexture2DMipMap\s*\(\s*\w', 'конструктор FTexture2DMipMap(SizeX, SizeY, SizeZ) — это UE5; в 4.27 только по умолчанию, затем SizeX/SizeY'),
    (r'\bTStaticArray\s*<[^>]*>\s*\w+\s*\(\s*InPlace', 'TStaticArray InPlace — UE5'),
    (r'\bFDoubleInterval\b|\bFVector2f\b', 'тип из UE5'),
    (r'\bGetGravityDirection\b|\bSetGravityDirection\b', 'направление гравитации персонажа — это UE5.4'),
    (r'\bAddInstance\s*\([^;]*,\s*(?:true|false)\s*\)', 'AddInstance(Transform, bWorldSpace) — сигнатура UE5; в 4.27 AddInstance(Transform) или AddInstanceWorldSpace(Transform)'),
    (r'\bUE_DEPRECATED_FORGAME\b', 'макрос из UE5'),
]
HEAVY = [
    (r'\bUVolumetricCloudComponent\b|VolumetricCloud', 'объёмные облака запрещены (ПК 2014 года)'),
    (r'SetVolumetricFog\s*\(\s*true|bEnableVolumetricFog\s*=\s*true', 'объёмный туман запрещён'),
    (r'bRealTimeCapture\s*=\s*true', 'пересъёмка небесного света каждый кадр запрещена (SkyLight — раз в игровой час)'),
    (r'CastRaytracedShadow\s*=\s*(?:1|true)|bUseRayTracedDistanceFieldShadows\s*=\s*(?:1|true)|SetCastRaytracedShadow\s*\(\s*true', 'трассировка лучей / DF-тени запрещены'),
    (r'bGenerateMeshDistanceField\s*=\s*(?:1|true)', 'Distance Field запрещены'),
    (r'\bLumen(?!s\b)\w*|\bNanite\w*', 'Lumen/Nanite — это UE5 и запрещены'),
]
INI_RULES = [
    (r'^\s*r\.RayTracing\s*=\s*(?:1|True)', 'трассировка лучей запрещена'),
    (r'^\s*r\.GenerateMeshDistanceFields\s*=\s*(?:1|True)', 'Distance Field запрещены'),
    (r'^\s*r\.DistanceFieldAO\s*=\s*(?:1|True)|^\s*r\.AOGlobalDistanceField\s*=\s*1', 'Distance Field AO запрещён'),
    (r'^\s*r\.VolumetricFog\s*=\s*(?:1|True)', 'объёмный туман запрещён'),
    (r'^\s*r\.VolumetricCloud\s*=\s*(?:1|True)', 'объёмные облака запрещены'),
    (r'^\s*r\.(?:Lumen|Nanite|DynamicGlobalIlluminationMethod|ReflectionMethod|Shadow\.Virtual)\S*\s*=', 'параметр UE5 (Lumen/Nanite/VSM) — в 4.27 не существует'),
    (r'^\s*DefaultGraphicsRHI\s*=\s*DefaultGraphicsRHI_DX12', 'нужен DX11 (DefaultGraphicsRHI_DX11)'),
    (r'^\s*r\.SSR\.Quality\s*=\s*[34]', 'SSR высокого качества запрещён'),
    (r'^\s*r\.Shadow\.CSM\.MaxCascades\s*=\s*([4-9]|\d\d)', 'больше 3 каскадов теней солнца — против бюджета'),
    (r'^\s*\+?EnhancedInput|^\s*DefaultPlayerInputClass\s*=\s*/Script/EnhancedInput|^\s*DefaultInputComponentClass\s*=\s*/Script/EnhancedInput', 'Enhanced Input запрещён'),
]
BP_BAD_TYPES = r'(?:double|uint16|uint32|uint64|int16|int8|long|unsigned|char|wchar_t|short|size_t|SIZE_T)'


class Linter:
    def __init__(self, root, src, stub):
        self.root, self.src, self.stub = root, src, stub
        self.errors = 0
        self.warnings = 0
        self.notes = []
        self.modules = self.read_modules()
        self.stub_modules = self.read_stub_modules()

    def rel(self, p):
        return os.path.relpath(p, self.root)

    def err(self, path, line, msg):
        self.errors += 1
        print('%s:%d: ОШИБКА: %s' % (self.rel(path), line, msg))

    def warn(self, path, line, msg):
        self.warnings += 1
        print('%s:%d: предупреждение: %s' % (self.rel(path), line, msg))

    def read_modules(self):
        mods = set()
        for dp, _, fns in os.walk(self.src):
            for fn in fns:
                if fn.endswith('.Build.cs'):
                    txt = strip_comments(open(os.path.join(dp, fn), encoding='utf-8-sig').read())
                    for blk in re.findall(r'(?:Public|Private)DependencyModuleNames\s*\.\s*AddRange\s*\(\s*new\s+string\s*\[\s*\]\s*\{([^}]*)\}', txt):
                        mods.update(re.findall(r'"(\w+)"', blk))
                    mods.update(re.findall(r'(?:Public|Private)DependencyModuleNames\s*\.\s*Add\s*\(\s*"(\w+)"', txt))
        return mods

    def read_stub_modules(self):
        m = {}
        for dp, _, fns in os.walk(self.stub):
            for fn in fns:
                p = os.path.join(dp, fn)
                rel = os.path.relpath(p, self.stub).replace(os.sep, '/')
                head = open(p, encoding='utf-8').read(600)
                mm = re.search(r'TP_STUB_MODULE:\s*(\w+)', head)
                if mm:
                    m[rel] = mm.group(1)
        return m

    # ---------- исходники ----------
    def lint_source(self, path):
        raw_bytes = open(path, 'rb').read()
        has_bom = raw_bytes.startswith(b'\xef\xbb\xbf')
        raw = raw_bytes.decode('utf-8-sig', errors='replace')
        code = strip_comments(raw)
        lines = code.split('\n')
        is_header = path.endswith('.h')
        stem = os.path.splitext(os.path.basename(path))[0]

        # кодировка: MSVC без BOM читает файл в кодовой странице системы (на русской Windows — CP1251)
        nonascii_str = [i + 1 for i, l in enumerate(lines) if re.search(r'"[^"\n]*[^\x00-\x7f][^"\n]*"', l)]
        if nonascii_str and not has_bom:
            self.err(path, nonascii_str[0], 'русские строки в коде, а файл без UTF-8 BOM: MSVC прочтёт их как CP1251 (кракозябры в игре). Сохраните файл как «UTF-8 с BOM» (%d строк)' % len(nonascii_str))
        elif not has_bom and re.search(r'[^\x00-\x7f]', raw):
            self.notes.append(self.rel(path))

        includes = [(i + 1, m.group(1)) for i, l in enumerate(lines) for m in [re.match(r'\s*#\s*include\s*[<"]([^>"]+)[>"]', l)] if m]
        for ln, inc in includes:
            mod = self.stub_modules.get(inc)
            if mod and mod not in self.modules and mod not in ('Core', 'CoreUObject', 'Engine'):
                self.err(path, ln, '#include "%s" из модуля %s, которого нет в TeniParizha.Build.cs' % (inc, mod))
            if mod and mod not in self.modules and mod in ('Core', 'CoreUObject', 'Engine'):
                self.err(path, ln, 'модуль %s не подключён в Build.cs' % mod)

        if is_header:
            if not re.search(r'^\s*#\s*pragma\s+once', code, re.M):
                self.err(path, 1, 'нет #pragma once')
            reflected = re.search(r'\b(UCLASS|USTRUCT|UENUM|UINTERFACE)\s*\(', code)
            gen = [(ln, inc) for ln, inc in includes if inc.endswith('.generated.h')]
            if reflected and not gen:
                self.err(path, 1, 'есть UCLASS/USTRUCT/UENUM, но нет #include "%s.generated.h"' % stem)
            for ln, inc in gen:
                if inc != stem + '.generated.h':
                    self.err(path, ln, 'имя .generated.h должно совпадать с именем файла: "%s.generated.h"' % stem)
                if includes and includes[-1][1] != inc:
                    self.err(path, ln, '.generated.h должен быть последним #include в заголовке (UHT остановит сборку)')
            if gen and not reflected:
                self.warn(path, gen[0][0], '.generated.h подключён, а UCLASS/USTRUCT/UENUM нет')
            self.check_reflection(path, code)
        else:
            own = stem + '.h'
            if includes and os.path.basename(includes[0][1]) != own and os.path.exists(os.path.join(self.src, 'Public', own)) | os.path.exists(os.path.join(self.src, 'Private', own)):
                self.warn(path, includes[0][0], 'первым должен идти собственный заголовок "%s" (правило IWYU в UE)' % own)
            if re.search(r'\.generated\.h', code):
                self.err(path, 1, '.generated.h нельзя подключать в .cpp')

        for rx, msg in UE5_NAMES:
            for i, l in enumerate(lines):
                if re.search(rx, l):
                    self.err(path, i + 1, msg)
                    break
        for rx, msg in HEAVY:
            for i, l in enumerate(lines):
                if re.search(rx, l):
                    self.err(path, i + 1, 'правила проекта: ' + msg)
                    break
        for i, l in enumerate(lines):
            if re.search(r'#\s*include\s*"(?:Engine/HitResult\.h|Engine/OverlapResult\.h|EnhancedInput\w*\.h|InputAction\w*\.h|InputMappingContext\.h|InputModifiers\.h|InputTriggers\.h|EnhancedPlayerInput\.h)"', l):
                self.err(path, i + 1, 'заголовок UE5/Enhanced Input: в 4.27 FHitResult — Engine/EngineTypes.h, FOverlapResult — WorldCollision.h')
            if re.search(r'\bLOCTEXT\s*\(', l) and 'LOCTEXT_NAMESPACE' not in code:
                self.err(path, i + 1, 'LOCTEXT без #define LOCTEXT_NAMESPACE')
            if re.search(r'ConstructorHelpers::F(?:Object|Class)Finder', l):
                self.warn(path, i + 1, 'ConstructorHelpers падает без ассета и работает только в конструкторе; правило проекта — LoadObject с запасными путями')
        if not is_header and re.search(r'#\s*define\s+LOCTEXT_NAMESPACE', code) and not re.search(r'#\s*undef\s+LOCTEXT_NAMESPACE', code):
            self.err(path, 1, 'LOCTEXT_NAMESPACE не снят #undef в конце файла (сломает unity-сборку)')

    def check_reflection(self, path, code):
        # UCLASS/USTRUCT без GENERATED_BODY, enum class без uint8, свойства с типами, которых нет в блюпринтах 4.27
        for m in re.finditer(r'\b(UCLASS|USTRUCT|UINTERFACE)\s*\([^)]*\)\s*(class|struct)\s+(?:\w+_API\s+)?(\w+)[^{;]*\{', code):
            depth, k = 0, m.end() - 1
            for k in range(m.end() - 1, len(code)):
                if code[k] == '{':
                    depth += 1
                elif code[k] == '}':
                    depth -= 1
                    if depth == 0:
                        break
            body = code[m.end():k]
            line = code.count('\n', 0, m.start()) + 1
            if not re.search(r'\bGENERATED_(?:U\w+_)?BODY\s*\(', body):
                self.err(path, line, '%s %s без GENERATED_BODY()' % (m.group(1), m.group(3)))
            name = m.group(3)
            prefix = {'UCLASS': 'UA', 'USTRUCT': 'F', 'UINTERFACE': 'U'}[m.group(1)]
            if name[0] not in prefix:
                self.err(path, line, 'имя %s %s должно начинаться с %s (UHT требует префикс)' % (m.group(1), name, '/'.join(prefix)))
        for m in re.finditer(r'\bUENUM\s*\(([^)]*)\)\s*enum\s+class\s+(\w+)\s*(?::\s*(\w+))?', code):
            line = code.count('\n', 0, m.start()) + 1
            if m.group(3) != 'uint8' and 'BlueprintType' in m.group(1):
                self.err(path, line, 'UENUM(BlueprintType) enum class %s должен быть ": uint8" (в 4.27 блюпринты знают только uint8)' % m.group(2))
            if not m.group(2).startswith('E'):
                self.err(path, line, 'имя UENUM должно начинаться с E')
        for m in re.finditer(r'\bUPROPERTY\s*\(((?:[^()]|\([^()]*\))*)\)\s*([^;{]*?)\s*(\w+)\s*(?:=[^;]*|\{[^}]*\}|:\s*\d+)?;', code):
            spec, typ, name = m.group(1), m.group(2).strip(), m.group(3)
            line = code.count('\n', 0, m.start()) + 1
            bp = re.search(r'Blueprint(?:ReadWrite|ReadOnly|Assignable|Callable)', spec)
            if bp and re.fullmatch(r'(?:const\s+)?' + BP_BAD_TYPES + r'(?:\s+\w+)?', typ):
                self.err(path, line, 'UPROPERTY(%s) с типом %s: в 4.27 блюпринты не поддерживают этот тип (UHT: ошибка). Уберите Blueprint* или смените тип' % (bp.group(0), typ))
            if re.search(r'\bT(?:Array|Set|Map)\s*<[^;]*\bT(?:Array|Set|Map)\s*<', typ):
                self.err(path, line, 'вложенные контейнеры в UPROPERTY не поддерживаются UHT (оберните во внутренний USTRUCT)')
            if re.search(r'\bTWeakPtr\b|\bTSharedPtr\b|\bTUniquePtr\b|\bstd::', typ):
                self.err(path, line, 'тип %s нельзя делать UPROPERTY' % typ)
        for m in re.finditer(r'\bUFUNCTION\s*\(((?:[^()]|\([^()]*\))*)\)', code):
            spec = m.group(1)
            line = code.count('\n', 0, m.start()) + 1
            if 'BlueprintCallable' in spec or 'BlueprintPure' in spec:
                tail = code[m.end():code.find(';', m.end())] if code.find(';', m.end()) > 0 else ''
                if re.search(r'[(,]\s*(?:const\s+)?' + BP_BAD_TYPES + r'\s+\w+', tail) or re.match(r'\s*(?:virtual\s+|static\s+)*' + BP_BAD_TYPES + r'\s+\w+\s*\(', tail):
                    self.err(path, line, 'BlueprintCallable-функция с параметром/результатом неподдерживаемого типа (double, uint32 и т. п.) — UHT: ошибка в 4.27')

    # ---------- конфигурация ----------
    def lint_configs(self):
        cfg = os.path.join(self.root, 'Config')
        if os.path.isdir(cfg):
            for fn in sorted(os.listdir(cfg)):
                if not fn.endswith('.ini'):
                    continue
                p = os.path.join(cfg, fn)
                for i, l in enumerate(open(p, encoding='utf-8-sig', errors='replace')):
                    for rx, msg in INI_RULES:
                        if re.search(rx, l, re.I):
                            self.err(p, i + 1, msg)
        for fn in os.listdir(self.root):
            if fn.endswith('.uproject'):
                p = os.path.join(self.root, fn)
                txt = open(p, encoding='utf-8-sig').read()
                m = re.search(r'"EngineAssociation"\s*:\s*"([^"]*)"', txt)
                if not m or m.group(1) != '4.27':
                    self.err(p, 1, 'EngineAssociation должен быть "4.27" (сейчас %s)' % (m.group(1) if m else 'нет'))
                for plug in ('EnhancedInput', 'ModelingToolsEditorMode', 'GeometryScripting', 'Niagara'):
                    if re.search(r'"Name"\s*:\s*"%s"[^}]*"Enabled"\s*:\s*true' % plug, txt):
                        self.err(p, 1, 'плагин %s не нужен/недоступен в проекте под 4.27' % plug)
        for dp, _, fns in os.walk(os.path.join(self.root, 'Source')):
            for fn in fns:
                p = os.path.join(dp, fn)
                if fn.endswith('.Target.cs'):
                    txt = open(p, encoding='utf-8-sig').read()
                    if 'BuildSettingsVersion.Latest' in txt or 'BuildSettingsVersion.V2' not in txt:
                        self.err(p, 1, 'в 4.27 нужно DefaultBuildSettings = BuildSettingsVersion.V2')
                    if 'IncludeOrderVersion' in txt or 'EngineIncludeOrderVersion' in txt:
                        self.err(p, 1, 'IncludeOrderVersion/EngineIncludeOrderVersion — это UE5 (в 4.27 не скомпилируется)')
                if fn.endswith('.Build.cs'):
                    txt = open(p, encoding='utf-8-sig').read()
                    for mod in ('EnhancedInput', 'GeometryScriptingCore', 'GeometryFramework', 'Niagara', 'ModelingComponents'):
                        if '"%s"' % mod in txt:
                            self.err(p, 1, 'модуль %s не используется в проекте под 4.27' % mod)

    def run(self, files):
        if not files:
            files = sorted(os.path.join(dp, fn) for dp, _, fns in os.walk(self.src) for fn in fns if fn.endswith(('.h', '.cpp')))
            if os.path.realpath(self.src).startswith(os.path.realpath(os.path.join(self.root, 'Source')) + os.sep):
                self.lint_configs()
        for f in files:
            self.lint_source(f)
        if self.notes:
            print('примечание: %d файлов с русскими комментариями без BOM (безопасно, но лучше сохранить в UTF-8 с BOM)' % len(self.notes))
        print('== Линтер: ошибок %d, предупреждений %d' % (self.errors, self.warnings))
        return 1 if self.errors else 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--root', required=True)
    ap.add_argument('--src', required=True)
    ap.add_argument('--stub', required=True)
    ap.add_argument('files', nargs='*')
    a = ap.parse_args()
    files = []
    for f in a.files:
        files.append(os.path.abspath(f))
        if f.endswith('.cpp'):
            stem = os.path.splitext(os.path.basename(f))[0]
            for d in ('Public', 'Private'):
                h = os.path.join(a.src, d, stem + '.h')
                if os.path.exists(h):
                    files.append(h)
    return Linter(a.root, a.src, a.stub).run(files)


if __name__ == '__main__':
    sys.exit(main())
