// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"

const bool SERVER_PRINT_DEBUG = true;

#define SDEBUG(...) if (SERVER_PRINT_DEBUG) tprintf("server: " __VA_ARGS__)

lock_server_cache::lock_server_cache()
{
  pthread_mutex_init(&locks_mutex, NULL);
}


lock_protocol::status
lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, int &)
{
  lock_protocol::status ret = lock_protocol::OK;
  SDEBUG("acq %llx from '%s'\n", lid, id.c_str());

  pthread_mutex_lock(&locks_mutex);
  server_lock_t &lock = locks[lid];
  SDEBUG("%llx is %d-locked by '%s'\n", lid, lock.is_locked, lock.holder.c_str());
  
  bool revoke = false;
  if ( ! lock.is_locked && (lock.next.empty() || lock.next == id) ) {
    lock.is_locked = true;
    lock.holder = id;
    lock.next.clear();
    lock.waiting_set.erase(id);
    SDEBUG("sending lock %llx to client %s\n", lid, id.c_str());

    if (lock.waiting_set.size()) {
      revoke = true;
      std::string next = *(lock.waiting_set.begin());
      lock.next = next;
      lock.waiting_set.erase(next);
    }
  } else if (lock.next.empty() || lock.next == id) {
    lock.next = id;
    lock.waiting_set.erase(id);
    revoke = true;
    ret = lock_protocol::RETRY;
  } else {
    lock.waiting_set.insert(id);
    revoke = true;
    ret = lock_protocol::RETRY;
  }
  std::string next = lock.next;
  std::string holder = lock.holder;
  pthread_mutex_unlock(&locks_mutex);

  if (revoke && ! holder.empty()) {
    SDEBUG("revoking %llx from '%s' ('%s' waiting)\n", lid, lock.holder.c_str(), lock.next.c_str());
    
    handle h(holder);
    rpcc *cl = h.safebind();
    VERIFY(cl);

    int r;
    rlock_protocol::status rval = cl->call(rlock_protocol::revoke, lid, next, r);
    VERIFY( rval == rlock_protocol::OK );
  }

  return ret;
}

lock_protocol::status
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &)
{
  ScopedLock m_(&locks_mutex);

  server_lock_t &lock = locks[lid];
  if ( ! lock.is_locked ) return lock_protocol::RPCERR;
  if ( lock.holder != id) return lock_protocol::RPCERR;

  SDEBUG("releasing lock %llx, previously held by client %s\n", lid, id.c_str());

  lock.is_locked = false;
  lock.holder.clear();

  return lock_protocol::OK;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  return lock_protocol::OK;
}

