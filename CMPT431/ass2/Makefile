all: ass2 tester generator

ass2: ass2.cpp
	$(CXX) -std=c++11 -O0 -ggdb -pthread -o ass2 ass2.cpp -lsqlite3

tester: tester.cpp
	$(CXX) -std=c++11 -O3 -ggdb -pthread -o tester tester.cpp

generator: generator.cpp
	$(CXX) -std=c++11 -O3 -ggdb -pthread -o generator generator.cpp

clean:
	rm ass2 tester generator
