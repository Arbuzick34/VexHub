# Input API

```lua
vexhub.input.is_down(0x20)
local jump = vexhub.input.binding("jump")
vexhub.input.key(0x20, true)
vexhub.input.key(0x20, false)
vexhub.input.tap(0x20)
vexhub.input.mouse_move(4, -2)
vexhub.input.mouse_button("primary", true)
vexhub.input.mouse_button("primary", false)
```

Имена binding: `forward`, `back`, `left`, `right`, `walk`, `duck`, `jump`,
`attack`, `attack2`.

Ввод доходит только до CS2. Нажатие, tap, движение мыши и клик отклоняются, если
открыто меню, игра не принимает ввод, набирается строка чата или окно CS2 не на
переднем плане в этот момент; для клика курсор ещё должен быть над окном игры.
`is_down` возвращает `false`, пока на переднем плане другая программа. Скрипт
может нажимать буквы, цифры, F1-F12, Space, Tab, Shift, Ctrl, стрелки, цифровой
блок и знаки препинания; Windows, Alt, Escape, Enter, клавиша консоли, клавиши
приложений, браузера и медиа и клавиши меню и выхода VexHub запрещены. Runtime
отслеживает ownership каждого скрипта и гарантированно отпускает его состояния при
остановке.
