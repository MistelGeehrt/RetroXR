// RetroXR — API publique du module VR (MIT)
//
// API C volontairement minimale : aucun type OpenXR ni OpenGL ne sort d'ici, pour que
// libultraship (C++) et les jeux (C) puissent l'appeler sans dépendre du SDK OpenXR.
// Sans ENABLE_XR, toutes les fonctions sont des no-ops inline (le jeu reste 100 % identique).
//
// Cycle par frame (appelé par Fast3dWindow::DrawAndRunGraphicsCommands) :
//   xr_begin_frame()  -> true si une frame VR doit être rendue
//   pour eye in 0..1 : xr_begin_eye(eye) ... rendu de la display list ... xr_end_eye(eye)
//   xr_end_frame()
// Les matrices par œil sont lues par Interpreter::GfxSpMatrix via xr_get_eye_projection /
// xr_get_eye_view, en convention N64 (row-major, vecteurs lignes : v' = v * M).

#ifndef RETROXR_XR_H
#define RETROXR_XR_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    XR_EYE_LEFT = 0,
    XR_EYE_RIGHT = 1,
    XR_EYE_COUNT = 2
} xr_eye_t;

typedef struct {
    float x, y, z;
} xr_vec3_t;

typedef struct {
    float x, y, z, w; // quaternion
} xr_quat_t;

typedef struct {
    xr_vec3_t position;    // mètres, espace LOCAL (origine = position au démarrage)
    xr_quat_t orientation;
    bool valid;
} xr_pose_t;

typedef struct {
    uint32_t width, height;   // taille recommandée par le runtime pour un œil
    float units_per_meter;    // échelle du monde du jeu (ex. SM64 ~ 100-150 u/m)
    float world_scale;        // 1.0 = taille réelle ; <1 = diorama
    bool  hmd_present;
    bool  session_running;
} xr_info_t;

#ifdef ENABLE_XR

// Initialisation (après création du contexte OpenGL par SDL). Retourne false sans casque/runtime :
// le jeu continue alors en mode écran, sans erreur.
bool xr_init(void);
void xr_shutdown(void);
bool xr_is_active(void);
void xr_set_enabled(bool enabled);

// Frame
bool xr_begin_frame(void);
void xr_begin_eye(xr_eye_t eye);      // bind du framebuffer de l'œil, viewport
void xr_end_eye(xr_eye_t eye);        // blit vers la swapchain OpenXR
void xr_end_frame(void);

// Matrices (row-major 4x4, convention N64), valides entre begin_frame et end_frame.
// near/far : ceux du jeu, pour conserver son depth range.
void xr_get_eye_projection(xr_eye_t eye, float near_plane, float far_plane, float out[4][4]);
// Vue : rotation + translation du casque (translation en unités du jeu), pour l'œil donné.
void xr_get_eye_view(xr_eye_t eye, float out[4][4]);
// Même chose sans translation (ciel, skybox, éléments « à l'infini »).
void xr_get_eye_view_rotation_only(xr_eye_t eye, float out[4][4]);

// Poses brutes (pour la V2 : caméra du jeu, manettes)
xr_pose_t xr_get_head_pose(void);
xr_pose_t xr_get_controller_pose(int hand); // 0 = gauche, 1 = droite

// Entrées manettes (V1 : exposées pour le futur PhysicalDeviceType::OpenXR)
// Masque de boutons et sticks normalisés [-1, 1].
#define XR_BTN_A        (1u << 0)
#define XR_BTN_B        (1u << 1)
#define XR_BTN_X        (1u << 2)
#define XR_BTN_Y        (1u << 3)
#define XR_BTN_LTRIGGER (1u << 4)
#define XR_BTN_RTRIGGER (1u << 5)
#define XR_BTN_LGRIP    (1u << 6)
#define XR_BTN_RGRIP    (1u << 7)
#define XR_BTN_LSTICK   (1u << 8)
#define XR_BTN_RSTICK   (1u << 9)
#define XR_BTN_MENU     (1u << 10)
uint32_t xr_get_buttons(void);
void     xr_get_stick(int hand, float* out_x, float* out_y);
void     xr_haptic(int hand, float amplitude, float seconds);

// Réglages (chargés depuis profiles/<jeu>/profile.json par le jeu ou les cvars)
void xr_get_info(xr_info_t* out);
void xr_set_units_per_meter(float u);
// Part de la résolution recommandée par le casque (0.3 - 1.5). À appeler AVANT xr_init.
void xr_set_resolution_scale(float s);
void xr_set_world_scale(float s);
void xr_recenter(void);

// Interface 2D (HUD ortho) : distance du plan virtuel en mètres et sa hauteur apparente
void xr_set_hud_plane(float distance_m, float height_m);
void xr_get_hud_projection(xr_eye_t eye, float out[4][4]); // remplace une projection ortho

// Point d'entrée unique pour libultraship (Interpreter::GfxSpMatrix) : remplace en place une
// projection du jeu (perspective -> vue+projection de l'œil ; ortho -> plan HUD). false = inconnue.
bool xr_override_projection(float m[4][4], xr_eye_t eye);

// Copie la texture couleur OpenGL du framebuffer du jeu vers l'image de swapchain de l'œil
// (à appeler entre xr_begin_eye et xr_end_eye, après le rendu).
void xr_submit_eye_texture(xr_eye_t eye, uint32_t gl_texture, uint32_t tex_width, uint32_t tex_height);

#else // ---- sans ENABLE_XR : tout est inerte ----

static inline bool xr_init(void) { return false; }
static inline void xr_shutdown(void) {}
static inline bool xr_is_active(void) { return false; }
static inline void xr_set_enabled(bool enabled) { (void)enabled; }
static inline bool xr_begin_frame(void) { return false; }
static inline void xr_begin_eye(xr_eye_t eye) { (void)eye; }
static inline void xr_end_eye(xr_eye_t eye) { (void)eye; }
static inline void xr_end_frame(void) {}
static inline void xr_get_eye_projection(xr_eye_t e, float n, float f, float o[4][4]) { (void)e; (void)n; (void)f; (void)o; }
static inline void xr_get_eye_view(xr_eye_t e, float o[4][4]) { (void)e; (void)o; }
static inline void xr_get_eye_view_rotation_only(xr_eye_t e, float o[4][4]) { (void)e; (void)o; }
static inline xr_pose_t xr_get_head_pose(void) { xr_pose_t p = {{0,0,0},{0,0,0,1},false}; return p; }
static inline xr_pose_t xr_get_controller_pose(int h) { (void)h; xr_pose_t p = {{0,0,0},{0,0,0,1},false}; return p; }
static inline uint32_t xr_get_buttons(void) { return 0; }
static inline void xr_get_stick(int h, float* x, float* y) { (void)h; if (x) *x = 0; if (y) *y = 0; }
static inline void xr_haptic(int h, float a, float s) { (void)h; (void)a; (void)s; }
static inline void xr_get_info(xr_info_t* o) { if (o) { o->width = 0; o->height = 0; o->units_per_meter = 100.f; o->world_scale = 1.f; o->hmd_present = false; o->session_running = false; } }
static inline void xr_set_units_per_meter(float u) { (void)u; }
static inline void xr_set_resolution_scale(float s) { (void)s; }
static inline void xr_set_world_scale(float s) { (void)s; }
static inline void xr_recenter(void) {}
static inline void xr_set_hud_plane(float d, float h) { (void)d; (void)h; }
static inline void xr_get_hud_projection(xr_eye_t e, float o[4][4]) { (void)e; (void)o; }
static inline bool xr_override_projection(float m[4][4], xr_eye_t e) { (void)m; (void)e; return false; }
static inline void xr_submit_eye_texture(xr_eye_t e, uint32_t t, uint32_t w, uint32_t h) { (void)e; (void)t; (void)w; (void)h; }

#endif // ENABLE_XR

#ifdef __cplusplus
}
#endif

#endif // RETROXR_XR_H
