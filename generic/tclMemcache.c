#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <tcl.h>
#include <libmemcached/memcached.h>


static int Memcache_Cmd(ClientData arg, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[]);

/*
 * Helper method that allocates/fetches the memcached instance associated with this thread.
 */
static inline memcached_st* get_memc()
{
  static __thread pid_t mempid = 0;
  static __thread memcached_st *memc = NULL;
  if (memc == NULL) {
    // first time being called, so allocate new handle.
    memc = memcached_create(NULL);
    mempid = getpid();
  } else if (mempid != getpid()) {
    // we must have forked, so allocate a new one, as a clone of the existing servers.
    memcached_st *oldmemc = memc;
    memc = memcached_clone(NULL, oldmemc);
    memcached_free(oldmemc);
    mempid = getpid();
  }

  return memc;
}

/*
 * Library initialization function, which registers the commands into the Tcl interpreter.
 */
int DLLEXPORT
Memcache_Init(Tcl_Interp *interp)
{
  if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
    return TCL_ERROR;
  }
  if (Tcl_PkgProvide(interp, "Memcache", PACKAGE_VERSION) == TCL_ERROR) {
    return TCL_ERROR;
  }
  get_memc();
  Tcl_CreateObjCommand(interp, "memcache", Memcache_Cmd, NULL, NULL);
  return TCL_OK;
}

/*
 * Handler for all commands invoked through the Tcl interpreter.
 */
static int Memcache_Cmd(ClientData arg, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[])
{
  memcached_return result;
  uint32_t flags = 0;
  char *key, *data = NULL;
  size_t size;
  int isize;
  uint32_t expires = 0;
  uint64_t size64;
  int cmd;


  // list of supported commands that we expose.
  enum {
    cmdGet, cmdAdd, cmdAppend, cmdPrepend, cmdSet, cmdReplace,
    cmdDelete, cmdFlush, cmdIncr, cmdDecr, cmdVersion, cmdServer, cmdBehavior
  };

  static CONST char *sCmd[] = {
    "get", "add", "append", "prepend", "set", "replace",
    "delete", "flush", "incr", "decr", "version", "server", "behavior",
    0
  };


  // this list should match the order defined in include/libmemcached-1.0/types/behavior.h
  static CONST char *sBehavior[] = {
    "MEMCACHED_BEHAVIOR_NO_BLOCK",
    "MEMCACHED_BEHAVIOR_TCP_NODELAY",
    "MEMCACHED_BEHAVIOR_HASH",
    "MEMCACHED_BEHAVIOR_KETAMA",
    "MEMCACHED_BEHAVIOR_SOCKET_SEND_SIZE",
    "MEMCACHED_BEHAVIOR_SOCKET_RECV_SIZE",
    "MEMCACHED_BEHAVIOR_CACHE_LOOKUPS",
    "MEMCACHED_BEHAVIOR_SUPPORT_CAS",
    "MEMCACHED_BEHAVIOR_POLL_TIMEOUT",
    "MEMCACHED_BEHAVIOR_DISTRIBUTION",
    "MEMCACHED_BEHAVIOR_BUFFER_REQUESTS",
    "MEMCACHED_BEHAVIOR_USER_DATA",
    "MEMCACHED_BEHAVIOR_SORT_HOSTS",
    "MEMCACHED_BEHAVIOR_VERIFY_KEY",
    "MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT",
    "MEMCACHED_BEHAVIOR_RETRY_TIMEOUT",
    "MEMCACHED_BEHAVIOR_KETAMA_WEIGHTED",
    "MEMCACHED_BEHAVIOR_KETAMA_HASH",
    "MEMCACHED_BEHAVIOR_BINARY_PROTOCOL",
    "MEMCACHED_BEHAVIOR_SND_TIMEOUT",
    "MEMCACHED_BEHAVIOR_RCV_TIMEOUT",
    "MEMCACHED_BEHAVIOR_SERVER_FAILURE_LIMIT",
    "MEMCACHED_BEHAVIOR_IO_MSG_WATERMARK",
    "MEMCACHED_BEHAVIOR_IO_BYTES_WATERMARK",
    "MEMCACHED_BEHAVIOR_IO_KEY_PREFETCH",
    "MEMCACHED_BEHAVIOR_HASH_WITH_PREFIX_KEY",
    "MEMCACHED_BEHAVIOR_NOREPLY",
    "MEMCACHED_BEHAVIOR_USE_UDP",
    "MEMCACHED_BEHAVIOR_AUTO_EJECT_HOSTS",
    "MEMCACHED_BEHAVIOR_NUMBER_OF_REPLICAS",
    "MEMCACHED_BEHAVIOR_RANDOMIZE_REPLICA_READ",
    "MEMCACHED_BEHAVIOR_CORK",
    "MEMCACHED_BEHAVIOR_TCP_KEEPALIVE",
    "MEMCACHED_BEHAVIOR_TCP_KEEPIDLE",
    "MEMCACHED_BEHAVIOR_LOAD_FROM_FILE",
    "MEMCACHED_BEHAVIOR_REMOVE_FAILED_SERVERS",
    "MEMCACHED_BEHAVIOR_DEAD_TIMEOUT",
    "MEMCACHED_BEHAVIOR_SERVER_TIMEOUT_LIMIT",
    0
  };

  if (objc < 2) {
    Tcl_WrongNumArgs(interp, 1, objv, "cmd args");
    return TCL_ERROR;
  }
  if (Tcl_GetIndexFromObj(interp, objv[1], sCmd, "command", TCL_EXACT, (int *) &cmd) != TCL_OK) {
    return TCL_ERROR;
  }

  switch (cmd) {

  case cmdServer:
    /*
     * Server list manipulation:
     *   - server add hostname port
     *   - memcache server delete hostname port
     */
    if (objc != 5) {
      Tcl_WrongNumArgs(interp, 2, objv, "cmd server port");
      return TCL_ERROR;
    }
    if (!strcmp(Tcl_GetString(objv[2]), "add")) {
      result = memcached_server_add(get_memc(), Tcl_GetString(objv[3]), atoi(Tcl_GetString(objv[4])));
    } else if (!strcmp(Tcl_GetString(objv[2]), "delete")) {
      // TODO: not supported
      //mc_server_delete(mc, mc_server_find(mc, Tcl_GetString(objv[3]), 0));
      Tcl_AppendResult(interp, "server delete not supported.", NULL);
      return TCL_ERROR;
    } else {
      Tcl_AppendResult(interp, "server command not recognized.", NULL);
      return TCL_ERROR;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;

  case cmdGet:
    /*
     * Lookup and retrieve a cached value:
     *  - memcache get key varname ?lengthVar? ?flagsVar?
     */
    if (objc < 4 || objc > 6) {
      Tcl_WrongNumArgs(interp, 2, objv, "key dataVar ?lengthVar? ?flagsVar?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    data = memcached_get(get_memc(), key, strlen(key), &size, &flags, &result);
    if (data != NULL) {
      Tcl_SetVar2Ex(interp, Tcl_GetString(objv[3]), NULL, Tcl_NewStringObj((const char *)data, size), 0);
      if (objc > 4) {
        Tcl_SetVar2Ex(interp, Tcl_GetString(objv[4]), NULL, Tcl_NewLongObj(size), 0);
      }
      if (objc > 5) {
        Tcl_SetVar2Ex(interp, Tcl_GetString(objv[5]), NULL, Tcl_NewIntObj(flags), 0);
      }
      free(data);
    } else {
      // error occurred (no data returned), so ensure that result always reflects an error.
      if (result == 0) {
        result = MEMCACHED_FAILURE;
      }
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;


  case cmdAdd:
  case cmdSet:
  case cmdAppend:
  case cmdPrepend:
  case cmdReplace:
    /*
     * Store a new value into the cache:
     *
     * - memcache add key value ?expires? ?flags?
     * - memcache append key value ?expires? ?flags?
     * - memcache prepend key value ?expires? ?flags?
     * - memcache set key value ?expires? ?flags?
     * - memcache replace key value ?expires? ?flags?
     */
    if (objc < 4 || objc > 6) {
      Tcl_WrongNumArgs(interp, 2, objv, "key value ?expires? ?flags?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    data = (char*)Tcl_GetStringFromObj(objv[3], &isize);
    if (objc > 4) {
      expires = atoi(Tcl_GetString(objv[4]));
    }
    if (objc > 5) {
      flags = atoi(Tcl_GetString(objv[5]));
    }
    switch (cmd) {
    case cmdAdd:
      result = memcached_add(get_memc(), key, strlen(key), data, isize, expires, flags);
      break;
    case cmdAppend:
      result = memcached_append(get_memc(), key, strlen(key), data, isize, expires, flags);
      break;
    case cmdPrepend:
      result = memcached_prepend(get_memc(), key, strlen(key), data, isize, expires, flags);
      break;
    case cmdSet:
      result = memcached_set(get_memc(), key, strlen(key), data, isize, expires, flags);
      break;
    case cmdReplace:
      result = memcached_replace(get_memc(), key, strlen(key), data, isize, expires, flags);
      break;
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;


  case cmdDelete:
    /*
     * Delete an existing value from the cache (if it exists):
     *
     * - memcache delete key ?expires?
     */
    if (objc < 3 || objc > 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "key ?expires?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    if (objc > 3) {
      expires = atoi(Tcl_GetString(objv[3]));
    }
    result = memcached_delete(get_memc(), key, strlen(key), expires);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;


  case cmdFlush:
    /*
     * Wipe clean the contents of memcached servers:
     *
     * - memcache flush ?expires?
     */
    if (objc != 2 && objc != 3) {
      Tcl_WrongNumArgs(interp, 2, objv, "?expires?");
      return TCL_ERROR;
    }
    if (objc == 3) {
      expires = atoi(Tcl_GetString(objv[2]));
    }
    result = memcached_flush(get_memc(), expires);
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;

    
  case cmdIncr:
  case cmdDecr:
    /*
     * Increment or decrement a cached key containing a numeric value.
     *
     * - memcache incr key value ?varname? ?initial? ?expires?
     * - memcache decr key value ?varname? ?initial? ?expires?
     */
    if (objc < 4 || objc > 7) {
      Tcl_WrongNumArgs(interp, 2, objv, "key value ?varname? ?initial? ?expires?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    size = atoi(Tcl_GetString(objv[3])); /* value to incr/decr by */
    if (objc > 5) {   /* initial value if missing */
      size64 = atoll(Tcl_GetString(objv[5]));
    } else {
      size64 = 0;
    }
    if (objc > 6) {   /* expiration */
      expires = atoi(Tcl_GetString(objv[6]));
    }

    switch (cmd) {
    case cmdIncr:
      if (objc > 5) {
	result = memcached_increment_with_initial(get_memc(), key, strlen(key), size, size64, expires, &size64);
      } else {
	result = memcached_increment(get_memc(), key, strlen(key), size, &size64);
      }
      break;
    case cmdDecr:
      if (objc > 5) {
	result = memcached_decrement_with_initial(get_memc(), key, strlen(key), size, size64, expires, &size64);
      } else {
	result = memcached_decrement(get_memc(), key, strlen(key), size, &size64);
      }
      break;
    }
    if (result == MEMCACHED_SUCCESS && objc > 4) {
      Tcl_SetVar2Ex(interp, Tcl_GetString(objv[4]), NULL, Tcl_NewLongObj(size64), 0);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;


  case cmdVersion:
    /*
     * Return the version of this package and the library we've been built against.
     */
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s (with libmemcached %s)", PACKAGE_STRING, memcached_lib_version()));
    break;


  case cmdBehavior:
    /*
     * View or modify various configuration parameters that control network timeouts, replication, and more.
     *
     * - memcache behavior flagname ?flagvalue?
     */
    if (objc < 3 || objc > 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "flagname ?flagvalue?");
      return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[2], sBehavior, "command", TCL_EXACT, (int *) &cmd) != TCL_OK) {
      return TCL_ERROR;
    }
    if (objc > 3) {
      result = memcached_behavior_set(get_memc(), cmd, atoll(Tcl_GetString(objv[3])));
      Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    } else {
      uint64_t currentVal = memcached_behavior_get(get_memc(), cmd);
      Tcl_SetObjResult(interp, Tcl_NewWideIntObj(currentVal));
    }
  }
  return TCL_OK;
}

