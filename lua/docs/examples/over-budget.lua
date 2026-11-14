-- Deliberately exceeds the callback execution budget.
vexhub.events.on("tick", function()
    while true do end
end)
