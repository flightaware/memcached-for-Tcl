
package require Memcache
#load ./memcache[info sharedlibextension]

memcache server add localhost 11211
memcache set moo "cows go moo"
memcache get moo value
if {$value != "cows go moo"} {
    puts "Error.  value=$value!\n";
    exit 1
}

set value "Boeing 777-200 (طائرة نفاثة ثنائية المحرك)"

memcache set unicodeTest $value
memcache get unicodeTest newvalue
if {$value != $newvalue} {
    puts "Error.  newvalue=$newvalue!\n";
	exit 1
}

puts "Success"
exit 0
