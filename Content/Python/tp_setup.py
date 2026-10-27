# -*- coding: utf-8 -*-
"""
Тени Парижа — настройка проекта в редакторе Unreal Engine 5 (Python, плагин Python Editor Script Plugin).

Что делает (можно запускать сколько угодно раз — повторный запуск ничего не дублирует):
  1. создаёт папки /Game/TeniParizha/Materials и /Game/Maps;
  2. создаёт (или пересобирает) материалы
       M_TPBase        — непрозрачный: Albedo × VertexColor × Tint, Normal, ORM → AO / Roughness / Metallic, UV = TexCoord × UVScale;
       M_TPBaseMasked  — то же, Masked + Two Sided, Opacity Mask из альфы альбедо (листва, ткань навесов);
       M_TPEmissive    — свечение: Emissive = Color × Intensity (стёкла фонарей);
     текстуры по умолчанию для параметров — свои копии движковых (T_TPDefault_*), с правильными sRGB/сжатием,
     чтобы тип сэмплера совпадал с процедурными текстурами, которые игра подставляет во время работы;
  3. создаёт (или открывает) уровень /Game/Maps/Paris и ставит на него, если их ещё нет:
       ATPCityGenerator (квартал), ATPEnvironment (небо, солнце, туман), NavMeshBoundsVolume на весь квартал, PlayerStart у фонтана;
  4. сохраняет всё.

Как запустить: Tools → Execute Python Script… → выбрать этот файл,
или в консоли редактора (режим Python / команда py):  py "<путь к проекту>/Content/Python/tp_setup.py"
Перед запуском проект должен быть собран (класс /Script/TeniParizha.TPCityGenerator должен существовать).
"""

import unreal

MAT_DIR = "/Game/TeniParizha/Materials"
MAP_DIR = "/Game/Maps"
MAP_PATH = "/Game/Maps/Paris"

CITY_CLASS = "/Script/TeniParizha.TPCityGenerator"
ENV_CLASS = "/Script/TeniParizha.TPEnvironment"

# границы квартала в сантиметрах (построитель: x −20..160 м, z −20..140 м, дома до ~25 м высотой)
CITY_MIN = unreal.Vector(-2200.0, -2200.0, -300.0)
CITY_MAX = unreal.Vector(16200.0, 14200.0, 4500.0)
# запасная точка старта (центр площади у фонтана), если генератор не ответит
DEFAULT_START = unreal.Vector(6000.0, 5400.0, 6.0)

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary


def log(msg):
    print("[tp_setup] " + msg)


def warn(msg):
    unreal.log_warning("[tp_setup] " + msg)


# ---------------------------------------------------------------- папки

def ensure_dir(path):
    if EAL.does_directory_exist(path):
        log("Папка уже есть: " + path)
    else:
        EAL.make_directory(path)
        log("Создана папка: " + path)


# ---------------------------------------------------------------- текстуры по умолчанию

def sampler_type_for(tex):
    """тот же выбор, что делает движок (UMaterialExpressionTextureBase::GetSamplerTypeForTexture)"""
    T = unreal.TextureCompressionSettings
    S = unreal.MaterialSamplerType
    cs = tex.get_editor_property("compression_settings")
    srgb = tex.get_editor_property("srgb")
    if cs == T.TC_NORMALMAP:
        return S.SAMPLERTYPE_NORMAL
    if cs == T.TC_MASKS:
        return S.SAMPLERTYPE_MASKS
    if cs == T.TC_GRAYSCALE:
        return S.SAMPLERTYPE_GRAYSCALE if srgb else S.SAMPLERTYPE_LINEAR_GRAYSCALE
    if cs == T.TC_ALPHA:
        return S.SAMPLERTYPE_ALPHA
    return S.SAMPLERTYPE_COLOR if srgb else S.SAMPLERTYPE_LINEAR_COLOR


def ensure_default_texture(name, sources, srgb, compression):
    """копия движковой текстуры с нужными настройками (sRGB и сжатие определяют тип сэмплера)"""
    path = MAT_DIR + "/" + name
    tex = None
    if EAL.does_asset_exist(path):
        tex = EAL.load_asset(path)
        log("Текстура по умолчанию уже есть: " + path)
    else:
        for src in sources:
            if EAL.does_asset_exist(src):
                tex = EAL.duplicate_asset(src, path)
                if tex:
                    log("Создана текстура по умолчанию " + path + " (копия " + src + ")")
                    break
    if tex is None:
        warn("Не удалось создать " + path + " — нет ни одной из движковых текстур: " + ", ".join(sources))
        return None
    changed = False
    if tex.get_editor_property("srgb") != srgb:
        tex.set_editor_property("srgb", srgb)
        changed = True
    if tex.get_editor_property("compression_settings") != compression:
        tex.set_editor_property("compression_settings", compression)
        changed = True
    try:
        if tex.get_editor_property("virtual_texture_streaming"):
            tex.set_editor_property("virtual_texture_streaming", False)
            changed = True
    except Exception:
        pass
    if changed:
        log("  настройки " + name + ": sRGB=" + str(srgb) + ", сжатие=" + str(compression))
    EAL.save_asset(path, only_if_is_dirty=False)
    return tex


# ---------------------------------------------------------------- материалы

def get_or_create_material(name):
    path = MAT_DIR + "/" + name
    if EAL.does_asset_exist(path):
        mat = EAL.load_asset(path)
        MEL.delete_all_material_expressions(mat)
        log("Материал " + path + " уже есть — пересобираю граф")
    else:
        factory = unreal.MaterialFactoryNew()
        mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT_DIR, unreal.Material, factory)
        log("Создан материал " + path)
    return mat, path


def expr(mat, cls, x, y):
    return MEL.create_material_expression(mat, cls, x, y)


def connect(a, out_name, b, in_name):
    if not MEL.connect_material_expressions(a, out_name, b, in_name):
        warn("  не удалось соединить " + a.get_name() + "." + (out_name or "<выход>") + " → " + b.get_name() + "." + in_name)


def to_property(a, out_name, prop):
    if not MEL.connect_material_property(a, out_name, prop):
        warn("  не удалось подключить " + a.get_name() + "." + (out_name or "<выход>") + " к " + str(prop))


def texture_param(mat, name, tex, x, y, uv):
    node = expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    if tex is not None:
        node.set_editor_property("texture", tex)
        node.set_editor_property("sampler_type", sampler_type_for(tex))
    if not MEL.connect_material_expressions(uv, "", node, "UVs"):
        connect(uv, "", node, "Coordinates")   # другое имя того же входа
    return node


def build_base_material(name, masked, defaults):
    mat, path = get_or_create_material(name)
    P = unreal.MaterialProperty
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("two_sided", bool(masked))
    if masked:
        mat.set_editor_property("opacity_mask_clip_value", 0.33)

    # UV = TexCoord × UVScale
    tc = expr(mat, unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    uvs = expr(mat, unreal.MaterialExpressionScalarParameter, -1400, 120)
    uvs.set_editor_property("parameter_name", "UVScale")
    uvs.set_editor_property("default_value", 1.0)
    uv = expr(mat, unreal.MaterialExpressionMultiply, -1150, 40)
    connect(tc, "", uv, "A")
    connect(uvs, "", uv, "B")

    alb = texture_param(mat, "Albedo", defaults.get("Albedo"), -850, -300, uv)
    nrm = texture_param(mat, "Normal", defaults.get("Normal"), -850, 50, uv)
    orm = texture_param(mat, "ORM", defaults.get("ORM"), -850, 400, uv)

    # BaseColor = Albedo × VertexColor × Tint
    vc = expr(mat, unreal.MaterialExpressionVertexColor, -850, -520)
    tint = expr(mat, unreal.MaterialExpressionVectorParameter, -550, -520)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))
    m1 = expr(mat, unreal.MaterialExpressionMultiply, -500, -300)
    connect(alb, "RGB", m1, "A")
    connect(vc, "", m1, "B")
    m2 = expr(mat, unreal.MaterialExpressionMultiply, -250, -300)
    connect(m1, "", m2, "A")
    connect(tint, "", m2, "B")
    to_property(m2, "", P.MP_BASE_COLOR)

    to_property(nrm, "RGB", P.MP_NORMAL)
    to_property(orm, "R", P.MP_AMBIENT_OCCLUSION)
    to_property(orm, "G", P.MP_ROUGHNESS)
    to_property(orm, "B", P.MP_METALLIC)
    if masked:
        to_property(alb, "A", P.MP_OPACITY_MASK)

    MEL.recompile_material(mat)
    EAL.save_asset(path, only_if_is_dirty=False)
    log("  " + name + ": " + ("Masked, Two Sided" if masked else "Opaque") + ", параметры Albedo / Normal / ORM / Tint / UVScale — сохранён")
    return mat


def build_emissive_material(name):
    mat, path = get_or_create_material(name)
    P = unreal.MaterialProperty
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("two_sided", False)
    color = expr(mat, unreal.MaterialExpressionVectorParameter, -700, 0)
    color.set_editor_property("parameter_name", "Color")
    color.set_editor_property("default_value", unreal.LinearColor(1.0, 0.65, 0.3, 1.0))
    inten = expr(mat, unreal.MaterialExpressionScalarParameter, -700, 200)
    inten.set_editor_property("parameter_name", "Intensity")
    inten.set_editor_property("default_value", 6.0)
    mul = expr(mat, unreal.MaterialExpressionMultiply, -400, 100)
    connect(color, "", mul, "A")
    connect(inten, "", mul, "B")
    to_property(color, "", P.MP_BASE_COLOR)
    to_property(mul, "", P.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_asset(path, only_if_is_dirty=False)
    log("  " + name + ": Emissive = Color × Intensity — сохранён")
    return mat


# ---------------------------------------------------------------- уровень

def subsystem(cls):
    try:
        return unreal.get_editor_subsystem(cls)
    except Exception:
        return None


def current_world_path():
    ues = subsystem(unreal.UnrealEditorSubsystem)
    world = ues.get_editor_world() if ues else unreal.EditorLevelLibrary.get_editor_world()
    return world.get_path_name() if world else ""


def open_or_create_map():
    les = subsystem(unreal.LevelEditorSubsystem)
    if current_world_path().startswith(MAP_PATH + "."):
        log("Уровень " + MAP_PATH + " уже открыт")
        return True
    if EAL.does_asset_exist(MAP_PATH):
        log("Открываю уровень " + MAP_PATH)
        ok = les.load_level(MAP_PATH) if les else unreal.EditorLevelLibrary.load_level(MAP_PATH)
    else:
        log("Создаю новый пустой уровень " + MAP_PATH)
        ok = les.new_level(MAP_PATH) if les else unreal.EditorLevelLibrary.new_level(MAP_PATH)
    if not ok:
        warn("Не удалось открыть/создать уровень " + MAP_PATH)
    return bool(ok)


def all_actors():
    eas = subsystem(unreal.EditorActorSubsystem)
    return eas.get_all_level_actors() if eas else unreal.EditorLevelLibrary.get_all_level_actors()


def find_actors(class_name):
    return [a for a in all_actors() if a.get_class().get_name() == class_name]


def spawn(cls, location, rotation=None):
    rot = rotation if rotation is not None else unreal.Rotator(0.0, 0.0, 0.0)
    eas = subsystem(unreal.EditorActorSubsystem)
    if eas:
        return eas.spawn_actor_from_class(cls, location, rot)
    return unreal.EditorLevelLibrary.spawn_actor_from_class(cls, location, rot)


def ensure_actor(class_path, class_name, label, location):
    found = find_actors(class_name)
    if found:
        log(label + ": уже есть на уровне (" + found[0].get_actor_label() + ")")
        return found[0]
    cls = unreal.load_class(None, class_path)
    if cls is None:
        warn("Класс " + class_path + " не найден — соберите проект (Development Editor) в Visual Studio и перезапустите редактор")
        return None
    actor = spawn(cls, location)
    if actor:
        actor.set_actor_label(label)
        log(label + ": добавлен на уровень")
    else:
        warn(label + ": не удалось создать")
    return actor


def ensure_navmesh_bounds():
    center = unreal.Vector((CITY_MIN.x + CITY_MAX.x) * 0.5, (CITY_MIN.y + CITY_MAX.y) * 0.5, (CITY_MIN.z + CITY_MAX.z) * 0.5)
    half = unreal.Vector((CITY_MAX.x - CITY_MIN.x) * 0.5, (CITY_MAX.y - CITY_MIN.y) * 0.5, (CITY_MAX.z - CITY_MIN.z) * 0.5)
    found = find_actors("NavMeshBoundsVolume")
    if found:
        vol = found[0]
        log("NavMeshBoundsVolume: уже есть — подгоняю под квартал")
    else:
        vol = spawn(unreal.NavMeshBoundsVolume, center)
        if not vol:
            warn("NavMeshBoundsVolume: не удалось создать — добавьте вручную (Place Actors → Volumes) на весь квартал")
            return
        vol.set_actor_label("TP_NavBounds")
        log("NavMeshBoundsVolume: добавлен")
    vol.set_actor_location(center, False, False)
    # текущий размер кисти при единичном масштабе (обычно куб 200 см)
    vol.set_actor_scale3d(unreal.Vector(1.0, 1.0, 1.0))
    origin, extent = vol.get_actor_bounds(False)
    if extent.x < 1.0 or extent.y < 1.0 or extent.z < 1.0:
        warn("У NavMeshBoundsVolume нет кисти — растяните его вручную на весь квартал: центр "
             + str(center) + ", полуразмеры " + str(half))
        return
    vol.set_actor_scale3d(unreal.Vector(half.x / extent.x, half.y / extent.y, half.z / extent.z))
    log("  центр " + str(center) + ", размер " + str(half.x * 2) + " × " + str(half.y * 2) + " × " + str(half.z * 2) + " см")


def ensure_player_start(city):
    start = DEFAULT_START
    if city is not None:
        try:
            start = city.get_player_start()
        except Exception:
            pass
    loc = unreal.Vector(start.x, start.y, start.z + 100.0)
    rot = unreal.Rotator(0.0, 0.0, 0.0)   # лицом вдоль +X — к фонтану
    found = find_actors("PlayerStart")
    if found:
        ps = found[0]
        ps.set_actor_location(loc, False, False)
        ps.set_actor_rotation(rot, False)
        log("PlayerStart: уже есть — перенесён к фонтану " + str(loc))
    else:
        ps = spawn(unreal.PlayerStart, loc, rot)
        if ps:
            ps.set_actor_label("TP_PlayerStart")
            log("PlayerStart: добавлен у фонтана " + str(loc))
        else:
            warn("PlayerStart: не удалось создать")


def save_all():
    les = subsystem(unreal.LevelEditorSubsystem)
    try:
        ok = les.save_current_level() if les else unreal.EditorLevelLibrary.save_current_level()
        log("Уровень сохранён" if ok else "Уровень не сохранён (возможно, нет изменений)")
    except Exception as e:
        warn("Ошибка сохранения уровня: " + str(e))
    EAL.save_directory("/Game/TeniParizha", only_if_is_dirty=True, recursive=True)
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception:
        pass
    log("Ассеты сохранены")


# ---------------------------------------------------------------- главное

def main():
    log("=== Тени Парижа: настройка проекта ===")
    log("1/4. Папки")
    ensure_dir("/Game/TeniParizha")
    ensure_dir(MAT_DIR)
    ensure_dir(MAP_DIR)

    log("2/4. Материалы")
    T = unreal.TextureCompressionSettings
    defaults = {
        "Albedo": ensure_default_texture("T_TPDefault_Albedo",
                                         ["/Engine/EngineResources/DefaultTexture", "/Engine/EngineResources/WhiteSquareTexture"],
                                         True, T.TC_DEFAULT),
        "Normal": ensure_default_texture("T_TPDefault_Normal",
                                         ["/Engine/EngineMaterials/DefaultNormal", "/Engine/EngineMaterials/FlatNormal"],
                                         False, T.TC_NORMALMAP),
        "ORM": ensure_default_texture("T_TPDefault_ORM",
                                      ["/Engine/EngineResources/DefaultTexture", "/Engine/EngineResources/WhiteSquareTexture"],
                                      False, T.TC_MASKS),
    }
    build_base_material("M_TPBase", False, defaults)
    build_base_material("M_TPBaseMasked", True, defaults)
    build_emissive_material("M_TPEmissive")

    log("3/4. Уровень " + MAP_PATH)
    if not open_or_create_map():
        return
    city = ensure_actor(CITY_CLASS, "TPCityGenerator", "TP_City", unreal.Vector(0.0, 0.0, 0.0))
    if city is not None:
        try:
            log("Строю квартал заново с новыми материалами (Generate)…")
            city.generate()
            log("  точек прогулок: " + str(len(city.get_walk_points())) + ", фонарей: " + str(len(city.get_lamp_points()))
                + ", телег с сеном: " + str(len(city.get_hay_points())))
        except Exception as e:
            warn("Generate не вызван: " + str(e))
    ensure_actor(ENV_CLASS, "TPEnvironment", "TP_Environment", unreal.Vector(0.0, 0.0, 0.0))
    ensure_navmesh_bounds()
    ensure_player_start(city)

    log("4/4. Сохранение")
    save_all()
    log("=== Готово. Уровень " + MAP_PATH + " — нажмите Play. ===")


main()
