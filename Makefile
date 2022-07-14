all:
	export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
	gcc -fPIC -shared dynLib.c -o libdynLib.so
	gcc -fPIC -o hookLib.o -c hookLib.c
	ar rcs libhookLib.a hookLib.o
	gcc -g main.c -L. -ldynLib -lhookLib
