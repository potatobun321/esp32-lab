#ifndef SCENES_H
#define SCENES_H

#include <stdint.h>

typedef enum {
    SCENE_CUBE = 0,
    SCENE_STARFIELD,
    SCENE_BOUNCING,
    SCENE_STATS,
    SCENE_TUX,
    SCENE_COUNT
} scene_id_t;

void scenes_init(void);
void scenes_next(void);
void scenes_set(scene_id_t scene);
scene_id_t scenes_get_current(void);
const char* scenes_get_name(scene_id_t scene);
void scenes_render_current_frame(void);

#endif // SCENES_H

