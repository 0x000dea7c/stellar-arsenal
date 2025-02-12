#pragma once

#include "hyper.hh"
#include "stellar.hh"

namespace stellar
{
  using function_ptr_signature = void (*)(hyper::Frame_context &, stellar::Game_data &);

  struct Hot_reload_library_data
  {
    void *handle;
    char const *path;
    function_ptr_signature update;
    function_ptr_signature render;
  };

  struct Hot_reload_watcher
  {
    int32_t inotify_fd;
    int32_t watch_fd;
    char const *path;
  };

  bool hot_reload_init (Hot_reload_library_data &, char const *);

  bool hot_reload_library_was_updated ();

  bool hot_reload_load (Hot_reload_library_data &);

  void hot_reload_quit (Hot_reload_library_data &);
};
