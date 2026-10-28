# Тесты поз (AnimTest)

Проверка `Source/TeniParizha/Public/TPAnimPoseMath.h` без Unreal Engine.

```
Tools/AnimTest/run.sh [каталог]   # по умолчанию Tools/AnimTest/out
```

- `shim/CoreMinimal.h` — рабочая замена FVector/FQuat/FMath с формулами UE 4.27 (только то, что использует заголовок).
- `anim_test.cpp` — оси и повороты, двухзвенный IK (2000 случайных цепей), все позы по 480 фазам:
  нет NaN, длины костей, колени к pole и не назад, локти не вперёд, цели стоп и кистей, скачки положения и поворота,
  стык цикла; независимость от позы покоя (Т-поза рук, прямые ноги, случайные RestRot); переходы и пружина.
  Последняя строка — `ИТОГ: чисто` или `ИТОГ: есть ошибки`.
- `render_sheet.c` — лист поз в PNG рендером `legacy/lutece` без окна (EGL/Mesa): вид сбоку и 3/4.
  Копии листов: `Docs/previews/anim_poses_side.png`, `Docs/previews/anim_poses_34.png`.
