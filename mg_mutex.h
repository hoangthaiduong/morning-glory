#pragma once

namespace mg {

struct mutex;

/* Block until the lock can be acquired then return true. Return false if something goes wrong.  */
bool Lock(mutex* Mutex);
/* Return immediately: true if the lock can be acquired, false if not */
bool TryLock(mutex* Mutex);
/* Return true if the lock can be released. Return false if something goes wrong. */
bool Unlock(mutex* Mutex);

}

#include "mg_mutex_posix.inl"
#include "mg_mutex_win.inl"
