#include <stdbool.h>

#include "game/camera.h"
#include "game/level_metadata.h"
#include "game/level/boxes.h"
#include "game/level/level_editor/color_picker.h"
#include "game/level/level_editor/rect_layer.h"
#include "game/level/level_editor/point_layer.h"
#include "game/level/level_editor/player_layer.h"
#include "game/level/level_editor/label_layer.h"
#include "ui/edit_field.h"
#include "system/stacktrace.h"
#include "system/nth_alloc.h"
#include "system/lt.h"
#include "system/lt_adapters.h"
#include "system/log.h"
#include "system/str.h"

#include "level_editor.h"

#define LEVEL_LINE_MAX_LENGTH 512
#define LEVEL_EDITOR_EDIT_FIELD_SIZE vec(5.0f, 5.0f)
#define LEVEL_EDITOR_EDIT_FIELD_COLOR COLOR_BLACK

static int level_editor_dump(const LevelEditor *level_editor);

LevelEditor *create_level_editor(void)
{
    Lt *lt = create_lt();
    LevelEditor *level_editor = PUSH_LT(
        lt,
        nth_calloc(1, sizeof(LevelEditor)),
        free);
    if (level_editor == NULL) {
        RETURN_LT(lt, NULL);
    }
    level_editor->lt = lt;

    level_editor->edit_field_filename = PUSH_LT(
        lt,
        create_edit_field(
            LEVEL_EDITOR_EDIT_FIELD_SIZE,
            LEVEL_EDITOR_EDIT_FIELD_COLOR),
        destroy_edit_field);
    if (level_editor->edit_field_filename == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->metadata = PUSH_LT(
        lt,
        create_level_metadata("New Level"),
        destroy_level_metadata);
    if (level_editor->metadata == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->background_layer = create_color_picker_from_rgba(hexstr("fffda5"));

    level_editor->player_layer = PUSH_LT(
        lt,
        create_player_layer(vec(0.0f, 0.0f), hexstr("ff8080")),
        destroy_player_layer);
    if (level_editor->player_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->platforms_layer = PUSH_LT(
        lt,
        create_rect_layer(),
        destroy_rect_layer);
    if (level_editor->platforms_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->goals_layer = PUSH_LT(
        lt,
        create_point_layer(),
        destroy_point_layer);
    if (level_editor->goals_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->lava_layer = PUSH_LT(
        lt,
        create_rect_layer(),
        destroy_rect_layer);
    if (level_editor->lava_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->back_platforms_layer = PUSH_LT(
        lt,
        create_rect_layer(),
        destroy_rect_layer);
    if (level_editor->back_platforms_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->boxes_layer = PUSH_LT(
        lt,
        create_rect_layer(),
        destroy_rect_layer);
    if (level_editor->boxes_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->label_layer = PUSH_LT(
        lt,
        create_label_layer(),
        destroy_label_layer);
    if (level_editor->label_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->regions_layer = PUSH_LT(
        lt,
        create_rect_layer(),
        destroy_rect_layer);
    if (level_editor->regions_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->supa_script_source =
        PUSH_LT(lt, string_duplicate("", NULL), free);

    level_editor->layers[LAYER_PICKER_BOXES] = rect_layer_as_layer(level_editor->boxes_layer);
    level_editor->layers[LAYER_PICKER_PLATFORMS] = rect_layer_as_layer(level_editor->platforms_layer);
    level_editor->layers[LAYER_PICKER_BACK_PLATFORMS] = rect_layer_as_layer(level_editor->back_platforms_layer);
    level_editor->layers[LAYER_PICKER_GOALS] = point_layer_as_layer(level_editor->goals_layer);
    level_editor->layers[LAYER_PICKER_PLAYER] = player_layer_as_layer(level_editor->player_layer);
    level_editor->layers[LAYER_PICKER_LAVA] = rect_layer_as_layer(level_editor->lava_layer);
    level_editor->layers[LAYER_PICKER_REGIONS] = rect_layer_as_layer(level_editor->regions_layer);
    level_editor->layers[LAYER_PICKER_BACKGROUND] = color_picker_as_layer(&level_editor->background_layer);
    level_editor->layers[LAYER_PICKER_LABELS] = label_layer_as_layer(level_editor->label_layer);

    level_editor->drag = false;

    return level_editor;
}

LevelEditor *create_level_editor_from_file(const char *file_name)
{
    trace_assert(file_name);

    Lt *lt = create_lt();
    LevelEditor *level_editor = PUSH_LT(
        lt,
        nth_calloc(1, sizeof(LevelEditor)),
        free);
    if (level_editor == NULL) {
        RETURN_LT(lt, NULL);
    }
    level_editor->lt = lt;

    level_editor->edit_field_filename = PUSH_LT(
        lt,
        create_edit_field(
            LEVEL_EDITOR_EDIT_FIELD_SIZE,
            LEVEL_EDITOR_EDIT_FIELD_COLOR),
        destroy_edit_field);
    if (level_editor->edit_field_filename == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->file_name =
        PUSH_LT(
            lt,
            string_duplicate(file_name, NULL),
            free);

    LineStream *level_stream = PUSH_LT(
        lt,
        create_line_stream(
            file_name,
            "r",
            LEVEL_LINE_MAX_LENGTH),
        destroy_line_stream);
    if (level_stream == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->metadata = PUSH_LT(
        lt,
        create_level_metadata_from_line_stream(level_stream),
        destroy_level_metadata);
    if (level_editor->metadata == NULL) {
        RETURN_LT(lt, NULL);
    }

    if (color_picker_read_from_line_stream(
            &level_editor->background_layer,
            level_stream) < 0) {
        RETURN_LT(lt, NULL);
    }

    level_editor->player_layer =
        PUSH_LT(
            lt,
            create_player_layer_from_line_stream(level_stream),
            destroy_player_layer);
    if (level_editor->player_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->platforms_layer =
        PUSH_LT(
            lt,
            create_rect_layer_from_line_stream(level_stream),
            destroy_rect_layer);
    if (level_editor->platforms_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->goals_layer = PUSH_LT(
        lt,
        create_point_layer_from_line_stream(level_stream),
        destroy_point_layer);
    if (level_editor->goals_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->lava_layer =
        PUSH_LT(
            lt,
            create_rect_layer_from_line_stream(level_stream),
            destroy_rect_layer);
    if (level_editor->lava_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->back_platforms_layer =
        PUSH_LT(
            lt,
            create_rect_layer_from_line_stream(level_stream),
            destroy_rect_layer);
    if (level_editor->back_platforms_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->boxes_layer =
        PUSH_LT(
            lt,
            create_rect_layer_from_line_stream(level_stream),
            destroy_rect_layer);
    if (level_editor->boxes_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->label_layer =
        PUSH_LT(
            lt,
            create_label_layer_from_line_stream(level_stream),
            destroy_label_layer);
    if (level_editor->label_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->regions_layer =
        PUSH_LT(
            lt,
            create_rect_layer_from_line_stream(level_stream),
            destroy_rect_layer);
    if (level_editor->regions_layer == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->supa_script_source =
        PUSH_LT(
            lt,
            line_stream_collect_until_end(level_stream),
            free);
    if (level_editor->supa_script_source == NULL) {
        RETURN_LT(lt, NULL);
    }

    level_editor->layers[LAYER_PICKER_BOXES] = rect_layer_as_layer(level_editor->boxes_layer);
    level_editor->layers[LAYER_PICKER_PLATFORMS] = rect_layer_as_layer(level_editor->platforms_layer);
    level_editor->layers[LAYER_PICKER_BACK_PLATFORMS] = rect_layer_as_layer(level_editor->back_platforms_layer);
    level_editor->layers[LAYER_PICKER_GOALS] = point_layer_as_layer(level_editor->goals_layer);
    level_editor->layers[LAYER_PICKER_PLAYER] = player_layer_as_layer(level_editor->player_layer);
    level_editor->layers[LAYER_PICKER_LAVA] = rect_layer_as_layer(level_editor->lava_layer);
    level_editor->layers[LAYER_PICKER_REGIONS] = rect_layer_as_layer(level_editor->regions_layer);
    level_editor->layers[LAYER_PICKER_BACKGROUND] = color_picker_as_layer(&level_editor->background_layer);
    level_editor->layers[LAYER_PICKER_LABELS] = label_layer_as_layer(level_editor->label_layer);

    level_editor->drag = false;

    return level_editor;
}

void destroy_level_editor(LevelEditor *level_editor)
{
    trace_assert(level_editor);
    RETURN_LT0(level_editor->lt);
}

int level_editor_render(const LevelEditor *level_editor,
                        Camera *camera)
{
    trace_assert(level_editor);
    trace_assert(camera);

    if (camera_clear_background(camera, color_picker_rgba(&level_editor->background_layer)) < 0) {
        return -1;
    }

    for (size_t i = 0; i < LAYER_PICKER_N; ++i) {
        if (layer_render(
                level_editor->layers[i],
                camera,
                i == level_editor->layer_picker) < 0) {
            return -1;
        }
    }

    if (layer_picker_render(&level_editor->layer_picker, camera) < 0) {
        return -1;
    }

    if (level_editor->state == LEVEL_EDITOR_SAVEAS) {
        /* CSS */
        const Point size = LEVEL_EDITOR_EDIT_FIELD_SIZE;
        const char *save_as_text = "Save as: ";
        const Point position = vec(200.0f, 200.0f);
        const float save_as_width =
            (float) strlen(save_as_text) * FONT_CHAR_WIDTH * size.x;

        /* HTML */
        if (camera_render_text_screen(
                camera,
                save_as_text,
                LEVEL_EDITOR_EDIT_FIELD_SIZE,
                LEVEL_EDITOR_EDIT_FIELD_COLOR,
                position) < 0) {
            return -1;
        }

        if (edit_field_render(
                level_editor->edit_field_filename,
                camera,
                vec(position.x + save_as_width, position.y)) < 0) {
            return -1;
        }
    }

    return 0;
}

int level_editor_event(LevelEditor *level_editor,
                       const SDL_Event *event,
                       const Camera *camera)
{
    trace_assert(level_editor);
    trace_assert(event);
    trace_assert(camera);

    if (level_editor->state == LEVEL_EDITOR_SAVEAS) {
        switch (event->type) {
        case SDL_TEXTINPUT: {
            if (edit_field_text_input(
                    level_editor->edit_field_filename,
                    &event->text) < 0) {
                return -1;
            }
        } break;
        case SDL_KEYUP:
        case SDL_KEYDOWN: {
            if (event->key.keysym.sym == SDLK_RETURN) {
                trace_assert(level_editor->file_name == NULL);
                level_editor->file_name = PUSH_LT(
                    level_editor->lt,
                    string_duplicate(
                        edit_field_as_text(
                            level_editor->edit_field_filename),
                        NULL),
                    free);
                level_editor_dump(level_editor);
                SDL_StopTextInput();
                level_editor->state = LEVEL_EDITOR_EDITING;
            } else {
                if (edit_field_keyboard(
                        level_editor->edit_field_filename,
                        &event->key) < 0) {
                    return -1;
                }
            }
        } break;
        }

        return 0;
    } else {
        switch (event->type) {
        case SDL_KEYDOWN: {
            switch(event-> key.keysym.sym) {
            case SDLK_s: {
                /* TODO(#903): There is no indication that the level is saved when you press S in Level Editor */
                if (level_editor->file_name) {
                    level_editor_dump(level_editor);
                    log_info("Saving level to `%s`\n", level_editor->file_name);
                } else {
                    SDL_StartTextInput();
                    level_editor->state = LEVEL_EDITOR_SAVEAS;
                }
            } break;
            }
        } break;

        case SDL_MOUSEWHEEL: {
            // TODO(#679): zooming in edit mode is not smooth enough
            if (event->wheel.y > 0) {
                level_editor->camera_scale += 0.1f;
            } else if (event->wheel.y < 0) {
                level_editor->camera_scale = fmaxf(0.1f, level_editor->camera_scale - 0.1f);
            }
        } break;

        case SDL_MOUSEBUTTONUP:
        case SDL_MOUSEBUTTONDOWN: {
            if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_MIDDLE) {
                level_editor->drag = true;
            }

            if (event->type == SDL_MOUSEBUTTONUP && event->button.button == SDL_BUTTON_MIDDLE) {
                level_editor->drag = false;
            }
        } break;

        case SDL_MOUSEMOTION: {
            if (level_editor->drag) {
                const Vec next_position = camera_map_screen(camera, event->motion.x, event->motion.y);
                const Vec prev_position = camera_map_screen(
                    camera,
                    event->motion.x + event->motion.xrel,
                    event->motion.y + event->motion.yrel);

                vec_add(&level_editor->camera_position,
                        vec_sub(next_position, prev_position));
            }

        } break;
        }

        bool selected = false;
        if (layer_picker_event(
                &level_editor->layer_picker,
                event,
                camera,
                &selected) < 0) {
            return -1;
        }

        if (!selected) {
            if (layer_event(
                    level_editor->layers[level_editor->layer_picker],
                    event,
                    camera) < 0) {
                return -1;
            }
        }
    }

    return 0;
}

int level_editor_focus_camera(LevelEditor *level_editor,
                              Camera *camera)
{
    camera_center_at(camera, level_editor->camera_position);
    camera_scale(camera, level_editor->camera_scale);
    return 0;
}

/* TODO(#904): LevelEditor does not check that the saved level file is modified by external program */
static int level_editor_dump(const LevelEditor *level_editor)
{
    trace_assert(level_editor);

    FILE *filedump = PUSH_LT(
        level_editor->lt,
        fopen(level_editor->file_name, "w"),
        fclose_lt);

    if (fprintf(filedump, "%s\n", level_metadata_title(level_editor->metadata)) < 0) {
        return -1;
    }

    for (size_t i = 0; i < LAYER_PICKER_N; ++i) {
        if (layer_dump_stream(
                level_editor->layers[i],
                filedump) < 0) {
            return -1;
        }
    }

    fprintf(filedump, "%s", level_editor->supa_script_source);

    fclose(RELEASE_LT(level_editor->lt, filedump));

    return 0;
}
