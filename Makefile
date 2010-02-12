
.PSEUDO: all
all: memcache.so

.PSEUDO: clean
clean:
	rm -f memcache.so

memcache.so: mymemcache.c
	gcc -Wall -O2 -DUSE_TCL_STUBS -I/usr/local/include -L/usr/local/lib \
		-o memcache.so -shared mymemcache.c -lmemcached -ltclstub8.5 

