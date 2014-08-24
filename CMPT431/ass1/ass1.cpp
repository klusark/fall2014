#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Only supported argument is a single filename" << std::endl;
		return -1;
	}

	std::ifstream file;
	file.open(argv[1], std::ios::in);

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream l(line);
		std::string name, position;
		l >> name >> position;
		std::vector<std::string> shops = std::vector<std::string>(std::istream_iterator<std::string>(l), std::istream_iterator<std::string>());

		std::cout << name << " " << position << " ";
		std::copy(shops.begin(), shops.end(), std::ostream_iterator<std::string>(std::cout, " "));
		std::cout << std::endl;
	}
}
