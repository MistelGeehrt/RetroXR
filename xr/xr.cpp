// RetroXR — implémentation OpenXR (OpenGL / Win32) — MIT
//
// Cible V1 : Meta Quest 3 via Quest Link / Air Link / Virtual Desktop (runtime OpenXR du PC),
// backend OpenGL de libultraship (contexte créé par SDL2). Un seul fichier, pas de dépendance
// sur le reste de libultraship : il ne connaît que OpenGL, OpenXR et le log spdlog.
//
// Conventions :
//  - OpenXR : mètres, y haut, -z devant, matrices colonne.
//  - N64 / libultraship : row-major, vecteurs lignes (v' = v * M). Les matrices exposées ici
//    sont donc les TRANSPOSÉES des matrices OpenGL classiques.

#include "xr.h"

#ifdef ENABLE_XR

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <unknwn.h> // IUnknown : requis par openxr_platform.h quand WIN32_LEAN_AND_MEAN est défini
#include <GL/glew.h>
#include <GL/wglew.h>
#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_OPENGL
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#define RETROXR_HAVE_BACKEND 1
#else
#define RETROXR_HAVE_BACKEND 0
#endif

#ifdef RETROXR_USE_SPDLOG
#include <cstdio>
#include <spdlog/spdlog.h>
// spdlog formate avec {} : on formate d'abord en style printf, puis on transmet le texte tel quel.
#define XRLOG_FMT_(LEVEL, ...) do { char xrlog_buf_[512]; std::snprintf(xrlog_buf_, sizeof(xrlog_buf_), __VA_ARGS__); LEVEL("[RetroXR] {}", xrlog_buf_); } while (0)
#define XRLOG_INFO(...)  XRLOG_FMT_(SPDLOG_INFO, __VA_ARGS__)
#define XRLOG_WARN(...)  XRLOG_FMT_(SPDLOG_WARN, __VA_ARGS__)
#define XRLOG_ERROR(...) XRLOG_FMT_(SPDLOG_ERROR, __VA_ARGS__)
#else
#include <cstdio>
#define XRLOG_INFO(...)  do { std::printf("[RetroXR] " __VA_ARGS__); std::printf("\n"); } while (0)
#define XRLOG_WARN(...)  do { std::printf("[RetroXR][warn] " __VA_ARGS__); std::printf("\n"); } while (0)
#define XRLOG_ERROR(...) do { std::printf("[RetroXR][error] " __VA_ARGS__); std::printf("\n"); } while (0)
#endif

namespace {

// ------------------------------------------------------------------ maths (row-major, vecteurs lignes)

using Mat4 = float[4][4];

void mat_identity(Mat4 m) {
    std::memset(m, 0, sizeof(Mat4));
    m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
}

// r = a * b  (v * a * b : a appliquée d'abord)
void mat_mul(Mat4 r, const Mat4 a, const Mat4 b) {
    Mat4 t;
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
    std::memcpy(r, t, sizeof(Mat4));
}

void mat_translate(Mat4 m, float x, float y, float z) {
    mat_identity(m);
    m[3][0] = x; m[3][1] = y; m[3][2] = z;
}

void mat_scale(Mat4 m, float x, float y, float z) {
    mat_identity(m);
    m[0][0] = x; m[1][1] = y; m[2][2] = z;
}

// Rotation (row-major pour vecteurs lignes) à partir d'un quaternion
void mat_from_quat(Mat4 m, xr_quat_t q) {
    const float x = q.x, y = q.y, z = q.z, w = q.w;
    mat_identity(m);
    // Matrice colonne classique R, puis transposée pour la convention ligne
    const float R[3][3] = {
        { 1 - 2 * (y * y + z * z), 2 * (x * y - z * w),     2 * (x * z + y * w) },
        { 2 * (x * y + z * w),     1 - 2 * (x * x + z * z), 2 * (y * z - x * w) },
        { 2 * (x * z - y * w),     2 * (y * z + x * w),     1 - 2 * (x * x + y * y) },
    };
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m[i][j] = R[j][i];
}

xr_quat_t quat_conj(xr_quat_t q) { return { -q.x, -q.y, -q.z, q.w }; }

xr_quat_t quat_mul(xr_quat_t a, xr_quat_t b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

xr_vec3_t quat_rotate(xr_quat_t q, xr_vec3_t v) {
    xr_quat_t p = { v.x, v.y, v.z, 0 };
    xr_quat_t r = quat_mul(quat_mul(q, p), quat_conj(q));
    return { r.x, r.y, r.z };
}

// Quaternion « yaw seulement » (rotation autour de y) d'une orientation
xr_quat_t quat_yaw_only(xr_quat_t q) {
    xr_vec3_t f = quat_rotate(q, { 0, 0, -1 });
    const float yaw = std::atan2(-f.x, -f.z);
    return { 0, std::sin(yaw / 2), 0, std::cos(yaw / 2) };
}

// Projection perspective asymétrique (OpenXR fov en tangentes), row-major pour vecteurs lignes
void mat_projection_from_tangents(Mat4 m, float tL, float tR, float tD, float tU, float n, float f) {
    std::memset(m, 0, sizeof(Mat4));
    const float w = tR - tL, h = tU - tD;
    // Colonne-major GL : P[0][0]=2/w, P[0][2]=(tR+tL)/w, P[1][1]=2/h, P[1][2]=(tU+tD)/h,
    // P[2][2]=-(f+n)/(f-n), P[2][3]=-2fn/(f-n), P[3][2]=-1  → transposée :
    m[0][0] = 2.f / w;
    m[2][0] = (tR + tL) / w;
    m[1][1] = 2.f / h;
    m[2][1] = (tU + tD) / h;
    m[2][2] = -(f + n) / (f - n);
    m[3][2] = -2.f * f * n / (f - n);
    m[2][3] = -1.f;
}

// ------------------------------------------------------------------ état

struct EyeTarget {
#if RETROXR_HAVE_BACKEND
    XrSwapchain swapchain = XR_NULL_HANDLE;
    std::vector<XrSwapchainImageOpenGLKHR> images;
    std::vector<GLuint> fbos; // un FBO par image de swapchain, pour le blit
    uint32_t width = 0, height = 0;
    uint32_t acquiredIndex = 0;
    XrView view{};
#endif
};

struct State {
    bool enabled = true;      // souhait utilisateur (cvar)
    bool initialized = false; // xr_init réussi
    bool sessionRunning = false;
    bool frameOpen = false;
    bool shouldRender = false;

    float unitsPerMeter = 100.f;
    float worldScale = 1.f;
    float hudDistance = 2.0f;
    float hudHeight = 1.5f;

    xr_pose_t head{};
    xr_pose_t hands[2]{};
    xr_pose_t reference{ { 0, 0, 0 }, { 0, 0, 0, 1 }, true }; // recentrage (yaw + position)

    uint32_t buttons = 0;
    float sticks[2][2]{};

#if RETROXR_HAVE_BACKEND
    XrInstance instance = XR_NULL_HANDLE;
    XrSystemId systemId = XR_NULL_SYSTEM_ID;
    XrSession session = XR_NULL_HANDLE;
    XrSpace localSpace = XR_NULL_HANDLE;
    XrSessionState sessionState = XR_SESSION_STATE_UNKNOWN;
    XrFrameState frameState{ XR_TYPE_FRAME_STATE };
    XrViewConfigurationType viewConfig = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    std::array<EyeTarget, 2> eyes;
    std::array<XrCompositionLayerProjectionView, 2> projViews{};
    GLint savedViewport[4]{};
    GLint savedFbo = 0;
    GLuint blitFbo = 0; // FBO temporaire pour lire la texture du jeu

    // Actions
    XrActionSet actionSet = XR_NULL_HANDLE;
    XrAction aPose = XR_NULL_HANDLE, aStick = XR_NULL_HANDLE, aHaptic = XR_NULL_HANDLE;
    XrAction aA = XR_NULL_HANDLE, aB = XR_NULL_HANDLE, aX = XR_NULL_HANDLE, aY = XR_NULL_HANDLE;
    XrAction aTrigger = XR_NULL_HANDLE, aGrip = XR_NULL_HANDLE, aStickClick = XR_NULL_HANDLE, aMenu = XR_NULL_HANDLE;
    XrPath handPaths[2]{};
    XrSpace handSpaces[2]{};
#endif
};

State g;

#if RETROXR_HAVE_BACKEND

bool check(XrResult r, const char* what) {
    if (XR_SUCCEEDED(r)) return true;
    char buf[XR_MAX_RESULT_STRING_SIZE] = "?";
    if (g.instance != XR_NULL_HANDLE) xrResultToString(g.instance, r, buf);
    XRLOG_ERROR("%s : %s (%d)", what, buf, (int)r);
    return false;
}

xr_pose_t to_pose(const XrPosef& p, bool valid) {
    return { { p.position.x, p.position.y, p.position.z },
             { p.orientation.x, p.orientation.y, p.orientation.z, p.orientation.w }, valid };
}

// Pose relative à la référence de recentrage : p' = ref^-1 * p
xr_pose_t relative_to_reference(xr_pose_t p) {
    const xr_quat_t invRef = quat_conj(g.reference.orientation);
    xr_vec3_t d = { p.position.x - g.reference.position.x, p.position.y - g.reference.position.y,
                    p.position.z - g.reference.position.z };
    xr_pose_t r;
    r.position = quat_rotate(invRef, d);
    r.orientation = quat_mul(invRef, p.orientation);
    r.valid = p.valid;
    return r;
}

bool create_instance() {
    uint32_t count = 0;
    xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr);
    std::vector<XrExtensionProperties> exts(count, { XR_TYPE_EXTENSION_PROPERTIES });
    xrEnumerateInstanceExtensionProperties(nullptr, count, &count, exts.data());
    bool hasGL = false;
    for (auto& e : exts) if (std::strcmp(e.extensionName, XR_KHR_OPENGL_ENABLE_EXTENSION_NAME) == 0) hasGL = true;
    if (!hasGL) {
        XRLOG_ERROR("le runtime OpenXR actif n'expose pas %s", XR_KHR_OPENGL_ENABLE_EXTENSION_NAME);
        return false;
    }

    const char* enabled[] = { XR_KHR_OPENGL_ENABLE_EXTENSION_NAME };
    XrInstanceCreateInfo ci{ XR_TYPE_INSTANCE_CREATE_INFO };
    std::strncpy(ci.applicationInfo.applicationName, "RetroXR", XR_MAX_APPLICATION_NAME_SIZE - 1);
    ci.applicationInfo.applicationVersion = 1;
    std::strncpy(ci.applicationInfo.engineName, "libultraship", XR_MAX_ENGINE_NAME_SIZE - 1);
    ci.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
    ci.enabledExtensionCount = 1;
    ci.enabledExtensionNames = enabled;
    if (!check(xrCreateInstance(&ci, &g.instance), "xrCreateInstance")) return false;

    XrInstanceProperties ip{ XR_TYPE_INSTANCE_PROPERTIES };
    xrGetInstanceProperties(g.instance, &ip);
    XRLOG_INFO("runtime : %s", ip.runtimeName);

    XrSystemGetInfo sgi{ XR_TYPE_SYSTEM_GET_INFO };
    sgi.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    if (!check(xrGetSystem(g.instance, &sgi, &g.systemId), "xrGetSystem (casque branché ?)")) return false;

    XrSystemProperties sp{ XR_TYPE_SYSTEM_PROPERTIES };
    xrGetSystemProperties(g.instance, g.systemId, &sp);
    XRLOG_INFO("casque : %s", sp.systemName);
    return true;
}

bool create_session() {
    PFN_xrGetOpenGLGraphicsRequirementsKHR pfnReq = nullptr;
    xrGetInstanceProcAddr(g.instance, "xrGetOpenGLGraphicsRequirementsKHR", (PFN_xrVoidFunction*)&pfnReq);
    XrGraphicsRequirementsOpenGLKHR req{ XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR };
    if (!pfnReq || !check(pfnReq(g.instance, g.systemId, &req), "xrGetOpenGLGraphicsRequirementsKHR")) return false;

    XrGraphicsBindingOpenGLWin32KHR binding{ XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR };
    binding.hDC = wglGetCurrentDC();
    binding.hGLRC = wglGetCurrentContext();
    if (!binding.hDC || !binding.hGLRC) {
        XRLOG_ERROR("aucun contexte OpenGL courant : xr_init doit être appelé après la création de la fenêtre GL");
        return false;
    }

    XrSessionCreateInfo sci{ XR_TYPE_SESSION_CREATE_INFO };
    sci.next = &binding;
    sci.systemId = g.systemId;
    if (!check(xrCreateSession(g.instance, &sci, &g.session), "xrCreateSession")) return false;

    XrReferenceSpaceCreateInfo rsci{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
    rsci.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    rsci.poseInReferenceSpace.orientation.w = 1.f;
    if (!check(xrCreateReferenceSpace(g.session, &rsci, &g.localSpace), "xrCreateReferenceSpace")) return false;
    return true;
}

bool create_swapchains() {
    uint32_t viewCount = 0;
    xrEnumerateViewConfigurationViews(g.instance, g.systemId, g.viewConfig, 0, &viewCount, nullptr);
    std::vector<XrViewConfigurationView> views(viewCount, { XR_TYPE_VIEW_CONFIGURATION_VIEW });
    xrEnumerateViewConfigurationViews(g.instance, g.systemId, g.viewConfig, viewCount, &viewCount, views.data());
    if (viewCount < 2) { XRLOG_ERROR("configuration de vue non stéréo"); return false; }

    uint32_t fmtCount = 0;
    xrEnumerateSwapchainFormats(g.session, 0, &fmtCount, nullptr);
    std::vector<int64_t> formats(fmtCount);
    xrEnumerateSwapchainFormats(g.session, fmtCount, &fmtCount, formats.data());
    int64_t chosen = 0;
    for (int64_t want : { (int64_t)GL_SRGB8_ALPHA8, (int64_t)GL_RGBA8 }) {
        if (std::find(formats.begin(), formats.end(), want) != formats.end()) { chosen = want; break; }
    }
    if (!chosen) chosen = formats.empty() ? GL_RGBA8 : formats[0];

    for (int eye = 0; eye < 2; eye++) {
        auto& t = g.eyes[eye];
        t.width = views[eye].recommendedImageRectWidth;
        t.height = views[eye].recommendedImageRectHeight;

        XrSwapchainCreateInfo sci{ XR_TYPE_SWAPCHAIN_CREATE_INFO };
        sci.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_SAMPLED_BIT;
        sci.format = chosen;
        sci.sampleCount = 1;
        sci.width = t.width;
        sci.height = t.height;
        sci.faceCount = 1;
        sci.arraySize = 1;
        sci.mipCount = 1;
        if (!check(xrCreateSwapchain(g.session, &sci, &t.swapchain), "xrCreateSwapchain")) return false;

        uint32_t imgCount = 0;
        xrEnumerateSwapchainImages(t.swapchain, 0, &imgCount, nullptr);
        t.images.assign(imgCount, { XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR });
        xrEnumerateSwapchainImages(t.swapchain, imgCount, &imgCount, (XrSwapchainImageBaseHeader*)t.images.data());

        t.fbos.resize(imgCount);
        glGenFramebuffers((GLsizei)imgCount, t.fbos.data());
        for (uint32_t i = 0; i < imgCount; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, t.fbos[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t.images[i].image, 0);
        }
        XRLOG_INFO("œil %d : %ux%u, %u images", eye, t.width, t.height, imgCount);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glGenFramebuffers(1, &g.blitFbo);
    return true;
}

XrAction make_action(const char* name, XrActionType type, bool perHand) {
    XrActionCreateInfo aci{ XR_TYPE_ACTION_CREATE_INFO };
    aci.actionType = type;
    std::strncpy(aci.actionName, name, XR_MAX_ACTION_NAME_SIZE - 1);
    std::strncpy(aci.localizedActionName, name, XR_MAX_LOCALIZED_ACTION_NAME_SIZE - 1);
    if (perHand) { aci.countSubactionPaths = 2; aci.subactionPaths = g.handPaths; }
    XrAction a = XR_NULL_HANDLE;
    check(xrCreateAction(g.actionSet, &aci, &a), name);
    return a;
}

bool create_actions() {
    XrActionSetCreateInfo asci{ XR_TYPE_ACTION_SET_CREATE_INFO };
    std::strncpy(asci.actionSetName, "gameplay", XR_MAX_ACTION_SET_NAME_SIZE - 1);
    std::strncpy(asci.localizedActionSetName, "Gameplay", XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE - 1);
    if (!check(xrCreateActionSet(g.instance, &asci, &g.actionSet), "xrCreateActionSet")) return false;
    xrStringToPath(g.instance, "/user/hand/left", &g.handPaths[0]);
    xrStringToPath(g.instance, "/user/hand/right", &g.handPaths[1]);

    g.aPose = make_action("hand_pose", XR_ACTION_TYPE_POSE_INPUT, true);
    g.aStick = make_action("stick", XR_ACTION_TYPE_VECTOR2F_INPUT, true);
    g.aHaptic = make_action("haptic", XR_ACTION_TYPE_VIBRATION_OUTPUT, true);
    g.aA = make_action("a", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
    g.aB = make_action("b", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
    g.aX = make_action("x", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
    g.aY = make_action("y", XR_ACTION_TYPE_BOOLEAN_INPUT, false);
    g.aTrigger = make_action("trigger", XR_ACTION_TYPE_BOOLEAN_INPUT, true);
    g.aGrip = make_action("grip", XR_ACTION_TYPE_BOOLEAN_INPUT, true);
    g.aStickClick = make_action("stick_click", XR_ACTION_TYPE_BOOLEAN_INPUT, true);
    g.aMenu = make_action("menu", XR_ACTION_TYPE_BOOLEAN_INPUT, false);

    auto P = [&](const char* s) { XrPath p; xrStringToPath(g.instance, s, &p); return p; };
    std::vector<XrActionSuggestedBinding> b = {
        { g.aPose, P("/user/hand/left/input/aim/pose") },      { g.aPose, P("/user/hand/right/input/aim/pose") },
        { g.aStick, P("/user/hand/left/input/thumbstick") },   { g.aStick, P("/user/hand/right/input/thumbstick") },
        { g.aHaptic, P("/user/hand/left/output/haptic") },     { g.aHaptic, P("/user/hand/right/output/haptic") },
        { g.aA, P("/user/hand/right/input/a/click") },          { g.aB, P("/user/hand/right/input/b/click") },
        { g.aX, P("/user/hand/left/input/x/click") },           { g.aY, P("/user/hand/left/input/y/click") },
        { g.aTrigger, P("/user/hand/left/input/trigger/value") },  { g.aTrigger, P("/user/hand/right/input/trigger/value") },
        { g.aGrip, P("/user/hand/left/input/squeeze/value") },     { g.aGrip, P("/user/hand/right/input/squeeze/value") },
        { g.aStickClick, P("/user/hand/left/input/thumbstick/click") },
        { g.aStickClick, P("/user/hand/right/input/thumbstick/click") },
        { g.aMenu, P("/user/hand/left/input/menu/click") },
    };
    XrInteractionProfileSuggestedBinding sb{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
    sb.interactionProfile = P("/interaction_profiles/oculus/touch_controller");
    sb.countSuggestedBindings = (uint32_t)b.size();
    sb.suggestedBindings = b.data();
    check(xrSuggestInteractionProfileBindings(g.instance, &sb), "xrSuggestInteractionProfileBindings (touch)");

    for (int h = 0; h < 2; h++) {
        XrActionSpaceCreateInfo asi{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
        asi.action = g.aPose;
        asi.subactionPath = g.handPaths[h];
        asi.poseInActionSpace.orientation.w = 1.f;
        check(xrCreateActionSpace(g.session, &asi, &g.handSpaces[h]), "xrCreateActionSpace");
    }
    XrSessionActionSetsAttachInfo att{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
    att.countActionSets = 1;
    att.actionSets = &g.actionSet;
    return check(xrAttachSessionActionSets(g.session, &att), "xrAttachSessionActionSets");
}

void poll_events() {
    XrEventDataBuffer ev{ XR_TYPE_EVENT_DATA_BUFFER };
    while (xrPollEvent(g.instance, &ev) == XR_SUCCESS) {
        if (ev.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
            auto* s = (XrEventDataSessionStateChanged*)&ev;
            g.sessionState = s->state;
            XRLOG_INFO("état de session : %d", (int)s->state);
            switch (s->state) {
                case XR_SESSION_STATE_READY: {
                    XrSessionBeginInfo bi{ XR_TYPE_SESSION_BEGIN_INFO };
                    bi.primaryViewConfigurationType = g.viewConfig;
                    if (check(xrBeginSession(g.session, &bi), "xrBeginSession")) {
                        g.sessionRunning = true;
                        XRLOG_INFO("session démarrée : le jeu s'affiche dans le casque");
                    }
                    break;
                }
                case XR_SESSION_STATE_STOPPING:
                    xrEndSession(g.session);
                    g.sessionRunning = false;
                    XRLOG_INFO("session arrêtée");
                    break;
                case XR_SESSION_STATE_EXITING:
                case XR_SESSION_STATE_LOSS_PENDING:
                    g.sessionRunning = false;
                    g.enabled = false;
                    XRLOG_WARN("session perdue : retour au mode écran");
                    break;
                default: break;
            }
        } else if (ev.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
            g.sessionRunning = false;
            g.enabled = false;
            XRLOG_WARN("instance OpenXR perdue : retour au mode écran");
        }
        ev = { XR_TYPE_EVENT_DATA_BUFFER };
    }
}

void sync_inputs() {
    XrActiveActionSet active{ g.actionSet, XR_NULL_PATH };
    XrActionsSyncInfo si{ XR_TYPE_ACTIONS_SYNC_INFO };
    si.countActiveActionSets = 1;
    si.activeActionSets = &active;
    if (!XR_SUCCEEDED(xrSyncActions(g.session, &si))) return;

    auto getBool = [&](XrAction a, XrPath sub) {
        XrActionStateGetInfo gi{ XR_TYPE_ACTION_STATE_GET_INFO };
        gi.action = a; gi.subactionPath = sub;
        XrActionStateBoolean st{ XR_TYPE_ACTION_STATE_BOOLEAN };
        xrGetActionStateBoolean(g.session, &gi, &st);
        return st.isActive && st.currentState;
    };
    uint32_t b = 0;
    if (getBool(g.aA, XR_NULL_PATH)) b |= XR_BTN_A;
    if (getBool(g.aB, XR_NULL_PATH)) b |= XR_BTN_B;
    if (getBool(g.aX, XR_NULL_PATH)) b |= XR_BTN_X;
    if (getBool(g.aY, XR_NULL_PATH)) b |= XR_BTN_Y;
    if (getBool(g.aMenu, XR_NULL_PATH)) b |= XR_BTN_MENU;
    if (getBool(g.aTrigger, g.handPaths[0])) b |= XR_BTN_LTRIGGER;
    if (getBool(g.aTrigger, g.handPaths[1])) b |= XR_BTN_RTRIGGER;
    if (getBool(g.aGrip, g.handPaths[0])) b |= XR_BTN_LGRIP;
    if (getBool(g.aGrip, g.handPaths[1])) b |= XR_BTN_RGRIP;
    if (getBool(g.aStickClick, g.handPaths[0])) b |= XR_BTN_LSTICK;
    if (getBool(g.aStickClick, g.handPaths[1])) b |= XR_BTN_RSTICK;
    g.buttons = b;

    for (int h = 0; h < 2; h++) {
        XrActionStateGetInfo gi{ XR_TYPE_ACTION_STATE_GET_INFO };
        gi.action = g.aStick; gi.subactionPath = g.handPaths[h];
        XrActionStateVector2f st{ XR_TYPE_ACTION_STATE_VECTOR2F };
        xrGetActionStateVector2f(g.session, &gi, &st);
        g.sticks[h][0] = st.isActive ? st.currentState.x : 0.f;
        g.sticks[h][1] = st.isActive ? st.currentState.y : 0.f;

        XrSpaceLocation loc{ XR_TYPE_SPACE_LOCATION };
        xrLocateSpace(g.handSpaces[h], g.localSpace, g.frameState.predictedDisplayTime, &loc);
        const bool ok = (loc.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) &&
                        (loc.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT);
        g.hands[h] = relative_to_reference(to_pose(loc.pose, ok));
    }
}

// Matrice vue d'un œil (row-major) : translation puis rotation inverses de la pose, en unités jeu
void eye_view(int eye, bool withTranslation, Mat4 out) {
    const XrView& v = g.eyes[eye].view;
    xr_pose_t p = relative_to_reference(to_pose(v.pose, true));
    Mat4 T, R;
    const float k = g.unitsPerMeter / (g.worldScale > 0.f ? g.worldScale : 1.f);
    if (withTranslation) mat_translate(T, -p.position.x * k, -p.position.y * k, -p.position.z * k);
    else mat_identity(T);
    mat_from_quat(R, quat_conj(p.orientation));
    mat_mul(out, T, R);
}

#endif // RETROXR_HAVE_BACKEND

} // namespace

// ================================================================== API publique

bool xr_init(void) {
#if RETROXR_HAVE_BACKEND
    if (g.initialized) return true;
    if (!g.enabled) return false;
    if (!create_instance()) { xr_shutdown(); return false; }
    if (!create_session()) { xr_shutdown(); return false; }
    if (!create_swapchains()) { xr_shutdown(); return false; }
    if (!create_actions()) { xr_shutdown(); return false; }
    g.initialized = true;
    XRLOG_INFO("initialisé (OpenGL, %u x %u par œil)", g.eyes[0].width, g.eyes[0].height);
    return true;
#else
    XRLOG_WARN("backend OpenXR indisponible sur cette plateforme (V1 = Windows + OpenGL)");
    return false;
#endif
}

void xr_shutdown(void) {
#if RETROXR_HAVE_BACKEND
    for (auto& t : g.eyes) {
        if (!t.fbos.empty()) glDeleteFramebuffers((GLsizei)t.fbos.size(), t.fbos.data());
        t.fbos.clear();
        if (t.swapchain != XR_NULL_HANDLE) xrDestroySwapchain(t.swapchain);
        t.swapchain = XR_NULL_HANDLE;
    }
    if (g.blitFbo) { glDeleteFramebuffers(1, &g.blitFbo); g.blitFbo = 0; }
    for (auto& s : g.handSpaces) { if (s != XR_NULL_HANDLE) xrDestroySpace(s); s = XR_NULL_HANDLE; }
    if (g.actionSet != XR_NULL_HANDLE) { xrDestroyActionSet(g.actionSet); g.actionSet = XR_NULL_HANDLE; }
    if (g.localSpace != XR_NULL_HANDLE) { xrDestroySpace(g.localSpace); g.localSpace = XR_NULL_HANDLE; }
    if (g.session != XR_NULL_HANDLE) { xrDestroySession(g.session); g.session = XR_NULL_HANDLE; }
    if (g.instance != XR_NULL_HANDLE) { xrDestroyInstance(g.instance); g.instance = XR_NULL_HANDLE; }
#endif
    g.initialized = false;
    g.sessionRunning = false;
    g.frameOpen = false;
}

bool xr_is_active(void) {
#if RETROXR_HAVE_BACKEND
    // Tant que la session n'a pas démarré, il faut écouter le runtime pour recevoir l'état READY
    // (sinon : le casque affiche « chargement » indéfiniment).
    if (g.enabled && g.initialized && !g.sessionRunning) {
        poll_events();
    }
#endif
    return g.enabled && g.initialized && g.sessionRunning;
}

void xr_set_enabled(bool enabled) {
    g.enabled = enabled;
}

bool xr_begin_frame(void) {
#if RETROXR_HAVE_BACKEND
    if (!g.enabled || !g.initialized) return false;
    poll_events();
    if (!g.sessionRunning) return false;

    XrFrameWaitInfo wi{ XR_TYPE_FRAME_WAIT_INFO };
    g.frameState = { XR_TYPE_FRAME_STATE };
    if (!check(xrWaitFrame(g.session, &wi, &g.frameState), "xrWaitFrame")) return false;
    XrFrameBeginInfo bi{ XR_TYPE_FRAME_BEGIN_INFO };
    if (!check(xrBeginFrame(g.session, &bi), "xrBeginFrame")) return false;
    g.frameOpen = true;
    g.shouldRender = g.frameState.shouldRender == XR_TRUE;
    if (!g.shouldRender) {
        // Le runtime ne veut pas d'image (casque retiré, menu système ouvert) : clore la frame tout de suite,
        // sinon le cycle wait/begin/end se bloque.
        xr_end_frame();
        return false;
    }

    // Vues
    XrViewLocateInfo li{ XR_TYPE_VIEW_LOCATE_INFO };
    li.viewConfigurationType = g.viewConfig;
    li.displayTime = g.frameState.predictedDisplayTime;
    li.space = g.localSpace;
    XrViewState vs{ XR_TYPE_VIEW_STATE };
    XrView views[2] = { { XR_TYPE_VIEW }, { XR_TYPE_VIEW } };
    uint32_t n = 0;
    xrLocateViews(g.session, &li, &vs, 2, &n, views);
    for (int e = 0; e < 2; e++) g.eyes[e].view = views[e];

    // Tête = milieu des deux yeux
    {
        XrPosef mid = views[0].pose;
        mid.position.x = (views[0].pose.position.x + views[1].pose.position.x) * 0.5f;
        mid.position.y = (views[0].pose.position.y + views[1].pose.position.y) * 0.5f;
        mid.position.z = (views[0].pose.position.z + views[1].pose.position.z) * 0.5f;
        g.head = relative_to_reference(to_pose(mid, (vs.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0));
    }

    sync_inputs();
    return g.shouldRender;
#else
    return false;
#endif
}

void xr_begin_eye(xr_eye_t eye) {
#if RETROXR_HAVE_BACKEND
    if (!g.frameOpen || !g.shouldRender) return;
    auto& t = g.eyes[eye];
    XrSwapchainImageAcquireInfo ai{ XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO };
    if (!check(xrAcquireSwapchainImage(t.swapchain, &ai, &t.acquiredIndex), "xrAcquireSwapchainImage")) return;
    XrSwapchainImageWaitInfo wi{ XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO };
    wi.timeout = XR_INFINITE_DURATION;
    check(xrWaitSwapchainImage(t.swapchain, &wi), "xrWaitSwapchainImage");
#else
    (void)eye;
#endif
}

// Appelé par libultraship avec l'identifiant GL de la texture couleur du framebuffer du jeu.
void xr_submit_eye_texture(xr_eye_t eye, uint32_t glTexture, uint32_t texWidth, uint32_t texHeight) {
#if RETROXR_HAVE_BACKEND
    if (!g.frameOpen || !g.shouldRender) return;
    auto& t = g.eyes[eye];
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &g.savedFbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, g.blitFbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glTexture, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, t.fbos[t.acquiredIndex]);
    // Le framebuffer du jeu (Fast3D/OpenGL) est stocké inversé en Y (invertY) : on le retourne à la copie.
    glBlitFramebuffer(0, (GLint)texHeight, (GLint)texWidth, 0, 0, 0, (GLint)t.width, (GLint)t.height,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)g.savedFbo);
#else
    (void)eye; (void)glTexture; (void)texWidth; (void)texHeight;
#endif
}

void xr_end_eye(xr_eye_t eye) {
#if RETROXR_HAVE_BACKEND
    if (!g.frameOpen || !g.shouldRender) return;
    auto& t = g.eyes[eye];
    XrSwapchainImageReleaseInfo ri{ XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO };
    check(xrReleaseSwapchainImage(t.swapchain, &ri), "xrReleaseSwapchainImage");

    auto& pv = g.projViews[eye];
    pv = { XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW };
    pv.pose = t.view.pose;
    pv.fov = t.view.fov;
    pv.subImage.swapchain = t.swapchain;
    pv.subImage.imageRect.offset = { 0, 0 };
    pv.subImage.imageRect.extent = { (int32_t)t.width, (int32_t)t.height };
    pv.subImage.imageArrayIndex = 0;
#else
    (void)eye;
#endif
}

void xr_end_frame(void) {
#if RETROXR_HAVE_BACKEND
    if (!g.frameOpen) return;
    XrCompositionLayerProjection layer{ XR_TYPE_COMPOSITION_LAYER_PROJECTION };
    layer.space = g.localSpace;
    layer.viewCount = 2;
    layer.views = g.projViews.data();
    const XrCompositionLayerBaseHeader* layers[] = { (const XrCompositionLayerBaseHeader*)&layer };

    XrFrameEndInfo ei{ XR_TYPE_FRAME_END_INFO };
    ei.displayTime = g.frameState.predictedDisplayTime;
    ei.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    ei.layerCount = g.shouldRender ? 1 : 0;
    ei.layers = g.shouldRender ? layers : nullptr;
    check(xrEndFrame(g.session, &ei), "xrEndFrame");
    g.frameOpen = false;
#endif
}

void xr_get_eye_projection(xr_eye_t eye, float near_plane, float far_plane, float out[4][4]) {
#if RETROXR_HAVE_BACKEND
    const XrFovf& f = g.eyes[eye].view.fov;
    mat_projection_from_tangents(out, std::tan(f.angleLeft), std::tan(f.angleRight), std::tan(f.angleDown),
                                 std::tan(f.angleUp), near_plane, far_plane);
#else
    (void)eye; (void)near_plane; (void)far_plane; mat_identity(out);
#endif
}

void xr_get_eye_view(xr_eye_t eye, float out[4][4]) {
#if RETROXR_HAVE_BACKEND
    eye_view(eye, true, out);
#else
    (void)eye; mat_identity(out);
#endif
}

void xr_get_eye_view_rotation_only(xr_eye_t eye, float out[4][4]) {
#if RETROXR_HAVE_BACKEND
    eye_view(eye, false, out);
#else
    (void)eye; mat_identity(out);
#endif
}

// Remplace, en place, une matrice de projection du jeu (perspective ou ortho) par son équivalent VR
// pour l'œil courant. Retourne false si la matrice n'est pas reconnue (elle est alors laissée intacte).
bool xr_override_projection(float m[4][4], xr_eye_t eye) {
#if RETROXR_HAVE_BACKEND
    if (!g.frameOpen) return false;
    const bool perspective = std::fabs(m[2][3]) > 1e-6f && std::fabs(m[3][3]) < 1e-6f;
    const bool ortho = std::fabs(m[2][3]) < 1e-6f && std::fabs(m[3][3] - 1.f) < 1e-3f && std::fabs(m[2][2]) > 1e-9f;

    if (perspective) {
        // guPerspective peut multiplier toute la matrice par un facteur d'échelle : normaliser par -m[2][3]
        const float s = -m[2][3];
        const float A = m[2][2] / s, B = m[3][2] / s;
        float n = B / (A - 1.f), f = B / (A + 1.f);
        if (!(n > 0.f) || !(f > n)) { n = 10.f; f = 20000.f; }
        Mat4 P, V;
        xr_get_eye_projection(eye, n, f, P);
        eye_view(eye, true, V);
        mat_mul(m, V, P); // v * MV * (V_eye * P_xr)
        return true;
    }
    if (ortho) {
        // Plan HUD « verrouillé sur la tête » : NDC du jeu -> rectangle à hudDistance devant l'œil
        const float aspect = (float)g.eyes[eye].width / (float)std::max(1u, g.eyes[eye].height);
        const float h = g.hudHeight, w = h * (4.f / 3.f);
        (void)aspect;
        Mat4 S, T, E, P, tmp;
        mat_scale(S, w * 0.5f, h * 0.5f, 0.001f);
        mat_translate(T, 0.f, 0.f, -g.hudDistance);
        // Décalage inter-oculaire seul (pas la rotation : le HUD suit la tête)
        const XrView& v = g.eyes[eye].view;
        const XrView& o = g.eyes[eye ^ 1].view;
        const float ipdHalf = 0.5f * (v.pose.position.x - o.pose.position.x);
        mat_translate(E, -ipdHalf, 0.f, 0.f);
        xr_get_eye_projection(eye, 0.05f, 100.f, P);
        // m (ortho jeu, unités jeu -> NDC) * S (NDC -> mètres) * T * E * P
        mat_mul(tmp, m, S);
        mat_mul(tmp, tmp, T);
        mat_mul(tmp, tmp, E);
        mat_mul(m, tmp, P);
        return true;
    }
    return false;
#else
    (void)m; (void)eye;
    return false;
#endif
}

xr_pose_t xr_get_head_pose(void) { return g.head; }
xr_pose_t xr_get_controller_pose(int hand) { return g.hands[hand & 1]; }
uint32_t xr_get_buttons(void) { return g.buttons; }

void xr_get_stick(int hand, float* out_x, float* out_y) {
    if (out_x) *out_x = g.sticks[hand & 1][0];
    if (out_y) *out_y = g.sticks[hand & 1][1];
}

void xr_haptic(int hand, float amplitude, float seconds) {
#if RETROXR_HAVE_BACKEND
    if (!g.initialized) return;
    XrHapticVibration hv{ XR_TYPE_HAPTIC_VIBRATION };
    hv.amplitude = amplitude;
    hv.duration = (XrDuration)(seconds * 1e9);
    hv.frequency = XR_FREQUENCY_UNSPECIFIED;
    XrHapticActionInfo hai{ XR_TYPE_HAPTIC_ACTION_INFO };
    hai.action = g.aHaptic;
    hai.subactionPath = g.handPaths[hand & 1];
    xrApplyHapticFeedback(g.session, &hai, (const XrHapticBaseHeader*)&hv);
#else
    (void)hand; (void)amplitude; (void)seconds;
#endif
}

void xr_get_info(xr_info_t* out) {
    if (!out) return;
#if RETROXR_HAVE_BACKEND
    out->width = g.eyes[0].width;
    out->height = g.eyes[0].height;
#else
    out->width = out->height = 0;
#endif
    out->units_per_meter = g.unitsPerMeter;
    out->world_scale = g.worldScale;
    out->hmd_present = g.initialized;
    out->session_running = g.sessionRunning;
}

void xr_set_units_per_meter(float u) { if (u > 0.f) g.unitsPerMeter = u; }
void xr_set_world_scale(float s) { if (s > 0.f) g.worldScale = s; }

void xr_recenter(void) {
#if RETROXR_HAVE_BACKEND
    // Nouvelle référence = position courante + yaw courant (on garde l'horizon du runtime)
    const XrView& v = g.eyes[0].view;
    const XrView& o = g.eyes[1].view;
    xr_pose_t absHead = to_pose(v.pose, true);
    absHead.position.x = (v.pose.position.x + o.pose.position.x) * 0.5f;
    absHead.position.y = (v.pose.position.y + o.pose.position.y) * 0.5f;
    absHead.position.z = (v.pose.position.z + o.pose.position.z) * 0.5f;
    g.reference.position = absHead.position;
    g.reference.orientation = quat_yaw_only(absHead.orientation);
    XRLOG_INFO("recentré");
#endif
}

void xr_set_hud_plane(float distance_m, float height_m) {
    if (distance_m > 0.1f) g.hudDistance = distance_m;
    if (height_m > 0.1f) g.hudHeight = height_m;
}

void xr_get_hud_projection(xr_eye_t eye, float out[4][4]) {
    mat_identity(out);
    xr_override_projection(out, eye);
}

#endif // ENABLE_XR
