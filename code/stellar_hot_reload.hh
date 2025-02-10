#pragma once

#include "hyper.hh"

namespace stellar
{
  typedef void (*update_function_ptr)(hyper::frame_context &);
  typedef void (*render_function_ptr)(hyper::frame_context &);

  struct hot_reload_library_data
  {
    void *_handle;
    char const *_path;
    update_function_ptr _update;
    render_function_ptr _render;
  };

  struct hot_reload_watcher
  {
    int32_t _inotify_fd;
    int32_t _watch_fd;
    char const *_path;
  };

  bool hot_reload_init (hot_reload_library_data &, char const *);

  bool hot_reload_library_was_updated ();

  bool hot_reload_load (hot_reload_library_data &);

  void hot_reload_quit (hot_reload_library_data &);
};
