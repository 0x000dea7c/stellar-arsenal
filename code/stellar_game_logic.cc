#include "stellar_game_logic.hh"
#include "hyper_renderer.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

static std::array<hyper::vec2<f32>, 3> const triangle = {
  hyper::vec2<f32>{0.0f,  10.0f},
  hyper::vec2<f32>{10.0f, 10.0f},
  hyper::vec2<f32>{5.0f,   0.0f}
};

STELLAR_API void
game_update ([[maybe_unused]] hyper::frame_context &context)
{
  // here I'm going to do my physics update stuff
}

STELLAR_API void
game_render ([[maybe_unused]] hyper::frame_context &context)
{
  hyper::set_background_colour (context._renderer_context, hyper::get_colour_from_preset (hyper::BLACK));

  // TODO: draw stars manually (circles)

  // TODO: draw triangles, but in world space coordinates
  hyper::draw_triangle_outline (context._renderer_context, triangle, hyper::get_colour_from_preset (hyper::WHITE));
}
