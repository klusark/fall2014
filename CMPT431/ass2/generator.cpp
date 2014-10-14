#include <iostream>
#include <unistd.h>

int main() {
	while (1) {
		std::cout << "NEW_TXN 0   0   file1.txt   ACK     $T  0 0     *" << std::endl;
		std::cout << "WRITE   $T  1   abc         ACK     $T  1 0     *" << std::endl;
		std::cout << "COMMIT  $T  2   *           ACK     $T  2 0     *" << std::endl;
	}
}
