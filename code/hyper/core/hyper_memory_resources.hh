#pragma once

#include <memory_resource>
#include <new>

namespace hyper
{
  struct fixed_memory_resource_bad_alloc : public std::bad_alloc
  {
    char const *
    what () const noexcept override
    {
      return "ran out of memory! this arena doesn't support reallocations!";
    }
  };

  class fixed_memory_resource : public std::pmr::memory_resource
  {
  protected:
    void *
    do_allocate ([[maybe_unused]] size_t bytes, [[maybe_unused]] size_t alignment) override
    {
      throw fixed_memory_resource_bad_alloc ();
    }

    void
    do_deallocate ([[maybe_unused]] void *ptr, [[maybe_unused]] size_t bytes, [[maybe_unused]] size_t alignment) override
    {
      // no-op
    }

    bool
    do_is_equal (std::pmr::memory_resource const& other) const noexcept override
    {
      return this == &other;
    }
  };
};
