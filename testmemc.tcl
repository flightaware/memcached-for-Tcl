#!/usr/bin/tclsh

load ./memcache[info sharedlibextension]

memcache server add localhost 11211
memcache set moo "cows go moo"
memcache get moo value
puts "value=$value!\n";
