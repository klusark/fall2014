#include <random>
#include <chrono>
#include <set>
#include <string>

std::default_random_engine generator((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

std::set usedNames;

std::string generateName() {
	std::uniform_int_distribution<int> distribution(4, 7);
	int len = distribution(generator);
	std::string output;
	for (int i = 0; i < len; ++i) {
		
	}
}

int main() {
	int numNames = 4;
	int numShops = 6;
	int numShopsPerName = 3;

	for (int i = 0; i < numNames; ++i) {
	}
}
