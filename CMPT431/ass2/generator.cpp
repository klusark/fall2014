#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[]) {
	std::string filename = "file1.txt";
	if (argc == 2) {
		filename = argv[1];
	}
	while (1) {
		std::cout << "NEW_TXN 0   0   " << filename << "   ACK     $T  0 0     *" << std::endl;
		std::cout << "WRITE   $T  1   abc         ACK     $T  1 0     *" << std::endl;
		std::cout << "COMMIT  $T  2   *           ACK     $T  2 0     *" << std::endl;
	}
}
