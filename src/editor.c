#include <stdio.h>

#include "simple_logger.h"

#include "gfc_types.h"
#include "gf2d_graphics.h"
#include "gf2d_windows.h"
#include "gf2d_elements.h"
#include "gf2d_element_label.h"
#include "gf2d_mouse.h"

#include "camera.h"
#include "actor_editor.h"
#include "windows_common.h"
#include "exhibits.h"
#include "exhibit_editor.h"
#include "scene.h"

extern void exitGame();
extern void exitCheck();

typedef enum
{
    EM_Exhibit,
    EM_Mask,
    EM_Layers,
    EM_MAX
}EditorModes;

typedef struct
{
    TextLine    filename;
    Scene      *scene;
    Exhibit    *selectedExhibit;
    Window     *exhibitEditor;
    TextLine    backgroundFileName;
    TextLine    backgroundActionName;
    Window     *subwindow;
    EditorModes editorMode;
}EditorData;


void onFileSaveCancel(void *Data)
{
    EditorData* data;
    if (!Data)return;
    data = Data;
    gfc_line_cpy(data->filename,data->scene->filename);
    return;
}

void onFileSaveOk(void *Data)
{
    EditorData* data;
    if (!Data)return;
    data = Data;
    gfc_line_cpy(data->scene->filename,data->filename);
    
    scene_save(data->scene, data->filename);
    
    return;
}


int editor_window_draw(Window *win)
{
    EditorData *data;
    if (!win->data)return 0;
    data = win->data;
    scene_draw(data->scene);
    gf2d_entity_draw_all();
    return 0;
}

int editor_window_free(Window *win)
{
    EditorData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    free(data);
    return 0;
}

void editor_deselect_exhibit(Window *win)
{
    EditorData *data;
    if (!win)return;
    data = (EditorData *)win->data;
    if ((!data)||(!data->selectedExhibit)||(!data->selectedExhibit->entity))return;
    data->selectedExhibit->entity->drawColor = gfc_color(0,0.5,0.5,1);
    data->selectedExhibit = NULL;
    if (data->exhibitEditor)
    {
        gf2d_window_free(data->exhibitEditor);
        data->exhibitEditor = NULL;
    }
}

void editor_select_exhibit(Window *win, Exhibit *exhibit)
{
    EditorData *data;
    Vector2D resolution;
    if ((!win)||(!exhibit))return;
    resolution = gf2d_graphics_get_resolution();
    editor_deselect_exhibit(win);
    data = (EditorData *)win->data;
    data->selectedExhibit = exhibit;
    exhibit->entity->drawColor = gfc_color(0,1,1,1);
    if (exhibit->rect.x > resolution.x / 2)
    {
        data->exhibitEditor = exhibit_editor(exhibit,vector2d(0,80));
    }
    else
    {
        data->exhibitEditor = exhibit_editor(exhibit,vector2d(resolution.x - 200,80));
    }
}

void onBackgroundActorChange(void *data)
{
    Window *win;
    EditorData *editor;
    Vector2D zero = {0};
    if (!data)return;
    win = data;
    if (!win)return;
    editor = win->data;
    if (!editor)return;
    gf2d_actor_free(&editor->scene->background);
    gfc_line_cpy(editor->scene->action,editor->backgroundActionName);
    gf2d_actor_load(&editor->scene->background,editor->backgroundFileName);
    gf2d_actor_set_action(&editor->scene->background,editor->scene->action);
    camera_set_bounds(0,0,editor->scene->background.size.x,editor->scene->background.size.y);
    camera_set_focus(zero);
    editor->subwindow = NULL;
}

void onBackgroundCancel(void *data)
{
    Window *win;
    EditorData *editor;
    if (!data)return;
    win = data;
    if (!win)return;
    editor = win->data;
    if (!editor)return;
    editor->subwindow = NULL;    
}

int editor_window_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    Exhibit *exhibit = NULL;
    TextLine label;
    EditorData *data;
    Vector2D mouse;
    if (!win)return 0;
    data = (EditorData *)win->data;
    mouse = gf2d_mouse_get_position();
    if (mouse.y < 8)
    {
        win->dimensions.y = 0;
    }
    else if (!gf2d_window_mouse_in(win))
    {
        win->dimensions.y = -win->dimensions.h;
    }
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        switch(e->index)
        {
            case 1000:
                data->editorMode = (data->editorMode + 1) % EM_MAX;
                switch (data->editorMode)
                {
                    case EM_MAX:
                    case EM_Exhibit:
                        gfc_line_sprintf(label, "Mode: Exhibit");
                        break;
                    case EM_Mask:
                        gfc_line_sprintf(label, "Mode: Mask");
                        break;
                    case EM_Layers:
                        gfc_line_sprintf(label, "Mode: Layers");
                        break;
                }
                gf2d_element_label_set_text(gf2d_window_get_element_by_id(win,1001),label);
                break;
            case 51:
                //background selector
                if (data->subwindow)break;
                if (data->scene->background.al != NULL)
                {
                    gfc_line_cpy(data->backgroundFileName,data->scene->background.al->filename);
                    gfc_line_cpy(data->backgroundActionName,gf2d_actor_get_action_name(&data->scene->background));
                }
                data->subwindow = actor_editor_menu(data->backgroundFileName,data->backgroundActionName,win, onBackgroundActorChange,onBackgroundCancel);
                break;
            case 53:
                // new exhibit
                exhibit = exhibit_new();
                scene_add_exhibit(data->scene,exhibit);
                scene_add_entity(data->scene, exhibit_entity_spawn(exhibit));
                editor_select_exhibit(win, exhibit);
                break;
            case 54:
                // save
                window_text_entry("Enter Scene to Load", data->filename, win->data, GFCLINELEN, onFileSaveOk,onFileSaveCancel);
                break;
            case 56:
                //exit
                exitCheck();
                return 1;
        }
    }
    if (gf2d_mouse_button_held(1))
    {
        camera_set_focus(mouse);
    }
    else if (gf2d_mouse_button_released(2))
    {
        switch (data->editorMode)
        {
            case EM_Exhibit:
                exhibit = exhibit_get_mouse_over_from_scene(data->scene);
                if (exhibit != NULL)
                {
                    editor_select_exhibit(win, exhibit);
                }
                else
                {
                    editor_deselect_exhibit(win);
                }
                break;
            case EM_Mask:
                break;
            default:
                break;
        }
    }
    return 0;
}

Window *editor_window(Scene * scene)
{
    Window *win;
    EditorData *data;
    win = gf2d_window_load("config/editor_window.json");
    if (!win)
    {
        slog("failed to load editor window");
        return NULL;
    }
    win->update = editor_window_update;
    win->free_data = editor_window_free;
    win->draw = editor_window_draw;
    data = (EditorData*)gfc_allocate_array(sizeof(EditorData),1);
    gfc_line_cpy(data->filename,scene->filename);
    win->data = data;
    data->scene = scene;

    return win;
}

/*
 * 
 * Intro menu for the editor
 * 
 */

typedef struct
{
    TextLine filename;
    Window *editorMenu;
}EditorMenuData;

int editor_menu_free(Window *win)
{
    EditorMenuData *data;
    if (!win)return 0;
    if (!win->data)return 0;
    data = win->data;
    free(data);

    return 0;
}

void onFileNameCancel(void *Data)
{
    EditorMenuData* data;
    if (!Data)return;
    data = Data;
    gfc_line_cpy(data->filename,"scenes/");
    return;
}

void onFileNameOk(void *Data)
{
    EditorMenuData* data;
    Scene *scene;
    if (!Data)return;
    data = Data;
    
    scene = scene_load(data->filename);
    if (!scene)
    {
        window_alert("File not found", data->filename,NULL,NULL);
        onFileNameCancel(Data);
        return;
    }
    editor_window(scene);
    scene_spawn_exhibits(scene);

    gf2d_window_free(data->editorMenu);
    return;
}

int editor_menu_update(Window *win,List *updateList)
{
    int i,count;
    Element *e;
    EditorMenuData* data;
    if (!win)return 0;
    if (!updateList)return 0;
    data = (EditorMenuData*)win->data;
    if (!data)return 0;
    count = gfc_list_get_count(updateList);
    for (i = 0; i < count; i++)
    {
        e = gfc_list_get_nth(updateList,i);
        if (!e)continue;
        switch(e->index)
        {
            case 51:
                window_text_entry("Enter Scene to Load", data->filename, win->data, GFCLINELEN, onFileNameOk,onFileNameCancel);
                return 1;
            case 52:
                editor_window(scene_new());
                gf2d_window_free(win);
                return 1;
            case 53:
                exitGame();
                gf2d_window_free(win);
                return 1;
        }
    }
    return 0;
}


Window *editor_menu()
{
    Window *win;
    EditorMenuData* data;
    win = gf2d_window_load("config/editor_menu.json");
    if (!win)
    {
        slog("failed to load editor menu");
        return NULL;
    }
    win->update = editor_menu_update;
    win->free_data = editor_menu_free;
    data = (EditorMenuData*)gfc_allocate_array(sizeof(EditorMenuData),1);
    data->editorMenu = win;
    gfc_line_cpy(data->filename,"scenes/");
    win->data = data;
    return win;
}


/*eol@eof*/
