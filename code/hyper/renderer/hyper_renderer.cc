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

  void
  set_background_colour (renderer_context *context, colour colour)
  {
    colourise_pixels_unaligned_simd (context->_framebuffer->_pixels.data (),
                                     get_colour_uint (colour),
                                     context->_framebuffer->_simd_chunks);
  }

  void
  draw ([[maybe_unused]] renderer_context &context)
  {
  }
};
