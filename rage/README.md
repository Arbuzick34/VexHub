# Rage bot + Anti-Aim

Модуль для CS2-чита: солвер стрельбы (rage bot) и генератор анти-эйма.
Код чистый, без SDK и без оффсетов — он не знает, как устроена игра, и не
трогает её напрямую. Всё общение с игрой идёт через тонкий интерфейс
`rage::World`, который реализует хост (внутренний чит с `CreateMove`-хуком).

```
rage/
├─ include/rage/
│  ├─ Math.h        Vec3, Angles, Basis, RayObb, VectorAngles
│  ├─ World.h       снимок мира: игроки, хитбоксы, трассы, TraceToExit
│  ├─ Config.h      RageConfig
│  ├─ AntiAim.h     AntiAimConfig, AntiAimInput/Output, AntiAim
│  └─ RageBot.h     AimPoint, ShotPlan, RageCommand, Resolver, RageBot
├─ src/
│  ├─ RageBot.cpp
│  └─ AntiAim.cpp
├─ tests/test_main.cpp   фейковый мир: стена + три хитбокса, 102 проверки
└─ build.ps1             сборка MSVC + запуск тестов
```

## Сборка и тесты

`cl.exe` в PATH нет, скрипт сам находит `vcvars64.bat` в Build Tools:

```powershell
cd VexHub-main\rage
.\build.ps1              # собрать и прогнать
.\build.ps1 -NoRun       # только собрать
.\build.ps1 -Clean       # пересобрать с нуля
```

Ожидаемый хвост:

```
rage solver tests

math
damage model
aim points
...
anti-aim

102 checks, 0 failures
```

## Как это работает

### Rage bot

`RageBot::Update(world, cfg, aaCfg, tick)` возвращает `RageCommand` — что
записать в `CUserCmd` на этом тике:

| поле | смысл |
|---|---|
| `writeAngles` / `angles` | писать ли углы и какие |
| `fire` | жать `IN_ATTACK` |
| `scope` | жать `IN_ATTACK2` (снайперка без зума — не стреляем) |
| `stop` | обнулить `forwardmove`/`sidemove` (авто-стоп) |
| `choke` | подавить пакет на тике выстрела (hide shots) |
| `plan` | выбранная точка: цель, группа хитбокса, урон, hitchance |

Порядок внутри:

1. **Resolver.** Для каждого игрока запоминается последний yaw; после
   промахов хитбоксы цели поворачиваются вокруг `origin` на угол из
   brute-force-таблицы `0, ±30, ±60, ±90, ±120, ±150, 180`. Попадание
   сбрасывает счётчик. Поворот делается в `Resolver::RotatePlayer`, поэтому
   трассы идут уже по «предполагаемому» телу.
2. **Точки прицела.** Центр каждого хитбокса + до 8 мультипоинтов по осям
   `right`/`up` (масштаб `multipointScale`). Радиус точки — остаток хитбокса
   от смещения; совсем краевые точки выбрасываются.
3. **Трасса.** `TraceLine(eye, point)`. Если попали в нужного игрока —
   план готов. Если в стену — `TraceToExit`, затем трасса с выхода; до
   `min(cfg.maxPenetrations, weapon.maxPenetrations)` стен. Попадание в
   другого игрока обрывает план.
4. **Урон.** `damage × множитель хитбокса × falloff(distance) ×
   penetrationScale^стены`, затем броня (`ApplyArmor`). Урон режется текущим
   HP цели.
5. **Hitchance.** Гауссова ошибка угла с сигмой `weapon.spread`; вероятность
   попасть в конус радиуса точки на дистанции:

   ```
   cone = atan2(radius, distance)
   hc   = (1 - exp(-cone² / (2·spread²))) · 100
   ```

   Это приближение, а не перебор сидов, как в платных читах. Зато
   предсказуемо и тестируемо.
6. **Выбор плана.** Фильтры `fov`, `autoWall`, `hitchance`, `minDamage`.
   Скоринг зависит от `priority` (`Damage`, `Hitchance`, `Distance`) плюс
   бонус за голову и штраф за стену/краевую точку.

### Anti-Aim

`AntiAim::Update(cfg, input)` — чистая функция плюс небольшое состояние
(спин, таймер LBY). Возвращает углы для `CUserCmd`, `bodyYaw`, `desync`,
`sendPacket`.

- **Pitch:** `Down` (+89), `Up` (−89), `Zero`, `Jitter` (±`pitchJitter`
  каждый тик), `FakeDown` (вниз/ноль через тик).
- **Yaw:** `Jitter` (±`yawJitter`), `Spin` (`spinSpeed`°/тик),
  `Sideways` (±90), `Backwards` (+180), `Static`, `Random` (детерминированный
  PRNG от тика).
- **База:** текущий view, направление движения или последнее движение.
- **Desync:** `±desyncLimit` (по умолчанию 58°), сторона меняется каждый тик;
  `breakLby` переворачивает сторону каждые 16 тиков стоя на земле.
- **hideShots:** на тике выстрела `sendPacket = false` — хост решает, как
  чокать пакеты в своей базе.

В `RageCommand` анти-эйм уже применён: если план не стреляет — в `angles`
лежат AA-углы, если стреляет с `hideShots` — углы прицела (выстрел должен
уйти в цель), а `choke = true` сигналит подавление пакета.

## Подключение к внутреннему читу (Osiris-подобная база)

Нужен только `CreateMove`-хук: анти-эйм физически невозможен снаружи, потому
что требует записи углов в `CUserCmd` до отправки на сервер.

### 1. Реализовать `World`

```cpp
class GameWorld final : public rage::World
{
public:
    explicit GameWorld(CUserCmd* cmd) : cmd_(cmd) {}

    int LocalTeam() const override { return local->m_iTeamNum(); }
    int LocalIndex() const override { return local->GetRefEHandle().GetEntryIndex(); }
    Vec3 EyePosition() const override { return local->GetEyePosition(); }
    Vec3 LocalVelocity() const override { return local->m_vecVelocity(); }
    bool LocalOnGround() const override { return local->m_fFlags() & FL_ONGROUND; }
    Angles ViewAngles() const override { return FromQAngle(cmd_->viewangles); }

    WeaponInfo Weapon() const override
    {
        WeaponInfo w;
        w.damage           = weapon->GetDamage();
        w.rangeModifier    = weapon->GetRangeModifier();
        w.armorRatio       = weapon->GetArmorRatio();
        w.spread           = weapon->GetInaccuracy() * kRadToDeg; // текущая, не базовая
        w.maxPenetrations  = weapon->GetPenetrationCount();
        w.hasScope         = weapon->IsSniper();
        w.scoped           = local->m_bIsScoped();
        return w;
    }

    std::vector<PlayerView> Players() const override; // скелет → OBB хитбоксы
    TraceHit TraceLine(const Vec3& from, const Vec3& to) const override;
    bool TraceToExit(Vec3& exit, const Vec3& start, const Vec3& dir,
                     const TraceHit& enter) const override;
};
```

`TraceLine` — это `TraceShape` с `Ray_t`; в `TraceHit` кладутся индекс
сущности, hitgroup и точка. `TraceToExit` — стандартный движковый цикл:
шаг 4 юнита, до 90 юнитов, два прохода (внутри/снаружи), иначе `false`.

### 2. CreateMove

```cpp
void Hooks::CreateMove(CHookContext* ctx)
{
    CUserCmd* cmd = ctx->cmd;
    if (!cmd || !cmd->command_number) return;

    GameWorld world(cmd);
    rage::RageCommand out = g_rage.Update(world, g_rageCfg, g_aaCfg, cmd->tick_count);

    if (out.writeAngles)
        cmd->viewangles = ToQAngle(out.angles.pitch, out.angles.yaw, out.angles.roll);

    if (out.fire)      cmd->buttons |=  IN_ATTACK;
    else               cmd->buttons &= ~IN_ATTACK;
    if (out.scope)     cmd->buttons |=  IN_ATTACK2;
    if (out.stop)    { cmd->forwardmove = 0; cmd->sidemove = 0; }
    if (out.choke)   { ctx->sendPacket = false; }   // имя флага зависит от базы

    ctx->viewAngles = cmd->viewangles;              // иначе база перезапишет
}
```

В CS2 `CreateMove` часто вызывается через `Hook_CreateMove` / `CHookContext`
(см. Osiris `Hook_CreateMove`), а углы живут в `cmd->viewangles` и
`cmd->csgoUserCmd`. Если база пишет `CViewSetup` отдельно — синхронизируйте
и его, иначе анти-эйм будет только на клиенте.

### 3. Попадания и промахи для резолвера

```cpp
// player_hurt: attacker == local → попали
g_rage.GetResolver().OnHit(victimIndex);

// weapon_fire без player_hurt на этом тике → промах
g_rage.GetResolver().OnMiss(targetIndex);
```

Резолвер — это brute-force по углу, а не телепатия: на каждый промах он
сдвигает хитбоксы на следующий шаг. Против статичного десинка он сходится за
1–2 выстрела, против меняющего сторону каждый тик — статистически, как любой
публичный resolver.

## Настройки

`RageConfig` (`Config.h`):

| поле | по умолчанию | смысл |
|---|---|---|
| `enabled` | true | включён ли солвер |
| `autoFire` | true | стрелять при готовом плане |
| `autoWall` | true | разрешить стрельбу через penetrable |
| `autoScope` | true | зум перед выстрелом |
| `autoStop` | true | тормозить перед выстрелом |
| `fov` | 180 | ограничение угла на цель |
| `minDamage` / `minDamageLow` | 30 / 10 | порог урона; второй — для цели с HP ≤ `lowHealthThreshold` |
| `hitchance` | 55 | минимальный процент |
| `multipoint` / `multipointScale` | true / 0.55 | точки по краям хитбокса |
| `penetration` / `maxPenetrations` / `penetrationScale` | true / 3 / 0.5 | пробитие стен |
| `resolver` / `bruteForce` | true / true | резолвер |
| `preferHead` | true | голова первой в списке |
| `priority` | Damage | чем ранжировать планы |

`AntiAimConfig` (`AntiAim.h`): `pitch`, `yaw`, `base`, `pitchDown`,
`pitchJitter`, `yawJitter`, `spinSpeed`, `staticYaw`, `roll`, `desync`,
`desyncLimit`, `jitterBody`, `breakLby`, `hideShots`, `onGroundOnly`.

## Ограничения, которые нужно знать

- **Оффсетов и SDK здесь нет.** Модуль не знает ни одной сигнатуры CS2 и не
  привязан к билду игры. Оффсеты, `CGameEntitySystem`, схемы и скелет
  поставляет хост; после каждого апдейта CS2 их надо обновлять.
- **Публичные исходники устарели.** Axion (`T1GxR/Axion-CS2-RAGE-CHEAT`)
  последний раз обновлялся 30.04.2024; leerware — май 2025. Идеи и структуру
  оттуда брать можно, оффсеты — нет. Солвер здесь написан с нуля, поэтому от
  билда игры не зависит.
- **Anti-Aim только для внутреннего чита.** Внешний процесс не может писать
  `CUserCmd`; максимум — двигать мышь (`SendInput`), это не анти-эйм.
- **CS2 и roll.** Сервер в CS2 не реплицирует roll так, как это было в CS:GO;
  в большинстве конфигураций он клампится или игнорируется. Держите `roll = 0`,
  пока не проверите на своём билде.
- **Subtick.** В CS2 выстрел и движение интерполируются по subtick-входам.
  Углы, записанные в `CUserCmd`, применяются к основному тику; для
  точного совмещения с выстрелом некоторые базы дополнительно пишут
  `csgoUserCmd`-сабтики. Если стрельба «плывёт» на 1–2°, смотрите туда.
- **Hide shots.** `sendPacket = false` — это только сигнал. Реальное
  подавление пакетов зависит от базы (в Osiris — `SendDatagram`/`bSendPacket`
  в NetChannel-хуке). Не подавляйте пакеты дольше нескольких тиков: сервер
  начнёт экстраполировать и вас выдаст.
- **Урон и hitchance — модель.** Формулы откалиброваны под стандартные
  множители CS, но не являются дампом `CCSPlayer::TraceAttack`. Для
  соревновательной настройки подгоняйте `penetrationScale`, `armorRatio` и
  `spread` под свой билд/оружие.
