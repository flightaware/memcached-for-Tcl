
package require Memcache
#load ./memcache[info sharedlibextension]

memcache server add localhost 11211
memcache set moo "cows go moo"
memcache get moo value
if {$value != "cows go moo"} {
    puts "Error.  value=$value!\n";
    exit 1
}

puts "Success"
exit 0
