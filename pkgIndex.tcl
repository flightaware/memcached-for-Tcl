# pkgIndex.tcl -- tells Tcl how to load my package.
package ifneeded "Memcache" 1.0 \
    [list load [file join $dir memcache[info sharedlibextension]]]
