#ifndef __BODY_H__
#define __BODY_H__

#include "gf2d_shape.h"
#include "gf2d_list.h"
#include "gf2d_text.h"

typedef struct Collision_S Collision;

typedef struct Body_S
{
    TextLine    name;           /**<name for debugging purposes*/
    int         inactive;       /**<internal use only*/
    float       gravity;        /**<the factor this body adheres to gravity*/
    Uint32      cliplayer;      /**<only bodies that share one or more layers will collide with each other*/
    Uint32      touchlayer;     /**<only bodies that share one or more layers will have their touch functions called*/
    Uint32      team;           /**<bodies that share a team will NOT interact*/
    Vector2D    position;       /**<position of the center of mass*/
    Vector2D    velocity;       /**<rate of change of position over time*/
    Vector2D    newvelocity;    /**<after a collision this is the new calculated velocity*/
    float       mass;           /**<used for inertia*/
    float       elasticity;     /**<how much bounce this body has*/
    Shape      *shape;          /**<which shape data will be used to collide for this body*/
    void       *data;           /**<custom data pointer*/
    int       (*bodyTouch)(struct Body_S *self, struct Body_S *other, Collision *collision);/**< function to call when two bodies collide*/
    int       (*worldTouch)(struct Body_S *self, Collision *collision);/**<function to call when a body collides with a static shape*/
}Body;


typedef struct
{
    Uint32      layer;          /**<layer mask to clip against*/
    Uint32      team;           /**<ignore any team ==*/
    Body       *ignore;         /**<if not null, the body will be ignored*/
}ClipFilter;


void gf2d_body_adjust_bounds_collision_velocity(Body *a,Vector2D poc, Vector2D normal);
void gf2d_body_adjust_collision_velocity(Body *a,Body *b,Vector2D poc, Vector2D normal);
void gf2d_body_adjust_static_bounce_velocity(Body *a,Shape *s,Vector2D poc, Vector2D normal);
void gf2d_body_adjust_collision_overlap(Body *a,float slop,Rect bounds);

/**
 * @brief draw a body to the screen.  Shape will be magenta, center point will be a green pixel
 * @param body the body to draw, a no-op if this is NULL
 * @param offset to adjust for camera or other position change relative to the body center
 */
void gf2d_body_draw(Body *body,Vector2D offset);

/**
 * check if body is within the bounds specified
 */
Uint8 gf2d_body_check_bounds(Body *body,Rect bounds,Vector2D *poc,Vector2D *normal);
Uint8 gf2d_body_collide_filter(Body *a,Body *b,Vector2D *poc, Vector2D *normal, ClipFilter filter);

/**
 * @brief initializes a body to zero
 * @warning do not use this on a body in use
 */
void gf2d_body_clear(Body *body);

/**
 * @brief set all parameters for a body
 * @param body the body to set the parameters for
 * @param name the name of the body
 * @param cliplayer the layer mask for bodies that clip each other
 * @param touchlayer the layer mask for what bodies to call the touch functions for
 * @param team the team
 * @param positition the position in space to be added at
 * @param velocity the velocity that the body is moving at
 * @param mass the mass of the body (for momentum purposes)
 * @param gravity the factor this body adheres to gravity
 * @param elasticity how much bounce this body has
 * @param shape a pointer to the shape data to use for the body
 * @param data any custom data you want associated with the body
 * @param bodyTouch the callback to invoke when this body touches another body
 * @param worldTouch the callback to invoke when this body touches the world
 */
void gf2d_body_set(
    Body *body,
    char       *name,
    Uint32      cliplayer,
    Uint32      touchlayer, 
    Uint32      team,
    Vector2D    position,
    Vector2D    velocity,
    float       mass,
    float       gravity,
    float       elasticity,
    Shape      *shape,
    void       *data,
    int     (*bodyTouch)(struct Body_S *self, struct Body_S *other, Collision *collision),
    int     (*worldTouch)(struct Body_S *self, Collision *collision));

/**
 * @brief apply a force to a body taking into account momentum
 * @param body the body to move
 * @param direction a unit vector for direction (Does not have to be)
 * @param force the amount of force to apply
 */
void gf2d_body_push(Body *body,Vector2D direction,float force);

/**
 * @brief get the shape, adjusted for position for the provided body
 * @param a the body to get the shape for
 * @return an empty {0} shape on error, or the body shape information otherwise
 */
Shape gf2d_body_to_shape(Body *a);

#endif
