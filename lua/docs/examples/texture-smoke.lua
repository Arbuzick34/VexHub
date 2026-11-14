local texture, error_message = vexhub.draw.load_texture("ct.png")
vexhub.storage.set("texture_loaded", texture ~= nil)
vexhub.storage.set("texture_error", error_message)

vexhub.events.on("frame", function()
    if texture then
        vexhub.draw.image(texture, 20, 20, 48, 48, 255, 255, 255, 255)
    end
end)
