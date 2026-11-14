-- This file is also used by the repository's manual runtime smoke test.
assert(vexhub.api.major == 1)
assert(debug == nil)

vexhub.ui.toggle("enabled", "Smoke overlay", true)
vexhub.ui.slider("x", "X", 30, 0, 400, 1)
vexhub.ui.select("style", "Style", {"Text", "Circle"}, 0)
vexhub.ui.input("label", "Label", "Lua smoke")
vexhub.ui.color("accent", "Accent", {r=120, g=200, b=255, a=255})
vexhub.ui.keybind("key", "Test key", 0)
vexhub.ui.separator("test_section", "Runtime test")
vexhub.ui.button("reset", "Reset counter")

local unused = vexhub.events.on("unused_test_event", function() end)
assert(vexhub.events.off("unused_test_event", unused))
assert(vexhub.config.schema() ~= nil)

local ticks = vexhub.storage.get("ticks") or 0

vexhub.events.on("load", function()
    vexhub.storage.set("loaded", true)
    vexhub.storage.set("loads", (vexhub.storage.get("loads") or 0) + 1)
end)

vexhub.events.on("tick", function()
    ticks = ticks + 1
    if ticks % 60 == 0 then vexhub.storage.set("ticks", ticks) end
    if vexhub.ui.consume("reset") then ticks = 0 end
end)

vexhub.events.on("frame", function(frame)
    assert(frame.sequence ~= nil)
    if not vexhub.ui.get("enabled") then return end
    local x = vexhub.ui.get("x")
    if vexhub.ui.get("style") == 0 then
        local color = vexhub.ui.get("accent")
        vexhub.draw.text(x, 120, vexhub.ui.get("label") .. ": " .. #frame.players,
            color.r, color.g, color.b, color.a)
    else
        vexhub.draw.circle(x, 120, 10, 120, 200, 255, 255, 2)
    end
end)
