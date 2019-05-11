.PNOHY: all compete debug
all: Strategy/*.cpp

compete: all
	cd Strategy && clang++ -arch i386 *.cpp -shared -o Strategy.dylib --std=c++11 -O3
	Compete/bin/Compete Testcases/2.dylib Strategy/Strategy.dylib test 1

debug: 
	cd Strategy && clang++ -arch i386 *.cpp -shared -o Strategy.dylib --std=c++11 -g
	lldb -- Compete/bin/Compete Testcases/2.dylib Strategy/Strategy.dylib test 1
