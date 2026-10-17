-- flatverse / fv_wrist_hud.lua (MIT) — ÉTAPE 1 du wrist-HUD (feuille de route, semaines 11-14)
--
-- Objectif final : afficher la mini-map / l'inventaire sur un panneau ancré au poignet gauche.
-- Cette première version fait le travail "sans rendu" :
--   * trouve les widgets UMG visibles et les liste dans l'overlay,
--   * laisse l'utilisateur en choisir un et le masquer/afficher (test de faisabilité),
--   * envoie au plugin C++ l'ordre de capture ("capture_widget:<nom>") via un custom event.
-- Le rendu du quad au poignet sera fait côté plugin C++ (texture -> quad dans la pose du contrôleur).

local api = uevr.api
local widgets = {}
local selected = 1
local last_refresh = 0

local function refresh()
    widgets = {}
    local klass = api:find_uobject("Class /Script/UMG.UserWidget")
    if klass == nil then return end
    local ok, objs = pcall(function() return UEVR_UObjectHook.get_objects_by_class(klass, false) end)
    if not ok or objs == nil then return end
    for _, w in ipairs(objs) do
        local okv, visible = pcall(function() return w:IsVisible() end)
        if okv and visible then
            widgets[#widgets + 1] = w
        end
        if #widgets >= 64 then break end
    end
end

uevr.sdk.callbacks.on_draw_ui(function()
    if not imgui.collapsing_header("flatverse / wrist-HUD (étape 1)") then return end

    if imgui.button("Rafraîchir les widgets visibles") or (os.clock() - last_refresh) > 30 then
        refresh()
        last_refresh = os.clock()
    end
    imgui.text(string.format("%d widgets visibles", #widgets))

    for i, w in ipairs(widgets) do
        local okn, name = pcall(function() return w:get_full_name() end)
        name = okn and name or ("widget#" .. i)
        if imgui.button((i == selected and "> " or "  ") .. name) then
            selected = i
        end
    end

    local w = widgets[selected]
    if w ~= nil then
        if imgui.button("Masquer / afficher") then
            pcall(function()
                if w:IsVisible() then w:SetVisibility(1) else w:SetVisibility(0) end -- 1 = Collapsed, 0 = Visible
            end)
        end
        imgui.same_line()
        if imgui.button("Capturer vers le poignet (plugin)") then
            local okn, name = pcall(function() return w:get_full_name() end)
            api:dispatch_custom_event("flatverse", "capture_widget:" .. (okn and name or "?"))
        end
    end
end)

print("[flatverse] fv_wrist_hud.lua chargé")
