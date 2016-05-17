memcached-for-Tcl
=================

memcached client library for Tcl based applications


Installation
------------

The actual memcached server installation is independent of the
installation of this client package and is not addressed by this
document.


On FreeBSD:
* pkg install databases/libmemcached
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


Available Commands
------------------

 memcache server add server port

 memcache get key varname ?lengthVar? ?flagsVar?

 memcache add key value ?expires? ?flags?

 memcache append key value ?expires? ?flags?

 memcache set key value ?expires? ?flags?

 memcache replace key value ?expires? ?flags?

 memcache delete key ?expires?

 memcache incr key value ?varname?

 memcache decr key value ?varname?

 memcache version

 memcache behavior flagname ?flagvalue?
