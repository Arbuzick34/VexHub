# API reference 1.0

## Core

- `vexhub.api.major`, `vexhub.api.minor`
- `vexhub.api.script_dir` — absolute directory of the active package
- `vexhub.api.data_dir` — absolute private writable data directory of the active package
- `vexhub.log(text)`
- `vexhub.events.on(name, callback) -> token`
- `vexhub.events.off(name [, token]) -> bool`

## Game

- `vexhub.game.snapshot()`
- `vexhub.game.radar_snapshot()`
- `vexhub.game.world_to_screen(vec3)`
- `vexhub.game.trace_ray(start, finish)`
- `vexhub.game.bomb_damage(position [, site, eye_angles, ducked])`
- `vexhub.game.predict_grenade(origin, velocity, kind [, remaining_lifetime])`
- `vexhub.game.radar_overview(map_name)`
- `vexhub.game.penetration_damage(start, finish [, target_handle])`

## Config and storage

- `vexhub.config.get(path)`
- `vexhub.config.set(path, value)`
- `vexhub.config.patch(table)`
- `vexhub.config.snapshot()`
- `vexhub.config.schema()`
- `vexhub.storage.get/set/remove(key [, value])`

## UI

- `vexhub.ui.text(id, label, value)`
- `vexhub.ui.button(id, label [, action_text])`
- `vexhub.ui.toggle(id, label, default)`
- `vexhub.ui.slider(id, label, default, min, max, step)`
- `vexhub.ui.select(id, label, options, default_index)`
- `vexhub.ui.input(id, label, default_text)`
- `vexhub.ui.color(id, label, {r,g,b,a})`
- `vexhub.ui.keybind(id, label, default_vk)`
- `vexhub.ui.separator(id, label)`
- `vexhub.ui.get(id)`, `vexhub.ui.consume(id)`

## External helpers

- `vexhub.helpers.start(relative_ps1, args) -> bool [, error]`
- `vexhub.helpers.copy_text(text) -> bool`

Only the built-in web radar sees `vexhub.helpers` (see the sandbox section of the README); for every other script
the table is absent. `start` accepts only a `.ps1` file inside the active package directory and returns immediately
after creating the external process.

## Bundled assets

- `vexhub.assets.weapon_font() -> string`

Возвращает байты того же TTF, который VexHub использует для штатных иконок
оружия и гранат. Скрипт может сохранить строку в бинарном режиме и использовать
font в локальном UI/Web UI без системных шрифтов и внешней загрузки.

## Draw

- `line(x1,y1,x2,y2,r,g,b,a[,thickness])`
- `rect/filled_rect(x,y,w,h,r,g,b,a[,thickness])`
- `circle/filled_circle(x,y,radius,r,g,b,a[,thickness])`
- `text(x,y,text,r,g,b,a)`
- `world_line(vec3,vec3,r,g,b,a[,thickness])`
- `world_text(vec3,text,r,g,b,a)`
- `load_texture(relative_path) -> texture_id | nil,error`
- `image(texture_id,x,y,w,h[,r,g,b,a])`
- `panel(x,y,w,h,title[,accent_r,g,b,a])`

## Input

- `is_down(vk)`, `binding(action)`, `key(vk,down)`, `tap(vk)`
- `mouse_move(dx,dy)`, `mouse_button("primary"|"secondary",down)`
