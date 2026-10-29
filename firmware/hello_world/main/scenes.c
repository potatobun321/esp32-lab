#include "scenes.h"
#include "oled_graphics.h"
#include "tux_bitmap.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_system.h"
#include "esp_timer.h"

static scene_id_t current_scene = SCENE_CUBE;

/* -------------------------------------------------------------
 * Scene 1: 3D Rotating Wireframe Cube
 * ------------------------------------------------------------- */
typedef struct {
    float x, y, z;
} point3d_t;

typedef struct {
    int x, y;
} point2d_t;

static const point3d_t cube_vertices[8] = {
    {-14.0f, -14.0f, -14.0f},
    { 14.0f, -14.0f, -14.0f},
    { 14.0f,  14.0f, -14.0f},
    {-14.0f,  14.0f, -14.0f},
    {-14.0f, -14.0f,  14.0f},
    { 14.0f, -14.0f,  14.0f},
    { 14.0f,  14.0f,  14.0f},
    {-14.0f,  14.0f,  14.0f}
};

static const int cube_edges[12][2] = {
    {0, 1}, {1, 2}, {2, 3}, {3, 0}, // Front face
    {4, 5}, {5, 6}, {6, 7}, {7, 4}, // Back face
    {0, 4}, {1, 5}, {2, 6}, {3, 7}  // Connecting edges
};

static float rot_x = 0.0f;
static float rot_y = 0.0f;
static float rot_z = 0.0f;

static void render_cube_scene(void)
{
    oled_draw_text(2, 0, "1/4: 3D Wireframe", COLOR_WHITE);
    oled_draw_line(0, 9, 127, 9, COLOR_WHITE);

    rot_x += 0.04f;
    rot_y += 0.06f;
    rot_z += 0.02f;

    float cx = cosf(rot_x), sx = sinf(rot_x);
    float cy = cosf(rot_y), sy = sinf(rot_y);
    float cz = cosf(rot_z), sz = sinf(rot_z);

    point2d_t projected[8];
    const float distance = 50.0f;
    const float fov = 50.0f;
    const int origin_x = 64;
    const int origin_y = 38;

    for (int i = 0; i < 8; i++) {
        // Rotate around X
        float y1 = cube_vertices[i].y * cx - cube_vertices[i].z * sx;
        float z1 = cube_vertices[i].y * sx + cube_vertices[i].z * cx;
        float x1 = cube_vertices[i].x;

        // Rotate around Y
        float x2 = x1 * cy + z1 * sy;
        float z2 = -x1 * sy + z1 * cy;
        float y2 = y1;

        // Rotate around Z
        float x3 = x2 * cz - y2 * sz;
        float y3 = x2 * sz + y2 * cz;
        float z3 = z2;

        // Perspective Projection
        float z_proj = z3 + distance;
        if (z_proj < 1.0f) z_proj = 1.0f;

        projected[i].x = origin_x + (int)((x3 * fov) / z_proj);
        projected[i].y = origin_y + (int)((y3 * fov) / z_proj);

        // Draw corner vertex dot
        oled_set_pixel(projected[i].x, projected[i].y, COLOR_WHITE);
    }

    // Draw 12 edges
    for (int i = 0; i < 12; i++) {
        int v0 = cube_edges[i][0];
        int v1 = cube_edges[i][1];
        oled_draw_line(projected[v0].x, projected[v0].y,
                       projected[v1].x, projected[v1].y,
                       COLOR_WHITE);
    }
}

/* -------------------------------------------------------------
 * Scene 2: 3D Warp Starfield
 * ------------------------------------------------------------- */
#define STAR_COUNT 64

typedef struct {
    float x, y, z;
    float prev_x, prev_y;
} star_t;

static star_t stars[STAR_COUNT];
static bool stars_initialized = false;

static void init_star(int i, bool initial)
{
    stars[i].x = (float)((rand() % 160) - 80);
    stars[i].y = (float)((rand() % 100) - 50);
    stars[i].z = initial ? (float)(rand() % 100 + 1) : 100.0f;
    stars[i].prev_x = -1;
    stars[i].prev_y = -1;
}

static void render_starfield_scene(void)
{
    oled_draw_text(2, 0, "2/4: Warp Starfield", COLOR_WHITE);
    oled_draw_line(0, 9, 127, 9, COLOR_WHITE);

    if (!stars_initialized) {
        for (int i = 0; i < STAR_COUNT; i++) {
            init_star(i, true);
        }
        stars_initialized = true;
    }

    const float speed = 2.5f;
    const int origin_x = 64;
    const int origin_y = 38;

    for (int i = 0; i < STAR_COUNT; i++) {
        stars[i].z -= speed;

        if (stars[i].z <= 1.0f) {
            init_star(i, false);
            continue;
        }

        int sx = origin_x + (int)((stars[i].x * 40.0f) / stars[i].z);
        int sy = origin_y + (int)((stars[i].y * 40.0f) / stars[i].z);

        if (sx < 0 || sx >= OLED_WIDTH || sy < 10 || sy >= OLED_HEIGHT) {
            init_star(i, false);
            continue;
        }

        // Draw light streak if previous position was valid
        if (stars[i].prev_x >= 0 && stars[i].z < 60.0f) {
            oled_draw_line((int)stars[i].prev_x, (int)stars[i].prev_y, sx, sy, COLOR_WHITE);
        } else {
            oled_set_pixel(sx, sy, COLOR_WHITE);
        }

        stars[i].prev_x = (float)sx;
        stars[i].prev_y = (float)sy;
    }
}

/* -------------------------------------------------------------
 * Scene 3: Bouncing Physics / Screensaver
 * ------------------------------------------------------------- */
static int box_x = 10;
static int box_y = 20;
static int box_vx = 2;
static int box_vy = 1;
static const int box_w = 34;
static const int box_h = 12;

static int ball_x = 80;
static int ball_y = 35;
static int ball_vx = -2;
static int ball_vy = 2;
static const int ball_r = 4;

static void render_bouncing_scene(void)
{
    oled_draw_text(2, 0, "3/4: Bounce Physics", COLOR_WHITE);
    oled_draw_line(0, 9, 127, 9, COLOR_WHITE);

    // Playfield outer border
    oled_draw_rect(0, 10, 128, 54, COLOR_WHITE);

    // Update box position
    box_x += box_vx;
    box_y += box_vy;

    if (box_x <= 1) {
        box_x = 1;
        box_vx = -box_vx;
    } else if (box_x + box_w >= 127) {
        box_x = 127 - box_w;
        box_vx = -box_vx;
    }

    if (box_y <= 11) {
        box_y = 11;
        box_vy = -box_vy;
    } else if (box_y + box_h >= 63) {
        box_y = 63 - box_h;
        box_vy = -box_vy;
    }

    // Update ball position
    ball_x += ball_vx;
    ball_y += ball_vy;

    if (ball_x - ball_r <= 1) {
        ball_x = 1 + ball_r;
        ball_vx = -ball_vx;
    } else if (ball_x + ball_r >= 126) {
        ball_x = 126 - ball_r;
        ball_vx = -ball_vx;
    }

    if (ball_y - ball_r <= 11) {
        ball_y = 11 + ball_r;
        ball_vy = -ball_vy;
    } else if (ball_y + ball_r >= 62) {
        ball_y = 62 - ball_r;
        ball_vy = -ball_vy;
    }

    // Draw bouncing box with text
    oled_draw_rect(box_x, box_y, box_w, box_h, COLOR_WHITE);
    oled_draw_text(box_x + 3, box_y + 2, "ESP32", COLOR_WHITE);

    // Draw bouncing filled ball
    oled_fill_circle(ball_x, ball_y, ball_r, COLOR_WHITE);
}

/* -------------------------------------------------------------
 * Scene 4: Diagnostics & Primitives Showcase
 * ------------------------------------------------------------- */
static void render_stats_scene(void)
{
    oled_draw_text(2, 0, "4/4: Diagnostics", COLOR_WHITE);
    oled_draw_line(0, 9, 127, 9, COLOR_WHITE);

    char buf[32];
    uint32_t uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t free_heap_kb = esp_get_free_heap_size() / 1024;

    snprintf(buf, sizeof(buf), "Uptime: %lus", (unsigned long)uptime_s);
    oled_draw_text(4, 12, buf, COLOR_WHITE);

    snprintf(buf, sizeof(buf), "RAM:    %lu KB free", (unsigned long)free_heap_kb);
    oled_draw_text(4, 21, buf, COLOR_WHITE);

    // Bottom half: Primitives showcase
    oled_draw_line(0, 31, 127, 31, COLOR_WHITE);

    // Concentric circles
    oled_draw_circle(16, 47, 13, COLOR_WHITE);
    oled_draw_circle(16, 47, 8, COLOR_WHITE);
    oled_fill_circle(16, 47, 3, COLOR_WHITE);

    // Nested rectangles
    oled_draw_rect(36, 36, 22, 22, COLOR_WHITE);
    oled_fill_rect(41, 41, 12, 12, COLOR_WHITE);

    // Crosshairs & rays
    oled_draw_line(68, 36, 92, 60, COLOR_WHITE);
    oled_draw_line(68, 60, 92, 36, COLOR_WHITE);
    oled_draw_line(80, 36, 80, 60, COLOR_WHITE);
    oled_draw_line(68, 48, 92, 48, COLOR_WHITE);

    // Inverted button indicator badge
    oled_fill_rect(100, 38, 25, 18, COLOR_WHITE);
    oled_draw_text(103, 43, "OK", COLOR_BLACK);
}

/* -------------------------------------------------------------
 * Scene 5: Linux Tux Bitmap Demo
 * ------------------------------------------------------------- */
static void render_tux_scene(void)
{
    oled_draw_text(2, 0, "5/5: Linux Tux", COLOR_WHITE);
    oled_draw_line(0, 9, 127, 9, COLOR_WHITE);

    // Draw Tux bitmap (48x52)
    oled_draw_bitmap(2, 11, TUX_WIDTH, TUX_HEIGHT, tux_bitmap, COLOR_WHITE);

    // Vertical separator
    oled_draw_line(52, 10, 52, 63, COLOR_WHITE);

    // Linux details on the right
    oled_draw_text(57, 12, "TUX", COLOR_WHITE);
    oled_draw_text(57, 21, "PENGUIN", COLOR_WHITE);
    oled_draw_line(57, 30, 124, 30, COLOR_WHITE);
    oled_draw_text(57, 34, "OS: Linux", COLOR_WHITE);
    oled_draw_text(57, 43, "ESP32 Lab", COLOR_WHITE);
    oled_draw_text(57, 52, "128x64 B/W", COLOR_WHITE);
}

/* -------------------------------------------------------------
 * Scene Switcher Management
 * ------------------------------------------------------------- */
void scenes_init(void)
{
    current_scene = SCENE_CUBE;
    stars_initialized = false;
}

void scenes_next(void)
{
    current_scene = (scene_id_t)((current_scene + 1) % SCENE_COUNT);
}

void scenes_set(scene_id_t scene)
{
    if (scene < SCENE_COUNT) {
        current_scene = scene;
    }
}

scene_id_t scenes_get_current(void)
{
    return current_scene;
}

const char* scenes_get_name(scene_id_t scene)
{
    switch (scene) {
        case SCENE_CUBE:      return "3D Cube";
        case SCENE_STARFIELD: return "Starfield";
        case SCENE_BOUNCING:  return "Bouncing";
        case SCENE_STATS:     return "Diagnostics";
        case SCENE_TUX:       return "Linux Tux";
        default:              return "Unknown";
    }
}

void scenes_render_current_frame(void)
{
    oled_clear(COLOR_BLACK);

    switch (current_scene) {
        case SCENE_CUBE:
            render_cube_scene();
            break;
        case SCENE_STARFIELD:
            render_starfield_scene();
            break;
        case SCENE_BOUNCING:
            render_bouncing_scene();
            break;
        case SCENE_STATS:
            render_stats_scene();
            break;
        case SCENE_TUX:
            render_tux_scene();
            break;
        default:
            break;
    }
}

