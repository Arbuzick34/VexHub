# Script UI и drawing

Контролы скрипта отображаются в его popup внутри `Misc -> Lua API`.

```lua
vexhub.ui.text("status", "Status", "Waiting")
vexhub.ui.button("reset", "Reset", "Apply")
vexhub.ui.toggle("enabled", "Enabled", true)
vexhub.ui.slider("size", "Size", 10, 1, 40, 0.5)
vexhub.ui.select("mode", "Mode", {"First", "Second"}, 0)
vexhub.ui.input("endpoint", "Helper pipe", [[\\.\pipe\my_helper]])
vexhub.ui.color("accent", "Accent", {r=128, g=90, b=255, a=255})
vexhub.ui.keybind("toggle_key", "Toggle key", 0x74)
vexhub.ui.separator("advanced", "Advanced")

local enabled = vexhub.ui.get("enabled")
if vexhub.ui.consume("reset") then vexhub.storage.remove("counter") end
```

Повторный вызов `ui.text` обновляет динамический текст. Значения toggle/slider/
select/input/color/keybind сохраняются на время жизни скрипта и читаются через
`get`. Элементы используют штатные контролы VexHub и наследуют DPI/style.

## Drawing

Внутри `frame` можно вызывать:

- `line`, `rect`, `filled_rect`;
- `circle`, `filled_circle`;
- `text`;
- `world_line`, `world_text`.
- `load_texture`, `image` для локальных PNG/JPEG/BMP.
- `panel` для отдельной стилизованной информационной панели overlay.

Цвет передаётся четырьмя каналами RGBA `0..255`. World-команды проектируются
актуальной camera matrix на render thread, поэтому движение камеры не получает
дополнительного Lua-lag.

```lua
vexhub.events.on("frame", function(frame)
    for _, p in ipairs(frame.players) do
        vexhub.draw.world_text(p.origin, p.name, 255, 255, 255, 255)
    end
end)
```

Информационное окно можно собрать без копирования стилей основного меню:

```lua
vexhub.draw.panel(24, 180, 220, 90, "Web radar", 128, 90, 255, 255)
vexhub.draw.text(34, 216, "Helper: connected", 210, 212, 220, 255)
```

Текстуры разрешены только внутри директории конкретного скрипта, ограничены
4096×4096, 16 MiB на файл и 64 ресурсами runtime. Декодирование и создание D3D11
ресурса выполняются лениво на render thread; Lua получает только opaque id.

```lua
local logo = vexhub.draw.load_texture("assets/logo.png")
vexhub.events.on("frame", function()
    if logo then
        vexhub.draw.image(logo, 30, 120, 64, 64, 255, 255, 255, 255)
    end
end)
```
