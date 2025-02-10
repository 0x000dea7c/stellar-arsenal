#include "stellar_hot_reload.hh"

#include <linux/limits.h>
#include <errno.h>
#include <sys/inotify.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

namespace stellar
{
  static hot_reload_watcher watcher;

  // this hack is needed for calling dylsym
  union
  {
    void *obj_ptr;
    update_function_ptr func_ptr;
  } upd_tmp;

  union
  {
    void *obj_ptr;
    render_function_ptr func_ptr;
  } rend_tmp;

  static bool
  watcher_init (char const *path)
  {
    watcher._path = path;

    watcher._inotify_fd = inotify_init1 (IN_NONBLOCK);
    if (watcher._inotify_fd == -1)
      {
        std::cerr << "couldn't initialise inotify: " << strerror (errno) << '\n';
        return false;
      }

    watcher._watch_fd = inotify_add_watch (watcher._inotify_fd, watcher._path, IN_ALL_EVENTS);
    if (watcher._watch_fd == -1)
      {
        close (watcher._inotify_fd);
        std::cerr << "couldn't add watch: " << strerror (errno) << '\n';
        return false;
      }

    return true;
  }

  static bool
  restart_watcher ()
  {
    if (inotify_rm_watch (watcher._inotify_fd, watcher._watch_fd) == -1)
      {
        std::cerr << "couldn't remove watch: " << strerror (errno) << '\n';
        return false;
      }

    return watcher_init (watcher._path);
  }

  bool
  hot_reload_init (hot_reload_library_data &library, char const *path)
  {
    library._path = path;
    library._handle = NULL;
    library._render = NULL;
    library._update = NULL;

    if (!watcher_init (path))
      {
        return false;
      }

    return hot_reload_load (library);
  }

  bool
  hot_reload_library_was_updated ()
  {
    char buffer[sizeof (struct inotify_event) + NAME_MAX + 1];
    bool modified = false;

    while (read (watcher._inotify_fd, buffer, sizeof (buffer)) > 0)
      {
        struct inotify_event *event = (struct inotify_event *) buffer;
        if (event->mask & (IN_OPEN | IN_ATTRIB))
          {
            restart_watcher ();
            modified = true;
            break;
          }
      }

    if (modified)
      {
        // XXX: don't try to open the new library immediately because
        // it might have not been fully created yet!
        usleep (50'000);
      }

    return modified;
  }

  bool
  hot_reload_load (hot_reload_library_data &library)
  {
    if (library._handle)
      {
        dlclose (library._handle);
        library._handle = NULL;
        library._render = NULL;
        library._update = NULL;
      }

    // RTLD_NOW: find all symbols immediately.
    library._handle = dlopen (library._path, RTLD_NOW);
    if (!library._handle)
      {
        std::cerr << "couldn't open lib " << library._path << ':' << strerror (errno) << '\n';
        return false;
      }

    // NOTE: this is to check for errors, read the man page
    dlerror ();

    // look for symbols inside this library
    upd_tmp.obj_ptr = dlsym (library._handle, HYPER_UPDATE_FUNCTION_NAME);
    char *err = dlerror ();
    if (err)
      {
        std::cerr << "couldn't find game_update symbol for lib " << library._path << ':' << strerror (errno) << '\n';
        return false;
      }

    dlerror ();

    rend_tmp.obj_ptr = dlsym (library._handle, HYPER_RENDER_FUNCTION_NAME);
    err = dlerror ();
    if (err)
      {
        std::cerr << "couldn't find game_render symbol for lib " << library._path << ':' << strerror (errno) << '\n';
        return false;
      }

    library._update = upd_tmp.func_ptr;
    library._render = rend_tmp.func_ptr;

    return true;
  }

  void
  hot_reload_quit (hot_reload_library_data &library)
  {
    if (library._handle)
      dlclose (library._handle);

    inotify_rm_watch (watcher._inotify_fd, watcher._watch_fd);
    close (watcher._inotify_fd);
    close (watcher._watch_fd);
  }
};
