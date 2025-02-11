#pragma once

#include "hyper_common.hh"
#include "hyper_memory_resources.hh"

#include <memory_resource>

namespace hyper
{
  struct Stack_arena_arguments
  {
    Fixed_memory_resource *resource;
    std::byte *backing_buffer;
    u32 size;
  };

  struct Stack_arena
  {
    explicit Stack_arena (Stack_arena_arguments const &args)
      : resource {args.backing_buffer, args.size, args.resource}
    {}

    std::pmr::monotonic_buffer_resource resource;
  };

  inline void
  stack_arena_release (Stack_arena *arena)
  {
    arena->resource.release ();
  }
};
