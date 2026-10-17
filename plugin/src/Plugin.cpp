// flatverse — plugin UEVR (MIT)
//
// Rôle de ce premier plugin (MVP, semaines 1-4 de la feuille de route) :
//   1. Prouver que la DLL se charge dans un jeu UE via UEVR.
//   2. Exposer la télémétrie VR (pose HMD + contrôleurs) à l'outillage hors-process
//      en écrivant un fichier JSON "heartbeat" dans le dossier persistant d'UEVR.
//   3. Servir de point d'ancrage pour le wrist-HUD (capture de widget -> quad au poignet),
//      qui viendra dans un second temps (voir docs/ROADMAP.md, semaines 11-14).
//
// Rien ici ne touche DirectX ni la mémoire du jeu "à la main" : tout passe par l'API UEVR.

#include <Windows.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

#include "uevr/Plugin.hpp"

using namespace uevr;

namespace {

constexpr const char* kPluginName = "flatverse";
constexpr const char* kPluginVersion = "0.1.0";

struct PoseSnapshot {
    UEVR_Vector3f position{};
    UEVR_Quaternionf rotation{};
    bool valid{false};
};

PoseSnapshot snapshot(UEVR_TrackedDeviceIndex index) {
    PoseSnapshot s{};
    if (index == UEVR_TrackedDeviceIndex(-1)) {
        return s;
    }
    const auto pose = API::VR::get_pose(index);
    s.position = pose.position;
    s.rotation = pose.rotation;
    s.valid = true;
    return s;
}

std::string pose_to_json(const char* name, const PoseSnapshot& p) {
    char buf[320];
    if (!p.valid) {
        std::snprintf(buf, sizeof(buf), "\"%s\":null", name);
        return buf;
    }
    std::snprintf(buf, sizeof(buf),
        "\"%s\":{\"pos\":[%.4f,%.4f,%.4f],\"rot\":[%.4f,%.4f,%.4f,%.4f]}",
        name,
        p.position.x, p.position.y, p.position.z,
        p.rotation.x, p.rotation.y, p.rotation.z, p.rotation.w);
    return buf;
}

} // namespace

class FlatversePlugin : public uevr::Plugin {
public:
    FlatversePlugin() = default;

    void on_dllmain() override {}

    void on_initialize() override {
        API::get()->log_info("[%s] v%s chargé", kPluginName, kPluginVersion);

        try {
            m_heartbeat_path = API::get()->get_persistent_dir(L"flatverse_heartbeat.json");
            API::get()->log_info("[%s] heartbeat -> %ls", kPluginName, m_heartbeat_path.c_str());
        } catch (...) {
            API::get()->log_error("[%s] impossible de résoudre le dossier persistant", kPluginName);
        }
    }

    // Tick moteur : on écrit la télémétrie ~4 fois par seconde.
    // Le fichier est lu par tools/ (validator, futur bridge MR) — jamais par le jeu.
    void on_post_engine_tick(API::UGameEngine* /*engine*/, float /*delta*/) override {
        using clock = std::chrono::steady_clock;
        const auto now = clock::now();
        if (now - m_last_write < std::chrono::milliseconds(250)) {
            return;
        }
        m_last_write = now;

        if (!API::VR::is_runtime_ready()) {
            return;
        }

        const auto hmd   = snapshot(API::VR::get_hmd_index());
        const auto left  = snapshot(API::VR::get_left_controller_index());
        const auto right = snapshot(API::VR::get_right_controller_index());

        const char* runtime = API::VR::is_openxr() ? "openxr" : (API::VR::is_openvr() ? "openvr" : "none");

        std::string json = "{";
        json += "\"plugin\":\"" + std::string(kPluginName) + "\",";
        json += "\"version\":\"" + std::string(kPluginVersion) + "\",";
        json += "\"runtime\":\"" + std::string(runtime) + "\",";
        json += std::string("\"hmd_active\":") + (API::VR::is_hmd_active() ? "true" : "false") + ",";
        json += pose_to_json("hmd", hmd) + ",";
        json += pose_to_json("left", left) + ",";
        json += pose_to_json("right", right);
        json += "}\n";

        write_atomic(json);
    }

    // Les scripts Lua peuvent nous parler via uevr.params.functions.dispatch_custom_event("flatverse", "...")
    void on_custom_event(const char* event_name, const char* event_data) override {
        if (std::string_view{event_name} != kPluginName) {
            return;
        }
        API::get()->log_info("[%s] event: %s", kPluginName, event_data);
        // TODO(wrist-hud): "capture_widget:<name>" -> copier le widget UMG dans une texture.
    }

private:
    void write_atomic(const std::string& content) {
        if (m_heartbeat_path.empty()) {
            return;
        }
        std::filesystem::path tmp = m_heartbeat_path;
        tmp += L".tmp";
        {
            std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
            if (!out) {
                return;
            }
            out << content;
        }
        MoveFileExW(tmp.c_str(), m_heartbeat_path.c_str(), MOVEFILE_REPLACE_EXISTING);
    }

    std::filesystem::path m_heartbeat_path{};
    std::chrono::steady_clock::time_point m_last_write{};
};

// Instanciation globale : enregistre les callbacks auprès d'UEVR (voir Plugin.hpp).
std::unique_ptr<FlatversePlugin> g_plugin{new FlatversePlugin()};
