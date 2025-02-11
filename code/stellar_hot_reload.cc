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
  static Hot_reload_watcher watcher;

  static bool
  watcher_init (char const *path)
  {
    watcher.path = path;

    watcher.inotify_fd = inotify_init1 (IN_NONBLOCK);
    if (watcher.inotify_fd == -1)
      {
        std::cerr << "couldn't initialise inotify: " << strerror (errno) << '\n';
        return false;
      }

    watcher.watch_fd = inotify_add_watch (watcher.inotify_fd, watcher.path, IN_ALL_EVENTS);
    if (watcher.watch_fd == -1)
      {
        close (watcher.inotify_fd);
        std::cerr << "couldn't add watch: " << strerror (errno) << '\n';
        return false;
      }

    return true;
  }

  static bool
  restart_watcher ()
  {
    if (inotify_rm_watch (watcher.inotify_fd, watcher.watch_fd) == -1)
      {
        std::cerr << "couldn't remove watch: " << strerror (errno) << '\n';
        return false;
      }

    return watcher_init (watcher.path);
  }

  bool
  hot_reload_init (Hot_reload_library_data &library, char const *path)
  {
    library.path = path;
    library.handle = NULL;
    library.render = NULL;
    library.update = NULL;

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

    while (read (watcher.inotify_fd, buffer, sizeof (buffer)) > 0)
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
  hot_reload_load (Hot_reload_library_data &library)
  {
    if (library.handle)
      {
        dlclose (library.handle);
        library.handle = NULL;
        library.render = NULL;
        library.update = NULL;
      }

    // RTLD_NOW: find all symbols immediately.
    library.handle = dlopen (library.path, RTLD_NOW);
    if (!library.handle)
      {
        std::cerr << "couldn't open lib " << library.path << ':' << strerror (errno) << '\n';
        return false;
      }

    // NOTE: this is to check for errors, read the man page
    dlerror ();

    // look for symbols inside this library
    void *function_ptr = dlsym (library.handle, HYPER_UPDATE_FUNCTION_NAME);
    char *error = dlerror ();
    if (error)
      {
        std::cerr << "couldn't find game_update symbol for lib " << library.path << ':' << error << '\n';
        return false;
      }

    library.update = reinterpret_cast<function_ptr_signature> (function_ptr);

    dlerror ();

    function_ptr = dlsym (library.handle, HYPER_RENDER_FUNCTION_NAME);
    error = dlerror ();
    if (error)
      {
        std::cerr << "couldn't find game_render symbol for lib " << library.path << ':' << error << '\n';
        return false;
      }

    library.render = reinterpret_cast<function_ptr_signature> (function_ptr);

    return true;
  }

  void
  hot_reload_quit (Hot_reload_library_data &library)
  {
    if (library.handle)
      dlclose (library.handle);

    inotify_rm_watch (watcher.inotify_fd, watcher.watch_fd);
    close (watcher.inotify_fd);
    close (watcher.watch_fd);
  }
};
