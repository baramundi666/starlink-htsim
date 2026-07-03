all:
	g++ -I../src -I../src/starlink -Wall -std=c++11 -g -c ../src/starlink/isl.cpp ../src/starlink/node.cpp ../src/starlink/satellite.cpp ../src/starlink/city.cpp ../src/starlink/constellation.cpp ../src/starlink/binary_heap.cpp ../src/starlink/main.cpp
	g++ *.o -L../src -lhtsim -o starlink2

clean:
	rm -f *.o starlink2