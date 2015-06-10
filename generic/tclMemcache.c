#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <tcl.h>
#include <libmemcached/memcached.h>


static int Memcache_Cmd(ClientData arg, Tcl_Interp * interp, int objc, Tcl_Obj * CONST objv[]);

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

  enum {
    cmdGet, cmdAdd, cmdAppend, cmdSet, cmdReplace,
    cmdDelete, cmdIncr, cmdDecr, cmdVersion, cmdServer
  };

  static CONST char *sCmd[] = {
    "get", "add", "append", "set", "replace",
    "delete", "incr", "decr", "version", "server",
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
    if (objc < 5) {
      Tcl_WrongNumArgs(interp, 2, objv, "cmd server port");
      return TCL_ERROR;
    }
    if (!strcmp(Tcl_GetString(objv[2]), "add")) {
      result = memcached_server_add(get_memc(), Tcl_GetString(objv[3]), atoi(Tcl_GetString(objv[4])));
    } else if (!strcmp(Tcl_GetString(objv[2]), "delete")) {
      // TODO: not supported
      //mc_server_delete(mc, mc_server_find(mc, Tcl_GetString(objv[3]), 0));
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;

  case cmdGet:
    if (objc < 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "key dataVar ?lengthVar? ?flagsVar?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    data = memcached_get(get_memc(), key, strlen(key), &size, &flags, &result);
    if (data != NULL) {
      Tcl_SetVar2Ex(interp, Tcl_GetString(objv[3]), NULL, Tcl_NewByteArrayObj((uint8_t*)data, size), 0);
      if (objc > 4) {
        Tcl_SetVar2Ex(interp, Tcl_GetString(objv[4]), NULL, Tcl_NewLongObj(size), 0);
      }
      if (objc > 5) {
        Tcl_SetVar2Ex(interp, Tcl_GetString(objv[5]), NULL, Tcl_NewIntObj(flags), 0);
      }
      free(data);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;

  case cmdAdd:
  case cmdSet:
  case cmdAppend:
  case cmdReplace:
    if (objc < 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "key value ?expires? ?flags?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    data = (char*)Tcl_GetByteArrayFromObj(objv[3], &isize);
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
    if (objc < 3) {
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

  case cmdIncr:
  case cmdDecr:
    if (objc < 4) {
      Tcl_WrongNumArgs(interp, 2, objv, "key value ?varname?");
      return TCL_ERROR;
    }
    key = Tcl_GetString(objv[2]);
    size = atoi(Tcl_GetString(objv[3]));
    switch (cmd) {
    case cmdIncr:
      result = memcached_increment(get_memc(), key, strlen(key), size, &size64);
      break;
    case cmdDecr:
      result = memcached_decrement(get_memc(), key, strlen(key), size, &size64);
      break;
    }
    if (result == 1 && objc > 4) {
      Tcl_SetVar2Ex(interp, Tcl_GetString(objv[4]), NULL, Tcl_NewLongObj(size64), 0);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(result));
    break;

  case cmdVersion:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf("%s (with libmemcached %s)", PACKAGE_STRING, memcached_lib_version()));
    break;

  }
  return TCL_OK;
}

