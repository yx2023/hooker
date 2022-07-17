#Need to execute `export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH` for set current path in dynamic library search route
all:
	cp ../lib/libhookLib.so .
	gcc -fPIC --shared -o libtestLib.so test_so.c
	gcc -fPIC --shared -o libtest2Lib.so test.c
	gcc main.c -L. -ltestLib -ltest2Lib -lhookLib
