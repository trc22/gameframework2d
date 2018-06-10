#include "gf2d_input.h"
#include "gf2d_list.h"
#include "simple_logger.h"
#include <simple_json.h>

static List *gf2d_input_list = NULL;
static const Uint8 * gf2d_input_keys = NULL;
static Uint8 * gf2d_input_old_keys = NULL;
static int gf2d_input_key_count = 0;

Input *gf2d_input_get_by_name(char *name);


void gf2d_input_delete(Input *in)
{
    if (!in)return;
    if (in->onDelete != NULL)
    {
        in->onDelete(in->data);
    }
    gf2d_list_delete(in->keyCodes);// data in the list is just integers
    free(in);
}

Input *gf2d_input_new()
{
    Input *in = NULL;
    in = (Input *)malloc(sizeof(Input));
    if (!in)
    {
        slog("Failed to allocate space for input");
        return NULL;
    }
    memset(in,0,sizeof(Input));
    in->keyCodes = gf2d_list_new();
    return in;
}

void gf2d_input_set_callbacks(
    char *command,
    void (*onPress)(void *data),
    void (*onHold)(void *data),
    void (*onRelease)(void *data),
    void (*onDelete)(void *data),
    void *data
)
{
    Input *in;
    if (!command)return;
    in = gf2d_input_get_by_name(command);
    if (!in)return;
    in->onPress = onPress;
    in->onHold = onHold;
    in->onRelease = onRelease;
    in->onDelete = onDelete;
    in->data = data;
}

void gf2d_input_close()
{
    gf2d_input_commands_purge();
    gf2d_list_delete(gf2d_input_list);
    gf2d_input_list = NULL;
    if (gf2d_input_old_keys)
    {
        free(gf2d_input_old_keys);
    }
}

void gf2d_input_commands_purge()
{
    Uint32 c,i;
    void *data;
    c = gf2d_list_get_count(gf2d_input_list);
    for (i = 0;i < c;i++)
    {
        data = gf2d_list_get_nth(gf2d_input_list,i);
        if (!data)continue;
        gf2d_input_delete((Input*)data);
    }
    gf2d_list_delete(gf2d_input_list);
    gf2d_input_list = gf2d_list_new();
}

void gf2d_input_update_command(Input *command)
{
    Uint32 c,i;
    SDL_Scancode kc;
    int old = 0, new = 0;
    if (!command)return;
    c = gf2d_list_get_count(command->keyCodes);
    if (!c)return;// no commands to update this with, do nothing
    for (i = 0; i < c; i++)
    {
#pragma GCC diagnostic ignored "-Wpointer-to-int-cast"
        kc = (SDL_Scancode)gf2d_list_get_nth(command->keyCodes,i);
        if (!kc)continue;
        old += gf2d_input_old_keys[kc];
        new += gf2d_input_keys[kc];
    }
    if ((old == c)&&(new == c))
    {
        command->state = IET_Hold;
        if (command->onHold)
        {
            command->onHold(command->data);
        }
    }
    else if ((old == c)&&(new != c))
    {
        command->state = IET_Release;
        if (command->onRelease)
        {
            command->onRelease(command->data);
        }
    }
    else if ((old != c)&&(new == c))
    {
        command->state = IET_Press;
        command->pressTime = SDL_GetTicks();
        if (command->onPress)
        {
            command->onPress(command->data);
        }
    }
    else
    {
        command->state = IET_Idle;
    }
}

Input *gf2d_input_get_by_name(char *name)
{
    Uint32 c,i;
    Input *in;
    if (!name)
    {
        return NULL;
    }
    c = gf2d_list_get_count(gf2d_input_list);
    for (i = 0;i < c;i++)
    {
        in = (Input *)gf2d_list_get_nth(gf2d_input_list,i);
        if (!in)continue;
        if (gf2d_line_cmp(in->command,name)==0)
        {
            return in;
        }
    }
    return 0;
}

Uint8 gf2d_input_command_pressed(char *command)
{
    Input *in;
    in = gf2d_input_get_by_name(command);
    if ((in)&&(in->state == IET_Press))return 1;
    return 0;
}

Uint8 gf2d_input_command_held(char *command)
{
    Input *in;
    in = gf2d_input_get_by_name(command);
    if ((in)&&(in->state == IET_Hold))return 1;
    return 0;
}

Uint8 gf2d_input_command_released(char *command)
{
    Input *in;
    in = gf2d_input_get_by_name(command);
    if ((in)&&(in->state == IET_Release))return 1;
    return 0;
}

InputEventType gf2d_input_command_get_state(char *command)
{
    Input *in;
    in = gf2d_input_get_by_name(command);
    if (!in)return 0;
    return in->state;
}


void gf2d_input_update()
{
    Uint32 c,i;
    void *data;

    memcpy(gf2d_input_old_keys,gf2d_input_keys,sizeof(Uint8)*gf2d_input_key_count);
    SDL_PumpEvents();   // update SDL's internal event structures
    gf2d_input_keys = SDL_GetKeyboardState(&gf2d_input_key_count);

    c = gf2d_list_get_count(gf2d_input_list);
    for (i = 0;i < c;i++)
    {
        data = gf2d_list_get_nth(gf2d_input_list,i);
        if (!data)continue;
        gf2d_input_update_command((Input*)data);        
    }
}

void gf2d_input_init(char *configFile)
{
    if (gf2d_input_list != NULL)
    {
        slog("gf2d_input_init: error, gf2d_input_list not NULL");
        return;
    }
    gf2d_input_list = gf2d_list_new();
    gf2d_input_keys = SDL_GetKeyboardState(&gf2d_input_key_count);
    if (!gf2d_input_key_count)
    {
        slog("failed to get keyboard count!");
    }
    else
    {
        gf2d_input_old_keys = (Uint8*)malloc(sizeof(Uint8)*gf2d_input_key_count);
        memcpy(gf2d_input_old_keys,gf2d_input_keys,sizeof(Uint8)*gf2d_input_key_count);
    }
    atexit(gf2d_input_close);
    gf2d_input_commands_load(configFile);
}

void gf2d_input_parse_command_json(SJson *command)
{
    SJson *value,*list;
    const char * buffer;
    Input *in;
    int count,i,F;
    SDL_Scancode kc = 0;
    if (!command)return;
    value = sj_object_get_value(command,"command");
    if (!value)
    {
        slog("input command missing 'command' key");
        return;
    }
    in = gf2d_input_new();
    if (!in)return;
    buffer = sj_get_string_value(value);
    gf2d_line_cpy(in->command,buffer);
    list = sj_object_get_value(command,"keys");
    count = sj_array_get_count(list);
    for (i = 0; i< count; i++)
    {
        value = sj_array_get_nth(list,i);
        if (!value)continue;
        buffer = sj_get_string_value(value);
        if (strlen(buffer) == 0)
        {
            slog("error in key list, empty value");
            continue;   //error
        }
        if (strlen(buffer) == 1)
        {
            //single letter code
            if ((buffer[0] >= 'a')&&(buffer[0] <= 'z'))
            {
                kc = SDL_SCANCODE_A + buffer[0] - 'a';
            }
            else if ((buffer[0] >= ' ')&&(buffer[0] <= '`'))
            {
                kc = SDL_SCANCODE_SPACE + buffer[0] - ' ';
            }

        }
        else
        {
            if (buffer[0] == 'F')
            {
                F = atoi(&buffer[1]);
                if (F <= 12)
                {
                   kc = SDL_SCANCODE_F1 + F - 1; 
                }
                else if (F <= 24)
                {
                   kc = SDL_SCANCODE_F13 + F - 1; 
                }
            }
            else if (strcmp(buffer,"LALT") == 0)
            {
                kc = SDL_SCANCODE_LALT;
            }
            else if (strcmp(buffer,"RALT") == 0)
            {
                kc = SDL_SCANCODE_RALT;
            }
            else if (strcmp(buffer,"LSHIFT") == 0)
            {
                kc = SDL_SCANCODE_LSHIFT;
            }
            else if (strcmp(buffer,"RSHIFT") == 0)
            {
                kc = SDL_SCANCODE_RSHIFT;
            }
            else if (strcmp(buffer,"LCTRL") == 0)
            {
                kc = SDL_SCANCODE_LCTRL;
            }
            else if (strcmp(buffer,"RCTRL") == 0)
            {
                kc = SDL_SCANCODE_RCTRL;
            }
            else if (strcmp(buffer,"TAB") == 0)
            {
                kc = SDL_SCANCODE_TAB;
            }
            else if (strcmp(buffer,"RETURN") == 0)
            {
                kc = SDL_SCANCODE_RETURN;
            }
            else if (strcmp(buffer,"BACKSPACE") == 0)
            {
                kc = SDL_SCANCODE_BACKSPACE;
            }
            else if (strcmp(buffer,"ESCAPE") == 0)
            {
                kc = SDL_SCANCODE_ESCAPE;
            }
        }
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
        gf2d_list_append(in->keyCodes,(void *)kc);
    }
    gf2d_list_append(gf2d_input_list,(void *)in);
}

void gf2d_input_commands_load(char *configFile)
{
    SJson *json;
    SJson *commands;
    SJson *value;
    int count,i;
    if (!configFile)return;
    json = sj_load(configFile);
    if (!json)return;
    commands = sj_object_get_value(json,"commands");
    if (!commands)
    {
        slog("config file %s does not contain 'commands' object",configFile);
        sj_free(json);
        return;
    }
    count = sj_array_get_count(commands);
    for (i = 0; i< count; i++)
    {
        value = sj_array_get_nth(commands,i);
        if (!value)continue;
        gf2d_input_parse_command_json(value);
    }
    sj_free(json);
}


/*eol@eof*/
