# Как собрать и запустить «Тени Парижа» (Windows)

Всё бесплатно.

## 1. Установить инструменты
1. **Epic Games Launcher** → вкладка Unreal Engine → Library → установить **Unreal Engine 5.7** (подойдёт и 5.4–5.6; при первом открытии проект предложит переключить версию). Нужно ~60–100 ГБ.
2. **Visual Studio 2022 Community** с нагрузками «Разработка игр на C++» и «Разработка классических приложений на C++» (в установщике отметить Windows 10/11 SDK и компоненты Unreal Engine).
3. **Git** (git-scm.com) — чтобы скачать проект: `git clone https://github.com/VolkoHvat/teni-parizha`.

Видеокарта: DirectX 12 (GeForce GTX 1070 / Radeon RX 5700 и новее). Для аппаратного Lumen — RTX 20xx / RX 6000 и новее; на старых картах движок сам переключится на программный режим.

## 2. Первая сборка
1. Правой кнопкой по `TeniParizha.uproject` → **Generate Visual Studio project files**.
2. Двойной щелчок по `TeniParizha.uproject` → «Would you like to rebuild TeniParizha?» → **Yes**. Сборка займёт несколько минут.
3. Если сборка упала: откройте `TeniParizha.sln` в Visual Studio → **Build → Build Solution** и пришлите мне окно **Error List** (или файл `Saved/Logs/TeniParizha.log`) — я исправлю.

## 3. Персонаж-манекен (один раз)
В редакторе: **Content Browser → Add (+) → Add Feature or Content Pack → Blueprint → Third Person → Add to Project**. Отсюда берутся меш героя, стражи, жителей и базовые анимации.

## 4. Материалы и уровень (один раз)
**Tools → Execute Python Script…** → выбрать `Content/Python/tp_setup.py`. Скрипт создаст материалы `/Game/TeniParizha/Materials/*`, уровень `/Game/Maps/Paris` с кварталом, окружением и навигацией. Если пункта нет — включите плагин **Python Editor Script Plugin** (Edit → Plugins) и перезапустите редактор.

Без этого шага игра тоже запустится (квартал и небо создаются сами в пустом уровне), но дома будут без текстур.

## 5. Играть
Кнопка **Play** (или Alt+P). Управление — F1 в игре:
- WASD — движение, мышь — камера
- Shift или ПКМ — свободный бег (забегать на стены, прыгать с крыш)
- Пробел — прыжок / перемахнуть / ухватиться
- Ctrl или C — красться; у края крыши — спуститься в вис
- Q — отпустить руки, R — разворот
- ЛКМ или F — удар, E — контратака, Alt — уклонение
- G — поговорить, Tab — альбом портретов, Esc/P — пауза

Геймпад Xbox: левый стик, правый стик, RT — свободный бег, A — прыжок, B — вниз, X — удар, Y — контратака, RB — уклонение.

## Что пока не готово
- Анимации лазания и виса: логика работает, но манекен в висе стоит в обычной позе. Следующий шаг — бесплатные анимации Mixamo (climbing, hanging idle, braced hang shimmy, climbing up wall) и AnimBP на базе `TPTraversalAnimInstance`.
- Звук и музыка — будут перенесены из `legacy/teni_v1/game2/audio.c`.
