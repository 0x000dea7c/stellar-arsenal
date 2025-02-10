#include <cstdlib>
#include <iostream>
#include <SDL3/SDL.h>

#include "stellar.hh"
#include "hyper_common.hh"
#include "hyper_memory_resources.hh"
#include "hyper.hh"
#include "hyper_renderer.hh"
#include "stellar_hot_reload.hh"
#include "stellar_game_logic.hh"

static void quit ();

// Constants
#define GAME_LOGIC_SHARED_LIBRARY_NAME "libgamelogic.so"

static f32 constexpr fixed_timestep = 1.0f / 60.0f;

// SDL globals
static SDL_Window *sdl_window = nullptr;
static SDL_Texture *sdl_texture = nullptr;
static SDL_Renderer *sdl_renderer = nullptr;

// Game globals
static stellar::config game_config;
static stellar::state game_state;
static hyper::fixed_memory_resource fixed_resource;
static std::byte linear_arena_backing_buffer[hyper::megabytes (128)];
static hyper::framebuffer game_framebuffer;
static hyper::renderer_context game_renderer_context;
static hyper::frame_context game_frame_context;
static hyper::vertex_buffer game_vertex_buffer;
static stellar::hot_reload_library_data game_logic_shared_library;

// Internal functions
[[noreturn]] static void
panic (char const *title, char const *msg)
{
  std::cerr << title << '-' << msg;
  quit ();
  exit (EXIT_FAILURE);
}

static void
toggle_vsync (void)
{
  SDL_SetRenderVSync (sdl_renderer, game_config._vsync ? 0 : 1);
  game_config._vsync = !game_config._vsync;
  printf ("VSync value changed to: %d\n", game_config._vsync);
}

static void
init (std::pmr::monotonic_buffer_resource &game_linear_arena)
{
  // Initialise game config
  game_config.resolution._width = 1024;
  game_config.resolution._height = 768;
  game_config._target_fps = fixed_timestep;
  game_config._vsync = false;
  game_state._running = true;

  // Initialise SDL stuff using game's config
  if (!SDL_Init (SDL_INIT_VIDEO))
    panic ("SDL_Init", SDL_GetError ());

  sdl_window = SDL_CreateWindow ("Stellar Arsenal", game_config.resolution._width, game_config.resolution._height, 0);
  if (!sdl_window)
    panic ("SDL_CreateWindow", SDL_GetError ());

  sdl_renderer = SDL_CreateRenderer (sdl_window, nullptr);
  if (!sdl_renderer)
    panic ("SDL_CreateRenderer", SDL_GetError ());

  sdl_texture = SDL_CreateTexture (sdl_renderer,
                                   SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   game_config.resolution._width,
                                   game_config.resolution._height);
  if (!sdl_texture)
    panic ("SDL_CreateTexture", SDL_GetError ());

  // Frame and context
  game_framebuffer._width = game_config.resolution._width;
  game_framebuffer._height = game_config.resolution._height;
  game_framebuffer._pitch = game_framebuffer._width * (i32) sizeof (u32);
  std::pmr::vector<u32> data {&game_linear_arena};
  data.resize ((u32) game_framebuffer._width * (u32) game_framebuffer._height, 0x00);
  game_framebuffer._pixels = std::move (data);
  game_framebuffer._simd_chunks = game_framebuffer._pixels.size () / 8; // AVX2

  game_renderer_context._framebuffer = &game_framebuffer;
  game_renderer_context._vertex_buffer = &game_vertex_buffer;

  // Hot reloading mechanism
  if (!stellar::hot_reload_init (game_logic_shared_library, GAME_LOGIC_SHARED_LIBRARY_NAME))
    panic ("hot_reload_init", "couldn't initialise hot reloading");

  game_frame_context._renderer_context = &game_renderer_context;
  game_frame_context._physics_accumulator = 0.0f;
  game_frame_context._fixed_timestep = fixed_timestep;
  game_frame_context._alpha_rendering = 0.0f;
  game_frame_context._last_frame_time = SDL_GetTicks ();
}

static void
run ()
{
  // Run main game's loop
  u64 frame_count = 0;
  u64 last_time = SDL_GetTicks ();
  u64 fps_update_time = last_time;
  f32 current_fps = 0.0f;
  char window_title[32];
  SDL_Event event;

  // TEMP PROTOTYPE
  hyper::renderer_init (static_cast<f32> (game_config.resolution._width),
                        static_cast<f32> (game_config.resolution._height));

  stellar::camera game_camera;

  // NOTE: these values aren't random, especially the height. The
  // height is picked so that it maintains the same aspect ratio 4:3
  game_camera.fov._width = 5000.0f;
  game_camera.fov._height = 3750.0f;
  // In world space
  game_camera.position._x = 0.0f;
  game_camera.position._y = 0.0f;

  stellar::world game_world;
  game_world._width = 100'000.0f;
  game_world._height = 100'000.0f;
  game_world._meters_per_pixel = game_camera.fov._width / static_cast<f32> (game_config.resolution._width);
  // END PROTOTYPE

  while (game_state._running)
    {
#if 0
      if (stellar::hot_reload_library_was_updated ())
        {
          stellar::hot_reload_load (game_logic_shared_library);
          hyper::renderer_init (game_config.resolution._width, game_config.resolution._height);
        }
#endif
      u64 const current_time = SDL_GetTicks ();
      f32 frame_time = (f32) (current_time - last_time) / 1000.0f;
      last_time = current_time;

      // Cap max frame rate, avoid spiral of death, that is to say,
      // constantly trying to catch up if I miss a deadline
      if (frame_time > 0.25f)
        frame_time = 0.25f;

      // FPS display every second
      u64 const time_since_fps_update = current_time - fps_update_time;
      if (time_since_fps_update > 1000)
        {
          current_fps = (f32) frame_count * 1000.0f / (f32) time_since_fps_update;
          (void) snprintf (window_title, sizeof (window_title), "Stellar-Arsenal FPS: %.2f", current_fps);
          SDL_SetWindowTitle (sdl_window, window_title);
          frame_count = 0;
          fps_update_time = current_time;
        }

      game_frame_context._physics_accumulator += frame_time;

      while (SDL_PollEvent (&event))
        {
          if (event.type == SDL_EVENT_QUIT)
            {
              game_state._running = false;
              break;
            }

          if (event.type == SDL_EVENT_KEY_DOWN)
            {
              SDL_Keycode const key = event.key.key;

              switch (key)
                {
                case SDLK_ESCAPE:
                  game_state._running = false;
                  break;
                case SDLK_Q:
                  break;
                case SDLK_F1:
                  toggle_vsync ();
                  break;
                default:
                  break;
                }
            }
        }

      // fixed timestep physics and logic updates
      while (game_frame_context._physics_accumulator >= game_frame_context._fixed_timestep)
        {

          game_logic_shared_library._update (game_frame_context);
          game_frame_context._physics_accumulator -= game_frame_context._fixed_timestep;
        }

      // render as fast as possible with interpolation
      game_frame_context._alpha_rendering = game_frame_context._physics_accumulator / game_frame_context._fixed_timestep;
      game_logic_shared_library._render (game_frame_context);

      // copy my updated framebuffer to the SDL texture
      SDL_UpdateTexture (sdl_texture, nullptr, game_framebuffer._pixels.data (), game_framebuffer._pitch);

      SDL_RenderClear (sdl_renderer);
      SDL_RenderTexture (sdl_renderer, sdl_texture, nullptr, nullptr);
      SDL_RenderPresent (sdl_renderer);

      ++frame_count;
    }
}

static void
quit ()
{
  SDL_DestroyTexture (sdl_texture);
  SDL_DestroyRenderer (sdl_renderer);
  SDL_DestroyWindow (sdl_window);
  SDL_Quit ();
}

int
main ()
{
  // Memory allocations (all I'm going to have) inside the game
  std::pmr::monotonic_buffer_resource game_linear_arena {linear_arena_backing_buffer,
                                                         sizeof (linear_arena_backing_buffer),
                                                         &fixed_resource};

  init (game_linear_arena);

  run ();

  quit ();

  return EXIT_SUCCESS;
}
