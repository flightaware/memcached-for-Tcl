![Linux CI](https://github.com/bovine/memcached-for-Tcl/workflows/Linux%20CI/badge.svg)

memcached-for-Tcl
=================

Memcached is a data caching service that can be used to accelerate computations or operations that are repeated
frequently, but do not often change. This package provides a Tcl interface to the memcached servers.


Installation
------------

The actual memcached server installation is independent of the
installation of this client package and is not addressed by this
document.

The underlying libmemcached installation recommended is awesomized/libmemcached
because the official libmemcached has not had a release since 2014 and there are
crashing bugs that have had fixes provided that haven't been rolled up into a release.

On FreeBSD:
* install git@github.com:awesomized/libmemcached.git
* env CPPFLAGS=-I/usr/local/include LDFLAGS=-L/usr/local/lib ./configure --with-tcl=/usr/local/lib/tcl8.6
* make
* make install


Usage
-----

To begin using this package, you must require it and then configure
the address of your local memcached servers.  If you have more than
one memcached server then you can repeat the "memcache server" line
for each additional server you have.

```
package require Memcache
memcache server add servername.example.com 11211
```

To set a value into the memcached server, use the following syntax to
define a key named "moo":

```
memcache set moo "cows go moo"
```

To fetch this same value back from the memcached server and write it
into the "result" variable, use this syntax:

```
memcache get moo result
```

All of the memcache commands will return an integer code that is zero
on success or some other integer error. If the returned value is
non-zero then the request failed, and you should not expect any 
varname arguments to have been modified.

Use `memcache strerror` to get a human readable version of the error code.


Available Commands
------------------

 memcache server add hostname port

 memcache server clear

 memcache get key varname ?lengthVar? ?flagsVar?

 memcache add key value ?expires? ?flags?

 memcache append key value ?expires? ?flags?

 memcache prepend key value ?expires? ?flags?

 memcache set key value ?expires? ?flags?

 memcache replace key value ?expires? ?flags?

 memcache delete key ?expires?

 memcache flush ?expires?

 memcache incr key value ?varname?

 memcache decr key value ?varname?

 memcache version

 memcache behavior flagname ?flagvalue?

 memcache strerror errorcode
