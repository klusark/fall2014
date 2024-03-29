#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <map>
#include <sstream>

std::map<std::string, std::string> _vars;

std::string getVar(const std::string &var) {
	return _vars[var];
}
int curline = 1;
bool verbose = false;

void checkVar(std::string &var) {
	if (var[0] == '$') {
		var = getVar(var);
	} else if (var[0] == '*') {
		var = "";
	}
}

void checkResponse(std::string r, std::string e) {
	if (e[0] == '$') {
		_vars[e] = r;
	} else if (e[0] == '*') {
	} else if (r != e) {
		std::cerr << "Error line: " << curline << " " << r << " != " << e << std::endl;
	}
}
struct hostent *server;
sockaddr_in serv_addr;

void connectA(int &sockfd) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		std::cerr << "socket error" << std::endl;
		exit(0);
	}
	int ret = connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr));
	if (ret < 0) {
        fprintf(stderr,"ERROR, no such connection: %s\n", strerror(errno));
		exit(0);
	}
}

std::string readResponse(int sockfd) {
	std::string out = "";
	while (1) {
		char buff[256];
		ssize_t len = read(sockfd, buff, 255);
		if (len <= 0) {
				return out;
		}
		out += std::string(buff, len);
		int size = out.size();
		for (int i = 0; i < size; ++i) {
				if (i < size - 4 && out[i] == '\r'
								 && out[i + 1] == '\n'
								 && out[i + 2] == '\r'
								 && out[i + 3] == '\n') {
						return out;
				}
		}
	}
	return "";
}

int main(int argc, char *argv[]) {
    int sockfd, portno, n;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
	int i = 0;


    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);



	connectA(sockfd);
	std::string line;
	while (std::getline(std::cin, line)) {
		if (line.length() == 0 || line[0] == '#') {
			++curline;
			continue;
		}
		std::stringstream strr(line);
		std::string method, t, s, message, exmethod, ext, exs, exe, exmessage;
		strr >> method >> t >> s >> message >> exmethod >> ext >> exs >> exe >> exmessage;
		if (!std::cin.good()) {
			return 0;
		}
		checkVar(t);
		checkVar(s);
		checkVar(message);
		std::stringstream str;
		str << method << " " << t << " " << s << " " << message.length()
			<< "\r\n\r\n" << message;
		if (message.length() == 0) {
			str << "\r\n";
		}
		std::string out = str.str();
		if (verbose) {
			std::cout << "Sending: " << out << std::endl;
		}
		int outlen = write(sockfd, out.c_str(), out.size());
		if (outlen != out.size()) {
			std::cerr << "write error " << outlen << std::endl;
		}
		std::string response = readResponse(sockfd);
		if (verbose) {
			std::cout << "Receiving: " << response << std::endl;
		}
		std::stringstream r(response);
		std::string rmethod, rt, rs, re, rl, rmessage;
		r >> rmethod >> rt >> rs >> re >> rl;
		if (rl != "") {
			r>> rmessage;
		}
		checkResponse(rmethod, exmethod);
		checkResponse(rt, ext);
		checkResponse(rs, exs);
		checkResponse(re, exe);
		checkResponse(rmessage, exmessage);
		++i;
		++curline;
	}
}
