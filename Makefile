.PNOHY: all compete
all:
	cd Strategy && clang++ -arch i386 *.cpp -shared -o Strategy.dylib

compete: all
	Compete/bin/Compete Strategy/Strategy.dylib Testcases/2.dylib test 1
