#include <cstdlib>
#include <iostream>
#include <random>
#include <SDL3/SDL.h>

#include "stellar.hh"
#include "hyper_common.hh"
#include "hyper_memory_resources.hh"
#include "hyper.hh"
#include "hyper_renderer.hh"
#include "stellar_hot_reload.hh"
#include "stellar_game_logic.hh"
#include "hyper_stack_arena.hh"
#include "hyper_geometry.hh"

static void quit ();

// Constants
#define GAME_LOGIC_SHARED_LIBRARY_NAME "libgamelogic.so"
#define GAME_WORLD_WIDTH 1250.0f
#define GAME_WORLD_HEIGHT 937.5f

static f32 constexpr fixed_timestep = 1.0f / 60.0f;

// SDL globals
static SDL_Window *sdl_window = nullptr;
static SDL_Texture *sdl_texture = nullptr;
static SDL_Renderer *sdl_renderer = nullptr;

// Game globals
static stellar::Config game_config;
static stellar::State game_state;
static hyper::Fixed_memory_resource fixed_resource;
static std::array<std::byte, hyper::megabytes (128)> linear_arena_backing_buffer;
static std::array<std::byte, hyper::megabytes (32)> stack_arena_backing_buffer;
static hyper::Framebuffer game_framebuffer;
static hyper::Renderer_context game_renderer_context;
static hyper::Frame_context game_frame_context;
static stellar::Hot_reload_library_data game_logic_shared_library;
static stellar::World game_world;
static stellar::Camera game_camera;
static stellar::Game_data game_data;

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
  SDL_SetRenderVSync (sdl_renderer, game_config.vsync ? 0 : 1);
  game_config.vsync = !game_config.vsync;
}

static void
init (std::pmr::monotonic_buffer_resource &game_linear_arena, hyper::Stack_arena &stack_arena)
{
  // Initialise game config
  game_config.resolution.width = 1024;
  game_config.resolution.height = 768;
  game_config.target_fps = fixed_timestep;
  game_config.vsync = false;
  game_state.running = true;

  // Initialise SDL stuff using game's config
  if (!SDL_Init (SDL_INIT_VIDEO))
    panic ("SDL_Init", SDL_GetError ());

  sdl_window = SDL_CreateWindow ("Stellar Arsenal", game_config.resolution.width, game_config.resolution.height, 0);
  if (!sdl_window)
    panic ("SDL_CreateWindow", SDL_GetError ());

  sdl_renderer = SDL_CreateRenderer (sdl_window, nullptr);
  if (!sdl_renderer)
    panic ("SDL_CreateRenderer", SDL_GetError ());

  sdl_texture = SDL_CreateTexture (sdl_renderer,
                                   SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_STREAMING,
                                   game_config.resolution.width,
                                   game_config.resolution.height);
  if (!sdl_texture)
    panic ("SDL_CreateTexture", SDL_GetError ());

  // Frame and context
  game_framebuffer.width = game_config.resolution.width;
  game_framebuffer.height = game_config.resolution.height;
  game_framebuffer.pitch = game_framebuffer.width * (i32) sizeof (u32);
  std::pmr::vector<u32> data {&game_linear_arena};
  data.resize ((u32) game_framebuffer.width * (u32) game_framebuffer.height, 0x00);
  game_framebuffer.pixels = std::move (data);
  game_framebuffer.simd_chunks = game_framebuffer.pixels.size () / 8; // AVX2

  game_renderer_context.framebuffer = &game_framebuffer;
  game_renderer_context.stack_arena = &stack_arena;

  // Hot reloading mechanism
  if (!stellar::hot_reload_init (game_logic_shared_library, GAME_LOGIC_SHARED_LIBRARY_NAME))
    panic ("hot_reload_init", "couldn't initialise hot reloading");

  game_frame_context.renderer_context = &game_renderer_context;
  game_frame_context.physics_accumulator = 0.0f;
  game_frame_context.fixed_timestep = fixed_timestep;
  game_frame_context.alpha_rendering = 0.0f;
  game_frame_context.last_frame_time = SDL_GetTicks ();

  game_camera.x = static_cast<f32> (game_renderer_context.framebuffer->width >> 1);
  game_camera.y = static_cast<f32> (game_renderer_context.framebuffer->height >> 1);
  game_camera.zoom = 1.0f;

  game_world.width = GAME_WORLD_WIDTH;
  game_world.height = GAME_WORLD_HEIGHT;
  game_world.meters_per_pixel = 1.0f / game_camera.zoom; // FIXME: is this right?
  game_renderer_context.meters_per_pixel = game_world.meters_per_pixel;

  game_renderer_context.meters_per_pixel = game_world.meters_per_pixel;

  // Initialise stars
  std::random_device random_seed;
  std::mt19937 generator (random_seed ());
  std::uniform_real_distribution<f32> distribution_x (0, game_world.width);
  std::uniform_real_distribution<f32> distribution_y (0, game_world.height);
  for (size_t i = 0; i < game_data.stars.size (); ++i)
    {
      game_data.stars[i].body.center.x = distribution_x (generator);
      game_data.stars[i].body.center.y = distribution_y (generator);
      game_data.stars[i].body.radius = 1.0f;
      game_data.stars[i].colour = hyper::get_colour_from_preset (hyper::WHITE);
    }

  // Initialise ship
  game_data.ship.body.width = 20.0f;
  game_data.ship.body.height = 20.0f;

  game_data.ship.body.data.vertices[0] = { (game_world.width / 2.0f) - (game_data.ship.body.width / 2.0f),
                                           (game_world.height / 2.0f) + (game_data.ship.body.height / 2.0f) };

  game_data.ship.body.data.vertices[1] = { (game_world.width / 2.0f) + (game_data.ship.body.width / 2.0f),
                                           (game_world.height / 2.0f) + (game_data.ship.body.height / 2.0f) };

  game_data.ship.body.data.vertices[2] = { (game_world.width / 2.0f),
                                           (game_world.height / 2.0f) - (game_data.ship.body.height / 2.0f) };

  game_data.ship.body.colour = hyper::get_colour_from_preset (hyper::GREY);

  // Left wing
  game_data.ship.wings.left.vertices[0] = {
    game_data.ship.body.data.vertices[2].x,
    game_data.ship.body.data.vertices[2].y,
  };

  game_data.ship.wings.left.vertices[1] = {
    game_data.ship.wings.left.vertices[0].x - 80.0f,
    game_data.ship.wings.left.vertices[0].y + 70.0f
  };

  game_data.ship.wings.left.vertices[2] = {
    game_data.ship.body.data.vertices[0].x,
    game_data.ship.body.data.vertices[0].y,
  };

  // Right wing
  game_data.ship.wings.right.vertices[0] = {
    game_data.ship.body.data.vertices[2].x,
    game_data.ship.body.data.vertices[2].y,
  };

  game_data.ship.wings.right.vertices[1] = {
    game_data.ship.wings.right.vertices[0].x + 80.0f,
    game_data.ship.wings.right.vertices[0].y + 70.0f
  };

  game_data.ship.wings.right.vertices[2] = {
    game_data.ship.body.data.vertices[1].x,
    game_data.ship.body.data.vertices[1].y,
  };

  game_data.ship.wings.colour = hyper::get_colour_from_preset (hyper::GREY);

  // Cockpit
  game_data.ship.cockpit.width = 10.0f;
  game_data.ship.cockpit.height = 10.0f;

  game_data.ship.cockpit.data.vertices[0] = {
    game_data.ship.body.data.vertices[0].x + 5.0f,
    game_data.ship.body.data.vertices[2].y + 3.0f,
  };

  game_data.ship.cockpit.data.vertices[1] = {
    game_data.ship.cockpit.data.vertices[0].x + game_data.ship.cockpit.width,
    game_data.ship.cockpit.data.vertices[0].y,
  };

  game_data.ship.cockpit.data.vertices[2] = {
    game_data.ship.cockpit.data.vertices[0].x + game_data.ship.cockpit.width / 2.0f,
    game_data.ship.cockpit.data.vertices[0].y - game_data.ship.cockpit.height,
  };

  game_data.ship.cockpit.colour = hyper::get_colour_from_preset (hyper::GREY);

  // Thrusters
  game_data.ship.thrusters.width = 5.0f;
  game_data.ship.thrusters.height = 15.0f;

  // Left
  game_data.ship.thrusters.data[0] = {
    { game_data.ship.wings.left.vertices[0].x - 70.0f,
      game_data.ship.wings.left.vertices[0].y + 60.0f },
    game_data.ship.thrusters.width,
    game_data.ship.thrusters.height
  };

  // Right
  game_data.ship.thrusters.data[1] = {
    { game_data.ship.wings.right.vertices[0].x + 70.0f,
      game_data.ship.wings.right.vertices[0].y + 60.0f },
    game_data.ship.thrusters.width,
    game_data.ship.thrusters.height
  };

  game_data.ship.thrusters.colour = hyper::get_colour_from_preset (hyper::GREY);
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

  while (game_state.running)
    {
#if DEBUG
      if (stellar::hot_reload_library_was_updated ())
        stellar::hot_reload_load (game_logic_shared_library);
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

      game_frame_context.physics_accumulator += frame_time;

      while (SDL_PollEvent (&event))
        {
          if (event.type == SDL_EVENT_QUIT)
            {
              game_state.running = false;
              break;
            }

          if (event.type == SDL_EVENT_KEY_DOWN)
            {
              SDL_Keycode const key = event.key.key;

              switch (key)
                {
                case SDLK_ESCAPE:
                  game_state.running = false;
                  break;
                case SDLK_Q:
                  break;
                case SDLK_F1:
                  toggle_vsync ();
                  break;
                case SDLK_F2:
                  break;
                case SDLK_F3:
                  break;
                case SDLK_UP:
                  game_camera.y -= 150.0f * game_frame_context.fixed_timestep;
                  break;
                case SDLK_DOWN:
                  game_camera.y += 150.0f * game_frame_context.fixed_timestep;
                  break;
                case SDLK_LEFT:
                  game_camera.x -= 150.0f * game_frame_context.fixed_timestep;
                  break;
                case SDLK_RIGHT:
                  game_camera.x += 150.0f * game_frame_context.fixed_timestep;
                  break;
                default:
                  break;
                }
            }
        }

      // fixed timestep physics and logic updates
      while (game_frame_context.physics_accumulator >= game_frame_context.fixed_timestep)
        {

          game_logic_shared_library.update (game_frame_context, game_data);
          game_frame_context.physics_accumulator -= game_frame_context.fixed_timestep;
        }

      // render as fast as possible with interpolation
      game_renderer_context.camera_x = game_camera.x;
      game_renderer_context.camera_y = game_camera.y;
      game_renderer_context.camera_zoom = game_camera.zoom;
      game_frame_context.alpha_rendering = game_frame_context.physics_accumulator / game_frame_context.fixed_timestep;
      game_logic_shared_library.render (game_frame_context, game_data);

      // copy my updated framebuffer to the SDL texture
      SDL_UpdateTexture (sdl_texture, nullptr, game_framebuffer.pixels.data (), game_framebuffer.pitch);

      SDL_RenderClear (sdl_renderer);
      SDL_RenderTexture (sdl_renderer, sdl_texture, nullptr, nullptr);
      SDL_RenderPresent (sdl_renderer);

      ++frame_count;

      hyper::stack_arena_release (game_renderer_context.stack_arena);
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
  // The linear arena is for the framebuffer
  std::pmr::monotonic_buffer_resource game_linear_arena { linear_arena_backing_buffer.data (),
                                                          linear_arena_backing_buffer.size (),
                                                          &fixed_resource };

  // This is for scratch operations that only live for a particular frame
  hyper::Stack_arena_arguments stack_arena_arguments { &fixed_resource,
                                                       stack_arena_backing_buffer.data (),
                                                       stack_arena_backing_buffer.size ()};

  hyper::Stack_arena stack_arena {stack_arena_arguments};

  init (game_linear_arena, stack_arena);

  run ();

  quit ();

  return EXIT_SUCCESS;
}
