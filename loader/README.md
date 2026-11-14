# VexHub Loader

Native Windows shell for the **VexHub** external client. No key, no login screen: it finds
`vexhub.exe`, checks the game, starts the client and then gets out of the way. VexHub is an external
client, so nothing is injected into the game — the loader only starts the process.

The UI is a WPF port of `website/loader.html` — same palette, iconography and session flow.

## Requirements

- Windows 10/11 x64
- .NET 8 Desktop Runtime (`Microsoft.WindowsDesktop.App 8.x`) for the framework-dependent build

## Build

```powershell
dotnet build VexHubLoader.csproj -c Release
dotnet publish VexHubLoader.csproj -c Release -r win-x64 --self-contained false `
  -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -o dist
```

`dist/VexHubLoader.exe` is a single file; the WPF resources (font, icon, language dictionaries)
are embedded in the bundle.

## Use

1. Put `VexHubLoader.exe` in the same folder as `vexhub.exe` (or point `VexHubPath` at it).
2. Press **Launch VexHub**.

The loader walks four steps: locate the client binary, check the game, start/resume the client,
prepare the session. If `cs2.exe` is not running the game step shows a warning instead of failing —
VexHub waits for the game itself. Hotkeys inside the client: `Insert` opens the menu, `End` unloads.

Pressing **Launch VexHub** while an instance is already running reuses that process instead of
starting a second one. Once the session is active the button becomes **Close loader** — closing the
loader leaves VexHub running.

## Session flow

| Step | What it does | Failure |
| --- | --- | --- |
| Locate VexHub client | `VexHubPath`, loader folder, working dir, `%TEMP%\vexhub` | `vexhub.exe` missing |
| Check Counter-Strike 2 | looks for `cs2.exe` | warning only |
| Start VexHub | reuses a running instance or starts `vexhub.exe` | start error |
| Prepare session | finishes and switches to the active state | — |

## Settings

Stored in `%TEMP%\vexhub\loader.json`:

```json
{
  "Language": "en",
  "VexHubPath": "C:\\Games\\VexHub\\vexhub.exe"
}
```

- `Language` — `en` or `ru` (also follows the system UI language on first run; the title-bar
  `RU`/`EN` button persists the choice).
- `VexHubPath` — explicit client path, checked before the default locations.

## Developer flags

```powershell
VexHubLoader.exe --shot out.png --state idle|busy|active|warn|error --lang en|ru
```

`--shot` renders the window to a PNG and exits — used to check the layout without a desktop.
