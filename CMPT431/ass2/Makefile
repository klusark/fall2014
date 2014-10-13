all: ass2 tester

ass2: ass2.cpp
	$(CXX) -std=c++11 -ggdb -pthread -o ass2 ass2.cpp

tester: tester.cpp
	$(CXX) -std=c++11 -O3 -pthread -o tester tester.cpp


clean:
	rm ass2 tester
