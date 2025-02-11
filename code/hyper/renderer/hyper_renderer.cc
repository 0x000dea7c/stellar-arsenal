//
// Game software renderer. All routines here use the CPU, not the GPU.
// Since the game is in 2D exclusively, I'm working exclusively with
// world and screen coordinates.
//
#include "hyper_renderer.hh"

#include <immintrin.h>
#include <cassert>

namespace hyper
{
  static inline i32
  get_simd_width (void)
  {
    return 8;
  }

  static inline void
  colourise_pixel (Framebuffer *framebuffer, i32 x, i32 y, u32 colour)
  {
    framebuffer->pixels[y * framebuffer->width + x] = colour;
  }

  static inline void
  colourise_pixels_unaligned_simd (u32 *row, u32 colour, size_t chunks)
  {
    __m256i const colour_i = _mm256_set1_epi32 ((i32) colour);

    for (size_t i = 0; i < chunks; ++i)
      {
        _mm256_storeu_si256 ((__m256i*) row, colour_i);
        row += get_simd_width ();
      }
  }

  static void
  draw_horizontal_line_bresenham (Framebuffer *framebuffer, Vec2<i32> p0, Vec2<i32> p1, u32 colour, i32 dx, i32 dy, i32 dy_abs)
  {
    if (p1.x < p0.x)
      {
        hyper::swap (p0.x, p1.x);
        hyper::swap (p0.y, p1.y);
        dx = p1.x - p0.x;
        dy = p1.y - p0.y;
        dy_abs = hyper::abs (dy);
      }

    i32 D = 2 * dy - dx;
    i32 y = p0.y;
    i32 y_step = (dy < 0) ? -1 : 1;

    // TODO: simd
    for (i32 x = p0.x; x <= p1.x; ++x)
      {
        colourise_pixel (framebuffer, x, y, colour);

        if (D > 0)
          {
            y += y_step;
            D += 2 * (dy_abs - dx);
            continue;
          }

        D += 2 * dy_abs;
      }
  }

  static void
  draw_vertical_line_bresenham (Framebuffer *framebuffer, Vec2<i32> p0, Vec2<i32> p1, u32 colour, i32 dx, i32 dy, i32 dy_abs)
  {
    hyper::swap (p0.x, p0.y);
    hyper::swap (p1.x, p1.y);

    if (p1.x < p0.x)
      {
        hyper::swap (p0.x, p1.x);
        hyper::swap (p0.y, p1.y);
      }

    dx = p1.x - p0.x;
    dy = p1.y - p0.y;
    dy_abs = hyper::abs (dy);

    i32 D = 2 * dy - dx;
    i32 y = p0.y;
    i32 y_step = (dy < 0) ? -1 : 1;

    // TODO: SIMD
    for (i32 x = p0.x; x <= p1.x; ++x)
      {
        colourise_pixel (framebuffer, y, x, colour);

        if (D > 0)
          {
            y += y_step;
            D += 2 * (dy_abs - dx);
            continue;
          }

        D += 2 * dy_abs;
      }
  }

  static void
  draw_line_bresenham (Framebuffer *framebuffer, Vec2<i32> p0, Vec2<i32> p1, u32 colour)
  {
    i32 const dx = p1.x - p0.x;
    i32 const dy = p1.y - p0.y;
    i32 const dy_abs = hyper::abs (dy);
    i32 const dx_abs = hyper::abs (dx);
    bool const steep = dy_abs > dx_abs;

    if (steep)
      draw_vertical_line_bresenham (framebuffer, p0, p1, colour, dx, dy, dy_abs);
    else
      draw_horizontal_line_bresenham (framebuffer, p0, p1, colour, dx, dy, dy_abs);
  }

  static void
  draw_line_pixels (Framebuffer *framebuffer, Vec2<i32> p0, Vec2<i32> p1, u32 colour)
  {
    draw_line_bresenham (framebuffer, p0, p1, colour);
  }

  static void
  draw_triangle_outline_pixels (Framebuffer *framebuffer, std::array<Vec2<i32>, 3> const& triangle, u32 colour)
  {
    draw_line_pixels (framebuffer, triangle[0], triangle[1], colour);
    draw_line_pixels (framebuffer, triangle[1], triangle[2], colour);
    draw_line_pixels (framebuffer, triangle[2], triangle[0], colour);
  }

  void
  set_background_colour (Renderer_context *context, Colour colour)
  {
    colourise_pixels_unaligned_simd (context->framebuffer->pixels.data (),
                                     get_colour_uint (colour),
                                     context->framebuffer->simd_chunks);
  }

  void
  draw ([[maybe_unused]] Renderer_context *context)
  {
  }

  void
  draw_triangle_outline (Renderer_context *context, std::array<Vec2<f32>, 3> const &triangle, Colour colour)
  {
    // World to screen transformation
    std::array<Vec2<f32>, 3> triangle_screen_coordinates;
    for (size_t i = 0; i < triangle.size (); ++i)
      {
        triangle_screen_coordinates[i].x = (triangle[i].x - context->camera_x) * context->camera_zoom + context->screen_width;
        triangle_screen_coordinates[i].y = (triangle[i].y - context->camera_y) * context->camera_zoom + context->screen_height;
      }

    // Screen to pixels
    std::array<Vec2<i32>, 3> triangle_pixel_coordinates;
    for (size_t i = 0; i < triangle_pixel_coordinates.size (); ++i)
      {
        triangle_pixel_coordinates[i].x = static_cast<u32> (hyper::floor (triangle_screen_coordinates[i].x));
        triangle_pixel_coordinates[i].y = static_cast<u32> (hyper::floor (triangle_screen_coordinates[i].y));
      }

    draw_triangle_outline_pixels (context->framebuffer, triangle_pixel_coordinates, get_colour_uint (colour));
  }
};
