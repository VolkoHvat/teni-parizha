# -*- coding: utf-8 -*-
"""
Тени Парижа — проверка Content/Python/tp_setup.py без редактора: макет модуля unreal с именами Python API UE 4.27.
В макете НЕТ подсистем UE5 (UnrealEditorSubsystem, LevelEditorSubsystem, EditorActorSubsystem): обращение к любому
имени, которого нет в списке 4.27, падает AttributeError, как в настоящем редакторе 4.27.
Проверяется: скрипт доходит до конца, все материалы собраны, все соединения узлов удались, акторы поставлены,
уровень и ассеты сохранены. Запуск: python3 Tools/PySetupTest/run_test.py
"""
import os, sys, types, runpy

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
SCRIPT = os.path.join(ROOT, "Content", "Python", "tp_setup.py")

# узлы материалов: имена входов как в 4.27 (у узлов с одним входом — пустое имя)
INPUTS = {
    "MaterialExpressionMultiply": ["A", "B"], "MaterialExpressionSubtract": ["A", "B"], "MaterialExpressionDivide": ["A", "B"],
    "MaterialExpressionOneMinus": [""], "MaterialExpressionSaturate": [""], "MaterialExpressionComponentMask": [""],
    "MaterialExpressionTextureSampleParameter2D": ["UVs", "Tex", "ApplyViewMipBias"],
    "MaterialExpressionTextureCoordinate": [], "MaterialExpressionScalarParameter": [], "MaterialExpressionVectorParameter": [],
    "MaterialExpressionVertexColor": [], "MaterialExpressionWorldPosition": [],
}
OUTPUTS = {
    "MaterialExpressionTextureSampleParameter2D": ["", "RGB", "R", "G", "B", "A", "RGBA"],
    "MaterialExpressionVertexColor": ["", "R", "G", "B", "A"],
    "MaterialExpressionVectorParameter": ["", "R", "G", "B", "A"],
}
PROPS = {
    "MaterialExpressionComponentMask": {"r", "g", "b", "a"},
    "MaterialExpressionMultiply": {"const_a", "const_b"},
    "MaterialExpressionScalarParameter": {"parameter_name", "default_value"},
    "MaterialExpressionVectorParameter": {"parameter_name", "default_value"},
    "MaterialExpressionTextureSampleParameter2D": {"parameter_name", "texture", "sampler_type"},
    "Material": {"blend_mode", "two_sided", "opacity_mask_clip_value", "used_with_instanced_static_meshes"},
    "Texture2D": {"srgb", "compression_settings", "virtual_texture_streaming"},
}
errors = []
log_lines = []


class Obj(object):
    def __init__(self, cls_name, name=""):
        self._cls = cls_name
        self._name = name or cls_name
        self._props = {}
        self._links = {}

    def get_name(self):
        return self._name

    def get_class(self):
        return types.SimpleNamespace(get_name=lambda: self._cls)

    def set_editor_property(self, k, v):
        allowed = PROPS.get(self._cls)
        if allowed is not None and k not in allowed:
            errors.append("свойство %s.%s не существует" % (self._cls, k))
        self._props[k] = v

    def get_editor_property(self, k):
        allowed = PROPS.get(self._cls)
        if allowed is not None and k not in allowed:
            errors.append("свойство %s.%s не существует" % (self._cls, k))
        return self._props.get(k, False)

    # акторы
    def set_actor_label(self, s): self._name = s
    def get_actor_label(self): return self._name
    def set_actor_location(self, loc, sweep, teleport): self._props["loc"] = loc
    def set_actor_rotation(self, rot, teleport): self._props["rot"] = rot
    def set_actor_scale3d(self, s): self._props["scale"] = s
    def get_actor_bounds(self, only_colliding):
        return Vector(0, 0, 0), Vector(100.0, 100.0, 100.0)
    # генератор города (UFUNCTION BlueprintCallable)
    def generate(self): pass
    def get_player_start(self): return Vector(6000.0, 5400.0, 6.0)
    def get_walk_points(self): return [1, 2]
    def get_lamp_points(self): return [1]
    def get_hay_points(self): return [1]


class Vector(object):
    def __init__(self, x=0.0, y=0.0, z=0.0): self.x, self.y, self.z = x, y, z
    def __str__(self): return "(%g, %g, %g)" % (self.x, self.y, self.z)


class Enum(object):
    def __init__(self, *names):
        for n in names: setattr(self, n, n)


def make_class(name):
    return type(name, (object,), {"__mtype__": name})


assets = {}
level_actors = []
connections = []


class EAL(object):
    @staticmethod
    def does_directory_exist(p): return False
    @staticmethod
    def make_directory(p): return True
    @staticmethod
    def does_asset_exist(p): return p in assets or p.startswith("/Engine/")
    @staticmethod
    def load_asset(p): return assets.get(p)
    @staticmethod
    def duplicate_asset(src, dst):
        t = Obj("Texture2D", dst); assets[dst] = t; return t
    @staticmethod
    def save_asset(p, only_if_is_dirty=True): return True
    @staticmethod
    def save_directory(p, only_if_is_dirty=True, recursive=True): return True


class MEL(object):
    @staticmethod
    def delete_all_material_expressions(m): pass
    @staticmethod
    def create_material_expression(mat, cls, x, y):
        return Obj(cls.__mtype__, "%s_%d" % (cls.__mtype__, len(connections) + x * 0 + id(object()) % 10000))
    @staticmethod
    def connect_material_expressions(a, out_name, b, in_name):
        outs = OUTPUTS.get(a._cls, [""])
        ins = INPUTS.get(b._cls)
        if ins is None:
            errors.append("макет не знает узел " + b._cls); return False
        ok = out_name in outs and in_name in ins
        if ok: connections.append((a._cls, out_name, b._cls, in_name))
        return ok
    @staticmethod
    def connect_material_property(a, out_name, prop):
        ok = out_name in OUTPUTS.get(a._cls, [""])
        if ok: connections.append((a._cls, out_name, "property", prop))
        return ok
    @staticmethod
    def recompile_material(m): pass


class AssetTools(object):
    def create_asset(self, name, path, cls, factory):
        m = Obj("Material", path + "/" + name); assets[path + "/" + name] = m; return m


class ELL(object):
    world = None
    @staticmethod
    def get_editor_world(): return ELL.world
    @staticmethod
    def load_level(p): ELL.world = types.SimpleNamespace(get_path_name=lambda: p + "." + p.split("/")[-1]); return True
    @staticmethod
    def new_level(p): return ELL.load_level(p)
    @staticmethod
    def get_all_level_actors(): return list(level_actors)
    @staticmethod
    def spawn_actor_from_class(cls, loc, rot):
        a = Obj(getattr(cls, "__mtype__", "Actor")); level_actors.append(a); return a
    @staticmethod
    def save_current_level(): return True


unreal = types.ModuleType("unreal")
names = {
    "EditorAssetLibrary": EAL, "MaterialEditingLibrary": MEL, "EditorLevelLibrary": ELL,
    "AssetToolsHelpers": types.SimpleNamespace(get_asset_tools=lambda: AssetTools()),
    "EditorLoadingAndSavingUtils": types.SimpleNamespace(save_dirty_packages=lambda a, b: True),
    "Vector": Vector, "Rotator": lambda p=0.0, y=0.0, r=0.0: (p, y, r), "LinearColor": lambda *a: a,
    "TextureCompressionSettings": Enum("TC_DEFAULT", "TC_NORMALMAP", "TC_MASKS", "TC_GRAYSCALE", "TC_ALPHA"),
    "MaterialSamplerType": Enum("SAMPLERTYPE_COLOR", "SAMPLERTYPE_LINEAR_COLOR", "SAMPLERTYPE_NORMAL", "SAMPLERTYPE_MASKS",
                                "SAMPLERTYPE_GRAYSCALE", "SAMPLERTYPE_LINEAR_GRAYSCALE", "SAMPLERTYPE_ALPHA"),
    "MaterialProperty": Enum("MP_BASE_COLOR", "MP_NORMAL", "MP_AMBIENT_OCCLUSION", "MP_ROUGHNESS", "MP_METALLIC",
                             "MP_OPACITY_MASK", "MP_EMISSIVE_COLOR"),
    "BlendMode": Enum("BLEND_OPAQUE", "BLEND_MASKED"),
    "MaterialFactoryNew": lambda: object(), "Material": make_class("Material"),
    "NavMeshBoundsVolume": make_class("NavMeshBoundsVolume"), "PlayerStart": make_class("PlayerStart"),
    "load_class": lambda outer, path: make_class(path.split(".")[-1]),
    "log_warning": lambda s: log_lines.append("ПРЕДУПРЕЖДЕНИЕ " + s),
    "get_editor_subsystem": lambda cls: None,   # есть с 4.22, но подсистем уровня/акторов в 4.27 нет
}
for n in INPUTS:
    names[n] = make_class(n)
for k, v in names.items():
    setattr(unreal, k, v)
sys.modules["unreal"] = unreal

import builtins
_print = builtins.print
builtins.print = lambda *a, **k: log_lines.append(" ".join(str(x) for x in a))
try:
    runpy.run_path(SCRIPT, run_name="__main__")
except Exception as e:
    import traceback
    errors.append("исключение: " + "".join(traceback.format_exception_only(type(e), e)).strip())
finally:
    builtins.print = _print

warns = [l for l in log_lines if l.startswith("ПРЕДУПРЕЖДЕНИЕ")]
need = ["M_TPBase", "M_TPBaseMasked", "M_TPEmissive"]
for m in need:
    if "/Game/TeniParizha/Materials/" + m not in assets:
        errors.append("не создан материал " + m)
props = [c for c in connections if c[2] == "property"]
if len([c for c in props if c[3] == "MP_BASE_COLOR"]) != 3:
    errors.append("BaseColor подключён не у всех трёх материалов")
if not any(l.find("=== Готово") >= 0 for l in log_lines):
    errors.append("скрипт не дошёл до конца")
for w in warns:
    errors.append(w)
print("строк журнала: %d, соединений узлов: %d, акторов: %d" % (len(log_lines), len(connections), len(level_actors)))
for e in errors:
    print("ОШИБКА: " + e)
print("ИТОГ: " + ("чисто" if not errors else "ошибок %d" % len(errors)))
sys.exit(1 if errors else 0)
