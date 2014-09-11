#include <random>
#include <chrono>
#include <set>
#include <string>
#include <iostream>
#include <vector>

std::default_random_engine generator((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

std::set<std::string> usedNames;
std::vector<std::string> shopNames;

std::string generateName() {
	//std::uniform_int_distribution<int> distribution(4, 7);
	int len = 7;//distribution(generator);
	std::string output;
	for (int i = 0; i < len; ++i) {
        std::uniform_int_distribution<int> cd(97, 122);
        char c = cd(generator);
        output += c;
	}
    if (usedNames.find(output) != usedNames.end()) {
        return generateName();
    }
    usedNames.insert(output);
    return output;
}

std::string generatePosition() {
	std::uniform_int_distribution<int> distribution(1, 2);
	int len = distribution(generator);
    if (len == 1) {
        return "Auror";
    } else {
        return "DeathEater";
    }
}

int main() {
	int numNames = 512;
	int numShops = 64;
	int numShopsPerName = 64;

    for (int i = 0; i < numShops; ++i) {
        shopNames.push_back(generateName());
    }

	for (int i = 0; i < numNames; ++i) {
        std::cout << generateName() << " " << generatePosition();
        std::set<std::string> personUsedShops;
        for (int j = 0; j < numShopsPerName; ++j) {
            std::uniform_int_distribution<int> d(0, shopNames.size() - 1);
            int c = d(generator);
            std::string shop = shopNames[c];
            if (personUsedShops.find(shop) == personUsedShops.end()) {
                std::cout << " " << shop;
                personUsedShops.insert(shop);
            }
        }
        std::cout << std::endl;
	}
}
