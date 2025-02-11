//
// Game software renderer. All routines here use the CPU, not the GPU.
// Since the game is in 2D exclusively, I'm working exclusively with
// world and screen coordinates.
//
#include "hyper_renderer.hh"
#include "hyper_stack_arena.hh"

#include <immintrin.h>
#include <cassert>

namespace hyper
{
  static inline i32
  get_simd_width ()
  {
    return 8;
  }

  static inline void
  set_pixel_colour (Framebuffer *framebuffer, i32 x, i32 y, u32 colour)
  {
    framebuffer->pixels[y * framebuffer->width + x] = colour;
  }

  static inline void
  set_pixels_colour_unaligned_simd (u32 *row, u32 colour, size_t chunks)
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
        set_pixel_colour (framebuffer, x, y, colour);

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
        set_pixel_colour (framebuffer, y, x, colour);

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

  static std::pmr::vector<i32>
  interpolate_array (Stack_arena *stack_arena, i32 y0, i32 x0, i32 y1, i32 x1, i32 points)
  {
    // PLEASE USE RVO, PLEASE
    std::pmr::vector<i32> x_values {&stack_arena->resource};
    x_values.resize (points);

    if (y0 > y1)
      {
        hyper::swap (y0, y1);
        hyper::swap (x0, x1);
      }

    i32 const dx = x1 - x0;
    i32 const dy = y1 - y0;

    if (dy == 0)
      {
        for (int32_t i = 0; i < points; ++i)
          x_values[i] = (x0 + dx * i) / points;
      }
    else
      {
        for (int32_t i = 0; i < points; ++i)
          x_values[i] = x0 + ((dx * i) / dy);
      }

    return x_values;
  }

  static std::pmr::vector<i32>
  vector_append (std::pmr::vector<i32> &array0, std::pmr::vector<i32> &array1)
  {
    // PLEASE USE RVO
    // Don't include the last pixel of array0 (it'd be repeated)
    size_t const n = (array0.size () - 1) + array1.size ();
    size_t j = 0;

    array0.resize (n);

    for (size_t i = array0.size (); i < n; ++i)
      array0[i] = array1[j++];

    return array0;
  }

  void
  set_background_colour (Renderer_context *context, Colour colour)
  {
    set_pixels_colour_unaligned_simd (context->framebuffer->pixels.data (),
                                      get_colour_uint (colour),
                                      context->framebuffer->simd_chunks);
  }

  static inline void
  plot_points (Framebuffer *framebuffer, i32 circle_center_x, i32 circle_center_y, i32 px, i32 py, u32 colour)
  {
    // each point I compute gives me 8 points on the circle (symmetry)
    // octant 1
    set_pixel_colour(framebuffer, (circle_center_x + px), (circle_center_y + py), colour);
    // octant 2
    set_pixel_colour(framebuffer, (circle_center_x + py), (circle_center_y + px), colour);
    // octant 3
    set_pixel_colour(framebuffer, (circle_center_x - py), (circle_center_y + px), colour);
    // octant 4
    set_pixel_colour(framebuffer, (circle_center_x - px), (circle_center_y + py), colour);
    // octant 5
    set_pixel_colour(framebuffer, (circle_center_x - px), (circle_center_y - py), colour);
    // octant 6
    set_pixel_colour(framebuffer, (circle_center_x - py), (circle_center_y - px), colour);
    // octant 7
    set_pixel_colour(framebuffer, (circle_center_x + py), (circle_center_y - px), colour);
    // octant 8
    set_pixel_colour(framebuffer, (circle_center_x + px), (circle_center_y - py), colour);
  }

  void
  draw_triangle_outline (Renderer_context *context, std::array<Vec2<f32>, 3> const &triangle, Colour colour)
  {
    // World to screen transformation
    std::array<Vec2<f32>, 3> triangle_screen_coordinates;
    for (size_t i = 0; i < triangle.size (); ++i)
      {
        triangle_screen_coordinates[i].x = (triangle[i].x - context->camera_x) * context->camera_zoom + (static_cast<f32> (context->framebuffer->width >> 1));
        triangle_screen_coordinates[i].y = (triangle[i].y - context->camera_y) * context->camera_zoom + (static_cast<f32> (context->framebuffer->height >> 1));
      }

    // Screen to pixels
    std::array<Vec2<i32>, 3> triangle_pixel_coordinates;
    for (size_t i = 0; i < triangle_pixel_coordinates.size (); ++i)
      {
        triangle_pixel_coordinates[i].x = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].x));
        triangle_pixel_coordinates[i].y = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].y));
      }

    draw_triangle_outline_pixels (context->framebuffer, triangle_pixel_coordinates, get_colour_uint (colour));
  }

  void
  draw_triangle_filled (Renderer_context *context, std::array<Vec2<f32>, 3> const &triangle, Colour colour)
  {
    // World to screen transformation
    std::array<Vec2<f32>, 3> triangle_screen_coordinates;
    for (size_t i = 0; i < triangle.size (); ++i)
      {
        triangle_screen_coordinates[i].x = (triangle[i].x - context->camera_x) * context->camera_zoom + (static_cast<f32> (context->framebuffer->width >> 1));
        triangle_screen_coordinates[i].y = (triangle[i].y - context->camera_y) * context->camera_zoom + (static_cast<f32> (context->framebuffer->height >> 1));
      }

    // Screen to pixels
    std::array<Vec2<i32>, 3> triangle_pixel_coordinates;
    for (size_t i = 0; i < triangle_pixel_coordinates.size (); ++i)
      {
        triangle_pixel_coordinates[i].x = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].x));
        triangle_pixel_coordinates[i].y = static_cast<i32> (hyper::floor (triangle_screen_coordinates[i].y));
      }

    u32 const colour_uint = get_colour_uint (colour);

    // sort vertices so that the first vertex is always at the top
    if (triangle_pixel_coordinates[1].y < triangle_pixel_coordinates[0].y)
      hyper::swap (triangle_pixel_coordinates[0], triangle_pixel_coordinates[1]);

    if (triangle_pixel_coordinates[2].y < triangle_pixel_coordinates[0].y)
      hyper::swap (triangle_pixel_coordinates[2], triangle_pixel_coordinates[0]);

    if (triangle_pixel_coordinates[2].y < triangle_pixel_coordinates[1].y)
      hyper::swap (triangle_pixel_coordinates[2], triangle_pixel_coordinates[1]);

    i32 const x01_length = triangle_pixel_coordinates[1].y - triangle_pixel_coordinates[0].y + 1;
    i32 const x12_length = triangle_pixel_coordinates[2].y - triangle_pixel_coordinates[1].y + 1;
    i32 const x02_length = triangle_pixel_coordinates[2].y - triangle_pixel_coordinates[0].y + 1;

    auto x01 = interpolate_array (context->stack_arena, triangle_pixel_coordinates[0].y, triangle_pixel_coordinates[0].x, triangle_pixel_coordinates[1].y, triangle_pixel_coordinates[1].x, x01_length);
    auto x12 = interpolate_array (context->stack_arena, triangle_pixel_coordinates[1].y, triangle_pixel_coordinates[1].x, triangle_pixel_coordinates[2].y, triangle_pixel_coordinates[2].x, x12_length);
    auto x02 = interpolate_array (context->stack_arena, triangle_pixel_coordinates[0].y, triangle_pixel_coordinates[0].x, triangle_pixel_coordinates[2].y, triangle_pixel_coordinates[2].x, x02_length);
    auto x012 = vector_append (x01, x12);

    // determine which array is the left and which one is the right
    size_t const mid = (x01.size () + x12.size ()) >> 1;
    auto x_left = x012.begin ();
    auto x_right = x02.begin ();

    if (x02[mid] < x012[mid])
      {
        x_left = x02.begin ();
        x_right = x012.begin ();
      }

    for (i32 y = triangle_pixel_coordinates[0].y; y <= triangle_pixel_coordinates[2].y; ++y)
      {
        i32 const x_start = x_left[y - triangle_pixel_coordinates[0].y];
        i32 const x_end = x_right[y - triangle_pixel_coordinates[0].y];
        i32 const span  = x_end - x_start + 1;
        i32 const chunks = span / get_simd_width ();

        if (chunks > 0)
          {
            u32 *row = &context->framebuffer->pixels[y * context->framebuffer->width + x_start];

            set_pixels_colour_unaligned_simd (row, colour_uint, chunks);

            // leftovers
            for (i32 x = x_start + (chunks * get_simd_width ()); x <= x_end; ++x)
              set_pixel_colour (context->framebuffer, x, y, colour_uint);

            continue;
          }

        // get here if I can't use SIMD
        for (i32 x = x_start; x <= x_end; ++x)
          set_pixel_colour (context->framebuffer, x, y, colour_uint);
      }
  }

  void
  draw_circle_outline (Renderer_context *context, f32 x, f32 y, f32 radius, Colour colour)
  {
    u32 const colour_uint = get_colour_uint (colour);

    // World to screen transformation
    f32 const circle_screen_coordinates_x = (x - context->camera_x) * context->camera_zoom + (static_cast<f32> (context->framebuffer->width >> 1));
    f32 const circle_screen_coordinates_y = (y - context->camera_y) * context->camera_zoom + (static_cast<f32> (context->framebuffer->height >> 1));

    // Screen to pixels
    i32 const circle_pixel_coordinates_x = static_cast<i32> (hyper::floor (circle_screen_coordinates_x));
    i32 const circle_pixel_coordinates_y = static_cast<i32> (hyper::floor (circle_screen_coordinates_y));
    i32 const radius_pixels = static_cast<i32> (radius * context->meters_per_pixel);

    // Start at the top!
    Vec2<i32> current = { 0, radius_pixels };
    i32 D = 3 - (2 * radius_pixels);

    plot_points (context->framebuffer, circle_pixel_coordinates_x, circle_pixel_coordinates_y, current.x, current.y, colour_uint);

    while (current.y > current.x)
      {
        /* move inward or not */
        if (D > 0)
          {
            --current.y;
            D = D + 4 * (current.x - current.y) + 10;
          }
        else
          D = D + 4 * current.x + 6;

        ++current.x;
        plot_points (context->framebuffer, circle_pixel_coordinates_x, circle_pixel_coordinates_y, current.x, current.y, colour_uint);
      }
  }

  void
  draw_circle_filled (Renderer_context *context, f32 circle_center_x, f32 circle_center_y, f32 radius, Colour colour)
  {
    // World to screen transformation
    f32 const circle_screen_coordinates_x = (circle_center_x - context->camera_x) * context->camera_zoom + (static_cast<f32> (context->framebuffer->width >> 1));
    f32 const circle_screen_coordinates_y = (circle_center_y - context->camera_y) * context->camera_zoom + (static_cast<f32> (context->framebuffer->height >> 1));

    // Screen to pixels
    i32 const circle_pixel_coordinates_x = static_cast<i32> (hyper::floor (circle_screen_coordinates_x));
    i32 const circle_pixel_coordinates_y = static_cast<i32> (hyper::floor (circle_screen_coordinates_y));
    i32 const radius_pixels = static_cast<i32> (radius * context->meters_per_pixel);

    u32 const colour_uint = get_colour_uint (colour);
    i32 const radius_squared = radius_pixels * radius_pixels;
    i32 const y_start = hyper::max (circle_pixel_coordinates_y - radius_pixels, 0);
    i32 const y_end = hyper::min (circle_pixel_coordinates_y + radius_pixels, context->framebuffer->height - 1);

    for (i32 y = y_start; y <= y_end; ++y)
      {
        i32 const dy = y - circle_pixel_coordinates_y;
        i32 const width_squared = radius_squared - (dy * dy);

        if (width_squared < 0)
          continue;

        i32 const width = static_cast<i32> (hyper::sqrt (static_cast<f32> (width_squared)));
        i32 const x_start = hyper::max (circle_pixel_coordinates_x - width, 0);
        i32 const x_end = hyper::min (circle_pixel_coordinates_x + width, context->framebuffer->width - 1);

        for (i32 x = x_start; x <= x_end; ++x)
          set_pixel_colour (context->framebuffer, x, y, colour_uint);
      }
  }
};
