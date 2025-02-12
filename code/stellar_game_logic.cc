#include "stellar_game_logic.hh"
#include "hyper_renderer.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

STELLAR_API void
game_update ([[maybe_unused]] hyper::Frame_context &context, [[maybe_unused]] stellar::Game_data &game_data)
{
  // here I'm going to do my physics update stuff
}

STELLAR_API void
game_render (hyper::Frame_context &context, stellar::Game_data &game_data)
{
  // Draw black background
  hyper::set_background_colour (context.renderer_context, hyper::get_colour_from_preset (hyper::BLACK));

  // Draw background stars (FIXME: blink stars)
  for (size_t i = 0; i < game_data.stars.size (); ++i)
    {
      hyper::draw_circle_filled (context.renderer_context,
                                 game_data.stars[i].body.center.x,
                                 game_data.stars[i].body.center.y,
                                 game_data.stars[i].body.radius,
                                 game_data.stars[i].colour);
    }

  // Draw body
  hyper::draw_triangle_outline (context.renderer_context, game_data.ship.body.data.vertices, game_data.ship.body.colour);
  // Left wing
  hyper::draw_triangle_outline (context.renderer_context, game_data.ship.wings.left.vertices, game_data.ship.wings.colour);
  // Right wing
  hyper::draw_triangle_outline (context.renderer_context, game_data.ship.wings.right.vertices, game_data.ship.wings.colour);

  // Draw cockpit
  hyper::draw_triangle_filled (context.renderer_context, game_data.ship.cockpit.data.vertices, game_data.ship.cockpit.colour);

  // Draw thrusters
  // Left
  hyper::draw_quad_filled (context.renderer_context,
                           game_data.ship.thrusters.data[0].position,
                           game_data.ship.thrusters.width,
                           game_data.ship.thrusters.height,
                           game_data.ship.thrusters.colour);
  // Right
  hyper::draw_quad_filled (context.renderer_context,
                           game_data.ship.thrusters.data[1].position,
                           game_data.ship.thrusters.width,
                           game_data.ship.thrusters.height,
                           game_data.ship.thrusters.colour);
}
