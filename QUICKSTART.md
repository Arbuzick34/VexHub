# VexHub — как скачать и запустить

## Вариант 1. Просто запустить (Windows 10/11 x64)

1. На странице репозитория нажми **Code → Download ZIP** (или `git clone https://github.com/Arbuzick34/VexHub.git`).
2. Распакуй архив в любую папку.
3. Открой `loader\dist\`.
4. Запусти **`VexHubLoader.exe`**.
5. Нажми **«Запустить VexHub»**.
6. В игре: `Insert` — меню, `End` — выгрузить клиент.

Что лежит в `loader\dist\`:

| Файл | Что это |
|---|---|
| `VexHubLoader.exe` | лоадер (397 КБ), сам находит и запускает клиент |
| `vexhub.exe` | клиент VexHub 1.2.3 (5.45 МБ), уже рядом с лоадером |

Если `vexhub.exe` удалить или переложить — лоадер покажет красный баннер «vexhub.exe не найден». Клиент должен лежать **в той же папке**, что и `VexHubLoader.exe`, либо путь прописывается в `%TEMP%\vexhub\loader.json`:

```json
{
  "Language": "ru",
  "VexHubPath": "C:\\Games\\VexHub\\vexhub.exe"
}
```

SHA-256 клиента (бинарник собран из исходников этого репозитория):

```
A08A402204ADA7B80C4599AA2E3C99CC89F4607CF22AB7EE74C41A0CB54D2740
```

> Defender может ругаться на `vexhub.exe` как на `PUA:Win32/GameHack` — это
> стандартная реакция на игровые читы, а не вирус. Если доверяешь файлу —
> добавь папку в исключения. Лоадер и исходники проверены, детектов нет.
>
> Если Windows пишет «Приложение заблокировано» после скачивания ZIP —
> правый клик по exe → **Свойства** → **Разблокировать** → ОК.

## Вариант 2. Собрать лоадер самому

Нужен [.NET 8 SDK](https://dotnet.microsoft.com/download/dotnet/8.0):

```powershell
cd VexHub-main\loader
dotnet build VexHubLoader.csproj -c Release
dotnet publish VexHubLoader.csproj -c Release -r win-x64 --self-contained false `
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o dist
```

Готовый файл — `loader\dist\VexHubLoader.exe`. `vexhub.exe` рядом надо положить
отдельно (или скачать из [релиза VexHub](https://github.com/Arbuzick34/VexHub/archive/refs/heads/main.zip)).

## Rage bot + Anti-Aim

Модуль лежит в `rage/`. Это исходники для внутреннего чита: чистый солвер
стрельбы и генератор анти-эйма, без оффсетов и SDK — их поставляет база,
в которую модуль встраивается (`CreateMove`-хук). Подробности — в
[`rage/README.md`](rage/README.md).

Сборка и тесты (нужны Visual Studio Build Tools с C++):

```powershell
cd rage
.\build.ps1
```

Ожидаемый результат:

```
rage solver tests
...
102 checks, 0 failures
```

## Структура репозитория

```
VexHub/
├─ loader/      нативный WPF-лоадер (наш)
├─ rage/        rage bot + anti-aim (наш)
├─ src/         клиент VexHub
├─ lua/         Lua API и примеры
├─ configs/     конфиги
├─ website/     сайт/макет
└─ .github/     CI и ассеты
```

Клиент VexHub — сторонний проект под Apache-2.0, атрибуция сохранена в
`LICENSE` и `NOTICE`. Лоадер и `rage/` написаны для этого репозитория.
