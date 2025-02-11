#include "stellar_game_logic.hh"
#include "hyper_renderer.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

STELLAR_API void
game_update ([[maybe_unused]] hyper::Frame_context &context)
{
  // here I'm going to do my physics update stuff
}

STELLAR_API void
game_render (hyper::Frame_context &context, hyper::Game_data &game_data)
{
  // Draw black background
  hyper::set_background_colour (context.renderer_context, hyper::get_colour_from_preset (hyper::BLACK));

  // Draw background stars
  for (size_t i = 0; i < game_data.stars.size; ++i)
    {
      hyper::draw_circle_filled (context.renderer_context,
                                 game_data.stars.data[i].x,
                                 game_data.stars.data[i].y,
                                 1.0f,
                                 hyper::get_colour_from_preset (hyper::WHITE));
    }
}
