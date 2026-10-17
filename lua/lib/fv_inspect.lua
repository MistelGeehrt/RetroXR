-- flatverse / fv_inspect.lua (MIT)
-- Panneau de diagnostic dans l'overlay UEVR (touche Insert) :
-- état du runtime, pose du casque et des contrôleurs, pawn et caméra courants.
-- Sert à vérifier en 10 secondes qu'un jeu "parle" bien à UEVR avant tout profil.

local api = uevr.api
local vr = uevr.params.vr

local function fmt_pose(index)
    if index == nil or index < 0 then return "n/a" end
    local ok, pose = pcall(function() return vr:get_pose(index) end)
    if not ok or pose == nil then return "err" end
    local p, r = pose.position, pose.rotation
    return string.format("pos(%.2f %.2f %.2f) rot(%.2f %.2f %.2f %.2f)",
        p.x, p.y, p.z, r.x, r.y, r.z, r.w)
end

local function name_or(obj, fallback)
    if obj == nil then return fallback end
    local ok, n = pcall(function() return obj:get_full_name() end)
    return ok and n or fallback
end

uevr.sdk.callbacks.on_draw_ui(function()
    if not imgui.collapsing_header("flatverse / inspect") then return end

    imgui.text(string.format("runtime: %s | hmd_active: %s",
        vr:is_openxr() and "OpenXR" or (vr:is_openvr() and "OpenVR" or "none"),
        tostring(vr:is_hmd_active())))

    imgui.text("HMD   " .. fmt_pose(vr:get_hmd_index()))
    imgui.text("Left  " .. fmt_pose(vr:get_left_controller_index()))
    imgui.text("Right " .. fmt_pose(vr:get_right_controller_index()))

    local pawn = api:get_local_pawn(0)
    local pc = api:get_player_controller(0)
    imgui.text("Pawn: " .. name_or(pawn, "nil"))
    imgui.text("PlayerController: " .. name_or(pc, "nil"))

    if pc ~= nil then
        local ok, cm = pcall(function() return pc.PlayerCameraManager end)
        imgui.text("CameraManager: " .. (ok and name_or(cm, "nil") or "err"))
    end
end)

print("[flatverse] fv_inspect.lua chargé")
