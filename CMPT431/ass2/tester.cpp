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
		std::cout << "Error " << r << " != " << e << std::endl;
	}
}

void connectA(int &sockfd, const char *host, int portno) {
    struct hostent *server;
    sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server = gethostbyname(host);
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

    if (connect(sockfd, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr,"ERROR, no such connection\n");
	}
}

std::string readResponse(int sockfd) {
	std::string out = "";
	while (1) {
		char buff[256];
		ssize_t len = read(sockfd, buff, 255);
		if (len == -1) {
				return "";
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

	while (std::cin.good()) {
		connectA(sockfd, argv[1], portno);
		std::string method, t, s, message, exmethod, ext, exs, exe, exmessage;
		std::cin >> method >> t >> s >> message >> exmethod >> ext >> exs >> exe >> exmessage;
		if (!std::cin.good()) {
			return 0;
		}
		checkVar(t);
		checkVar(s);
		checkVar(message);
		//checkVar(ext);
		//checkVar(exs);
		//checkVar(exmessage);
		std::stringstream str;
		str << method << " " << t << " " << s << " " << message.length()
			<< "\r\n\r\n" << message << "\r\n";
		std::string out = str.str();
		std::cout << "Sending: " << out << std::endl;
		write(sockfd, out.c_str(), out.size());
		std::string response = readResponse(sockfd);
		std::cout << "Receiving: " << response << std::endl;
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
		close(sockfd);
		++i;
	}
	std::cout << i << std::endl;
}
