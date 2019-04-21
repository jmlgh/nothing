#include "system/stacktrace.h"

#include "broadcast.h"
#include "ebisp/builtins.h"
#include "ebisp/interpreter.h"
#include "game/level/boxes.h"
#include "game/level/player.h"
#include "game/level/rigid_bodies.h"
#include "math/rand.h"
#include "system/line_stream.h"
#include "system/log.h"
#include "system/lt.h"
#include "system/nth_alloc.h"
#include "system/str.h"

#define BOXES_CAPACITY 1000
#define BOXES_MAX_ID_SIZE 36

struct Boxes
{
    Lt *lt;
    RigidBodies *rigid_bodies;
    RigidBodyId *body_ids;
    Color *body_colors;
    size_t count;
};

Boxes *create_boxes_from_line_stream(LineStream *line_stream, RigidBodies *rigid_bodies)
{
    trace_assert(line_stream);

    Lt *lt = create_lt();

    if (lt == NULL) {
        return NULL;
    }

    Boxes *boxes = PUSH_LT(lt, nth_alloc(sizeof(Boxes)), free);
    if (boxes == NULL) {
        RETURN_LT(lt, NULL);
    }
    boxes->lt = lt;

    boxes->rigid_bodies = rigid_bodies;

    if (sscanf(
            line_stream_next(line_stream),
            "%lu",
            &boxes->count) == EOF) {
        log_fail("Could not read amount of boxes\n");
        RETURN_LT(lt, NULL);
    }
    log_info("Boxes count: %d\n", boxes->count);

    trace_assert(boxes->count < BOXES_CAPACITY);

    boxes->body_ids = PUSH_LT(lt, nth_alloc(sizeof(RigidBodyId) * BOXES_CAPACITY), free);
    if (boxes->body_ids == NULL) {
        RETURN_LT(lt, NULL);
    }

    boxes->body_colors = PUSH_LT(lt, nth_alloc(sizeof(Color) * BOXES_CAPACITY), free);
    if (boxes->body_colors == NULL) {
        RETURN_LT(lt, NULL);
    }

    for (size_t i = 0; i < boxes->count; ++i) {
        char color[7];
        Rect rect;
        // TODO: box id is ignored
        char id[BOXES_MAX_ID_SIZE];

        if (sscanf(line_stream_next(line_stream),
                   "%" STRINGIFY(BOXES_MAX_ID_SIZE) "s%f%f%f%f%6s\n",
                   id,
                   &rect.x, &rect.y,
                   &rect.w, &rect.h,
                   color) < 0) {
            log_fail("Could not read rigid rect\n");
            RETURN_LT(lt, NULL);
        }

        boxes->body_colors[i] = hexstr(color);
        boxes->body_ids[i] = rigid_bodies_add(rigid_bodies, rect);
    }

    return boxes;
}

void destroy_boxes(Boxes *boxes)
{
    trace_assert(boxes);

    for (size_t i = 0; i < boxes->count; ++i) {
        rigid_bodies_remove(boxes->rigid_bodies, boxes->body_ids[i]);
    }

    RETURN_LT0(boxes->lt);
}

int boxes_render(Boxes *boxes, Camera *camera)
{
    trace_assert(boxes);
    trace_assert(camera);

    for (size_t i = 0; i < boxes->count; ++i) {
        if (rigid_bodies_render(
                boxes->rigid_bodies,
                boxes->body_ids[i],
                boxes->body_colors[i],
                camera) < 0) {
            return -1;
        }
    }

    return 0;
}

int boxes_update(Boxes *boxes,
                 float delta_time)
{
    trace_assert(boxes);
    trace_assert(delta_time);

    for (size_t i = 0; i < boxes->count; ++i) {
        if (rigid_bodies_update(boxes->rigid_bodies, boxes->body_ids[i], delta_time) < 0) {
            return -1;
        }
    }

    return 0;
}

void boxes_float_in_lava(Boxes *boxes, Lava *lava)
{
    trace_assert(boxes);
    trace_assert(lava);

    for (size_t i = 0; i < boxes->count; ++i) {
        lava_float_rigid_body(lava, boxes->rigid_bodies, boxes->body_ids[i]);
    }
}

int boxes_add_box(Boxes *boxes, Rect rect, Color color)
{
    trace_assert(boxes);
    trace_assert(boxes->count < BOXES_CAPACITY);

    boxes->body_ids[boxes->count] = rigid_bodies_add(boxes->rigid_bodies, rect);
    boxes->body_colors[boxes->count] = color;
    boxes->count++;

    return 0;
}

struct EvalResult
boxes_send(Boxes *boxes, Gc *gc, struct Scope *scope, struct Expr path)
{
    trace_assert(boxes);
    trace_assert(gc);
    trace_assert(scope);

    struct Expr target = void_expr();
    struct Expr rest = void_expr();
    struct EvalResult res = match_list(gc, "e*", path, &target, &rest);
    if (res.is_error) {
        return res;
    }

    if (symbol_p(target)) {
        const char *action = target.atom->str;

        if (strcmp(action, "new") == 0) {
            struct Expr optional_args = void_expr();
            long int x, y, w, h;
            res = match_list(gc, "dddd*", rest, &x, &y, &w, &h, &optional_args);
            if (res.is_error) {
                return res;
            }

            Color color = rgba(rand_float(1.0f), rand_float(1.0f), rand_float(1.0f), 1.0f);
            if (!nil_p(optional_args)) {
                const char *color_hex = NULL;
                res = match_list(gc, "s*", optional_args, &color_hex, NULL);
                color = hexstr(color_hex);
            }

            boxes_add_box(boxes, rect((float) x, (float) y, (float) w, (float) h), color);

            return eval_success(NIL(gc));
        }

        return unknown_target(gc, "box", action);
    }

    return wrong_argument_type(gc, "string-or-symbol-p", target);
}


int boxes_delete_at(Boxes *boxes, Vec position)
{
    trace_assert(boxes);

    for (size_t i = 0; i < boxes->count; ++i) {
        const Rect hitbox = rigid_bodies_hitbox(
            boxes->rigid_bodies,
            boxes->body_ids[i]);
        if (rect_contains_point(hitbox, position)) {
            rigid_bodies_remove(boxes->rigid_bodies, boxes->body_ids[i]);
            for (size_t j = i; j < boxes->count - 1; ++j) {
                boxes->body_ids[j] = boxes->body_ids[j + 1];
            }
            boxes->count--;
            return 0;
        }
    }

    return 0;
}
