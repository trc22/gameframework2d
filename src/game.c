#include <SDL.h>
#include "simple_logger.h"
#include "gf2d_graphics.h"
#include "gf2d_sprite.h"
#include "entity.h"
#include "bug.h"
#include "level.h"


int main(int argc, char * argv[])
{
    /*variable declarations*/
    int done = 0;
    const Uint8 * keys;
    Sprite *mouse;
    
    int mx,my;
    float mf = 0;
    Vector4D mouseColor = {255,100,255,200};
    SDL_Rect bounds = {0,0,1200,720};
    
    Level *level;
    Entity *bug;
    /*program initializtion*/
    init_logger("gf2d.log");
    slog("---==== BEGIN ====---");
    gf2d_graphics_initialize(
        "gf2d",
        1200,
        720,
        1200,
        720,
        vector4d(0,0,0,255),
        0);
    gf2d_graphics_set_frame_delay(16);
    gf2d_sprite_init(1024);
    SDL_ShowCursor(SDL_DISABLE);
    entity_manager_init(1024);
    
    /*demo setup*/
    level = level_new("images/backgrounds/bg_flat.png",bounds);
    mouse = gf2d_sprite_load_all("images/pointer.png",32,32,16);
    /*main game loop*/
    bug = bug_new(vector2d(100,100),vector2d(gfc_crandom(),gfc_crandom()));
    bug_new(vector2d(200,100),vector2d(gfc_crandom(),gfc_crandom()));
    bug_new(vector2d(200,200),vector2d(gfc_crandom(),gfc_crandom()));
    bug_new(vector2d(100,200),vector2d(gfc_crandom(),gfc_crandom()));
    while(!done)
    {
        SDL_PumpEvents();   // update SDL's internal event structures
        keys = SDL_GetKeyboardState(NULL); // get the keyboard state for this frame
        /*update things here*/
        SDL_GetMouseState(&mx,&my);
        mf+=0.1;
        if (mf >= 16.0)mf = 0;
        entity_update_all();
        
        gf2d_graphics_clear_screen();// clears drawing buffers
        // all drawing should happen betweem clear_screen and next_frame
            //backgrounds drawn first
            level_draw(level);
            entity_draw_all();          
            //UI elements last
            gf2d_sprite_draw(
                mouse,
                vector2d(mx,my),
                NULL,
                NULL,
                NULL,
                NULL,
                &mouseColor,
                (int)mf);
        gf2d_grahics_next_frame();// render current draw frame and skip to the next frame
        
        if (keys[SDL_SCANCODE_ESCAPE])done = 1; // exit condition
        if (keys[SDL_SCANCODE_SPACE])entity_free(bug); // exit condition
//        slog("Rendering at %f FPS",gf2d_graphics_get_frames_per_second());
    }
    level_free(level);
    slog("---==== END ====---");
    return 0;
}
/*eol@eof*/
