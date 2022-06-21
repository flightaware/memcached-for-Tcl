
package require Memcache
#load ./memcache[info sharedlibextension]

set result [memcache server add localhost 11211]
if {$result} {
    puts "memcache server add: [memcache strerror $result]"
    exit 1
}

# Clear server list just to make sure.
set result [memcache server clear]
if {$result} {
    puts "memcache server clear: [memcache strerror $result]"
    exit 1
}

# add again
set result [memcache server add localhost 11211]
if {$result} {
    puts "memcache server add: [memcache strerror $result]"
    exit 1
}

# actually test something
set result [memcache set moo "cows go moo"]
if {$result} {
    puts "memcache set: [memcache strerror $result]"
    exit 1
}
set result [memcache get moo value]
if {$result} {
    puts "memcache get: [memcache strerror $result]"
    exit 1
}
if {$value != "cows go moo"} {
    puts "Error.  value=$value!\n";
    exit 1
}

set value "Boeing 777-200 (طائرة نفاثة ثنائية المحرك)"

set result [memcache set unicodeTest $value]
if {$result} {
    puts "memcache set: [memcache strerror $result]"
    exit 1
}
set result [memcache get unicodeTest newvalue]
if {$result} {
    puts "memcache set: [memcache strerror $result]"
    exit 1
}
if {$value != $newvalue} {
    puts "Error.  newvalue=$newvalue!\n";
    exit 1
}

puts "Success"
exit 0
