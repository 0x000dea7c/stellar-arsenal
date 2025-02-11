#include "stellar_game_logic.hh"
#include "hyper_renderer.hh"
#include "hyper_colour.hh"
#include "hyper_math.hh"

#include <array>

static std::array<hyper::Vec2<f32>, 3> const triangle = {
  hyper::Vec2<f32>{0.0f,  100.0f},
  hyper::Vec2<f32>{100.0f, 100.0f},
  hyper::Vec2<f32>{50.0f,   0.0f}
};

STELLAR_API void
game_update ([[maybe_unused]] hyper::Frame_context &context)
{
  // here I'm going to do my physics update stuff
}

STELLAR_API void
game_render (hyper::Frame_context &context)
{
  hyper::set_background_colour (context.renderer_context, hyper::get_colour_from_preset (hyper::BLACK));

  // TODO: draw stars manually (circles)

  // TODO: draw triangles, but in world space coordinates
  hyper::draw_triangle_outline (context.renderer_context, triangle, hyper::get_colour_from_preset (hyper::WHITE));
}
