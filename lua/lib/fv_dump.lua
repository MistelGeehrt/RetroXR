-- flatverse / fv_dump.lua (MIT)
-- Dump JSON des classes UObject utiles pour générer un profil :
-- joueur (Pawn), PlayerController, PlayerCameraManager, widgets UMG, composants caméra.
--
-- Installation : copier dans <config UEVR>/<NomDuJeu>/scripts/
-- Sortie       : <config UEVR>/<NomDuJeu>/scripts/data/flatverse_dump.json
--                (lu ensuite par tools/uobject-dump/summarize.py)
--
-- Déclencheur : touche F9 en jeu (ou automatique 10 s après le premier niveau chargé).

local api = uevr.api
local MAX_OBJECTS_PER_CLASS = 200  -- garde-fou : un jeu peut avoir des milliers de widgets

local DUMP_DONE = false
local FIRST_LEVEL_AT = nil

local function safe(fn, ...)
    local ok, res = pcall(fn, ...)
    if ok then return res end
    return nil
end

-- Remonte la chaîne d'héritage d'un UClass -> liste de noms
local function class_chain(klass)
    local chain = {}
    local c = klass
    local guard = 0
    while c ~= nil and guard < 32 do
        chain[#chain + 1] = c:get_full_name()
        c = safe(function() return c:get_super() end)
        guard = guard + 1
    end
    return chain
end

-- Propriétés déclarées par une UStruct (nom, classe de propriété, offset)
local function struct_properties(ustruct)
    local props = {}
    local p = safe(function() return ustruct:get_child_properties() end)
    local guard = 0
    while p ~= nil and guard < 2000 do
        local entry = {
            name = safe(function() return p:get_fname():to_string() end) or "?",
            type = safe(function() return p:get_class():get_fname():to_string() end) or "?",
            offset = safe(function() return p:get_offset() end),
        }
        props[#props + 1] = entry
        p = safe(function() return p:get_next() end)
        guard = guard + 1
    end
    return props
end

local function dump_objects_of(class_path, with_props)
    local klass = api:find_uobject(class_path)
    if klass == nil then
        return { class = class_path, found = false }
    end

    local objects = safe(function() return UEVR_UObjectHook.get_objects_by_class(klass, false) end) or {}
    local out = { class = class_path, found = true, count = #objects, instances = {} }

    for i, obj in ipairs(objects) do
        if i > MAX_OBJECTS_PER_CLASS then break end
        local inst = {
            name = obj:get_full_name(),
            chain = class_chain(obj:get_class()),
        }
        if with_props then
            inst.properties = struct_properties(obj:get_class())
        end
        out.instances[#out.instances + 1] = inst
    end
    return out
end

local function run_dump()
    if DUMP_DONE then return end
    DUMP_DONE = true
    print("[flatverse] dump en cours...")

    local pawn = api:get_local_pawn(0)
    local pc = api:get_player_controller(0)

    local result = {
        schema = "flatverse-dump/1",
        generated_by = "fv_dump.lua",
        runtime = {
            openxr = safe(function() return uevr.params.vr:is_openxr() end),
            openvr = safe(function() return uevr.params.vr:is_openvr() end),
            hmd_active = safe(function() return uevr.params.vr:is_hmd_active() end),
        },
        local_pawn = pawn and {
            name = pawn:get_full_name(),
            chain = class_chain(pawn:get_class()),
            properties = struct_properties(pawn:get_class()),
        } or nil,
        player_controller = pc and {
            name = pc:get_full_name(),
            chain = class_chain(pc:get_class()),
        } or nil,
        camera_manager = pc and safe(function()
            local cm = pc.PlayerCameraManager
            if cm == nil then return nil end
            return { name = cm:get_full_name(), chain = class_chain(cm:get_class()) }
        end) or nil,
        classes = {
            dump_objects_of("Class /Script/Engine.CameraComponent", false),
            dump_objects_of("Class /Script/Engine.SpringArmComponent", false),
            dump_objects_of("Class /Script/Engine.SkeletalMeshComponent", false),
            dump_objects_of("Class /Script/UMG.UserWidget", false),
            dump_objects_of("Class /Script/Engine.HUD", true),
        },
    }

    json.dump_file("data/flatverse_dump.json", result, 2)
    print("[flatverse] dump écrit : scripts/data/flatverse_dump.json")
end

uevr.sdk.callbacks.on_post_engine_tick(function(engine, delta)
    if DUMP_DONE then return end
    if FIRST_LEVEL_AT == nil then
        local pawn = api:get_local_pawn(0)
        if pawn ~= nil then
            FIRST_LEVEL_AT = os.clock()
            print("[flatverse] pawn détecté, dump auto dans 10 s (ou F9)")
        end
        return
    end
    if os.clock() - FIRST_LEVEL_AT > 10 then
        run_dump()
    end
end)

uevr.sdk.callbacks.on_script_reset(function()
    DUMP_DONE = false
    FIRST_LEVEL_AT = nil
end)

-- Bouton dans l'overlay UEVR (touche Insert pour l'ouvrir) : dump manuel / re-dump
uevr.sdk.callbacks.on_draw_ui(function()
    imgui.text("flatverse")
    if imgui.button(DUMP_DONE and "Re-dump UObjects" or "Dump UObjects") then
        DUMP_DONE = false
        run_dump()
    end
end)

print("[flatverse] fv_dump.lua chargé")
