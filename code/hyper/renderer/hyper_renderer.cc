#include "hyper_renderer.hh"

#include <immintrin.h>
#include <cassert>

// FIXME: these need to get passed as arguments!
static hyper::mat4 perspective;
static f32 screen_width;
static f32 screen_height;

namespace hyper
{
  static inline i32
  get_simd_width (void)
  {
    return 8;
  }

  static inline void
  colourise_pixel (framebuffer *framebuffer, i32 x, i32 y, u32 colour)
  {
    framebuffer->_pixels[y * framebuffer->_width + x] = colour;
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
  draw_horizontal_line_bresenham (framebuffer *framebuffer, vec2<i32> p0, vec2<i32> p1, u32 colour, i32 dx, i32 dy, i32 dy_abs)
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
  draw_vertical_line_bresenham (framebuffer *framebuffer, vec2<i32> p0, vec2<i32> p1, u32 colour, i32 dx, i32 dy, i32 dy_abs)
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
  draw_line_bresenham (framebuffer *framebuffer, vec2<i32> p0, vec2<i32> p1, u32 colour)
  {
    i32 const dx = p1.x - p0.x;
    i32 const dy = p1.y - p0.y;
    i32 const dy_abs = hyper::abs (dy);
    i32 const dx_abs = hyper::abs (dx);
    bool const steep = dy_abs > dx_abs;

    if (steep)
      {
        draw_vertical_line_bresenham (framebuffer, p0, p1, colour, dx, dy, dy_abs);
        return;
      }

    draw_horizontal_line_bresenham (framebuffer, p0, p1, colour, dx, dy, dy_abs);
  }

  static void
  draw_line_pixels (framebuffer *framebuffer, vec2<i32> p0, vec2<i32> p1, u32 colour)
  {
    draw_line_bresenham (framebuffer, p0, p1, colour);
  }

  static void
  draw_triangle_outline_pixels (framebuffer *framebuffer, std::array<vec2<i32>, 3> const& triangle, u32 colour)
  {
    draw_line_pixels (framebuffer, triangle[0], triangle[1], colour);
    draw_line_pixels (framebuffer, triangle[1], triangle[2], colour);
    draw_line_pixels (framebuffer, triangle[2], triangle[0], colour);
  }

  void
  renderer_init (f32 width, f32 height)
  {
    perspective = hyper::orthographic (0.0f, width, height, 0.0f, -1.0f, 1.0f);
    screen_width = width;
    screen_height = height;
  }

  void
  set_background_colour (renderer_context *context, colour colour)
  {
    colourise_pixels_unaligned_simd (context->_framebuffer->_pixels.data (),
                                     get_colour_uint (colour),
                                     context->_framebuffer->_simd_chunks);
  }

  void
  draw ([[maybe_unused]] renderer_context *context)
  {
  }

  void
  draw_triangle_outline (renderer_context *context, std::array<vec2<f32>, 3> const &triangle, colour colour)
  {
    assert (screen_height != 0.0f);

    // TODO: assume camera is always at (0, 0)
    std::array<vec4<f32>, 3> triangle_screen_coordinates = {
      vec4<f32>{ triangle[0].x, triangle[0].y, 0.0f, 1.0f },
      vec4<f32>{ triangle[1].x, triangle[1].y, 0.0f, 1.0f },
      vec4<f32>{ triangle[2].x, triangle[2].y, 0.0f, 1.0f }
    };

    // Transform to NDC (multiply by perspective matrix and perspective divide)
    for (size_t i = 0; i < 3; ++i)
      triangle_screen_coordinates[i] = mat4_vec4_mul (perspective, triangle_screen_coordinates[i]);

    for (size_t i = 0; i < 3; ++i)
      {
        triangle_screen_coordinates[i].x /= triangle_screen_coordinates[i].w;
        triangle_screen_coordinates[i].y /= triangle_screen_coordinates[i].w;
      }

    // Transform NDC to screen space (assume top left is 0, 0)
    // [-1, 1] in X maps to [0, screen_width]
    // [-1, 1] in Y maps to [screen_height, 0]
    for (size_t i = 0; i < 3; ++i)
      {
        triangle_screen_coordinates[i].x = (triangle_screen_coordinates[i].x + 1.0f) * 0.5f * screen_width;
        triangle_screen_coordinates[i].y = (triangle_screen_coordinates[i].y - 1.0f) * 0.5f * screen_height;
      }

    // TODO: clip, but assume this case fits for now

    // Call routines to draw with integers, not floating points I have
    // a horizontal array of pixels. The left edge of the leftmost
    // pixel is 0.0; the center of this pixel is 0.5. So:
    // d = floor (c)
    // c = d + 0.5
    // d -> discrete integer index of the pixel
    // c -> continuous (floating point) value within the pixel
    std::array<vec2<i32>, 3> triangle_pixels;

    for (size_t i = 0; i < 3; ++i)
      {
        triangle_pixels[i].x = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].x));
        triangle_pixels[i].y = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].y));
      }

    draw_triangle_outline_pixels (context->_framebuffer, triangle_pixels, get_colour_uint (colour));
  }
};
