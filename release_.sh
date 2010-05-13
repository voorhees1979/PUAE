make clean

rm -rf ./df0.adz
rm -rf ./kick.rom

rm -rf ./config.log
rm -rf ./config.status
rm -rf ./configure
rm -rf ./aclocal.m4

rm -rf ./src/gfxdep
rm -rf ./src/guidep
rm -rf ./src/joydep
rm -rf ./src/machdep
rm -rf ./src/osdep
rm -rf ./src/sounddep
rm -rf ./src/threaddep

rm -rf `find . -type d -name autom4te.cache`
rm -rf `find . -type d -name .deps`
rm -rf `find . -type f -name Makefile`
rm -rf `find . -type f -name *~`
