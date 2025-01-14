#ifndef REGIONS_H_
#define REGIONS_H_

#include "math/rect.h"

typedef struct Regions Regions;
typedef struct Player Player;
typedef struct LineStream LineStream;
typedef struct Level Level;
typedef struct Camera Camera;
typedef struct RectLayer RectLayer;

Regions *create_regions_from_line_stream(LineStream *line_stream);
Regions *create_regions_from_rect_layer(const RectLayer *rect_layer);
void destroy_regions(Regions *regions);

int regions_render(Regions *regions, Camera *camera);

void regions_player_enter(Regions *regions, Player *player, Script *supa_script);
void regions_player_leave(Regions *regions, Player *player, Script *supa_script);

#endif  // REGIONS_H_
