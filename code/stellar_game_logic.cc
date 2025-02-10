#include "stellar_game_logic.hh"
#include "hyper_renderer.hh"
#include "hyper_colour.hh"

STELLAR_API void
game_update ([[maybe_unused]] hyper::frame_context &context)
{
  // here I'm going to do my physics update stuff
}

STELLAR_API void
game_render ([[maybe_unused]] hyper::frame_context &context)
{
  // here I'm going to do my rendering stuff
  hyper::set_background_colour (context._renderer_context, hyper::get_colour_from_preset (hyper::WHITE));
}
