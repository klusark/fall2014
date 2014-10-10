#include <thread>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <sstream>
#include <random>
#include <set>

class Transaction {
public:
    Transaction();
    int _id;
    int getId() { return _id; }
    static std::set<int> _usedIds;
};


std::set<int> Transaction::_usedIds;

Transaction::Transaction() {
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1, 0x7fffffff);
    int id = distribution(generator);
    while (_usedIds.find(id) != _usedIds.end()) {
        id = distribution(generator);
    }
    _usedIds.insert(id);
    _id = id;
}

class Client {
public:
    sockaddr_in _addr;
    std::thread _thread;
    int _fd;
    std::vector<char> _buffer;
    void readThread();
    void parseMessage(const char *);
    void bufferData(int length);
    void respond(const std::string &method, int id, int seqno, int error, int len, const char *buff = nullptr);

    std::vector<Transaction *> t;
};

void Client::readThread() {
    std::cout << "got a connection" << std::endl;
    while (1) {
        char buff[256];
        ssize_t len = read(_fd, buff, 255);
        if (len == -1) {
            return;
        }
        _buffer.insert(_buffer.end(), buff, buff + len);
        int size = _buffer.size();
        for (int i = 0; i < size; ++i) {
            if (i < size - 4 && _buffer[i] == '\r'
                             && _buffer[i + 1] == '\n'
                             && _buffer[i + 2] == '\r'
                             && _buffer[i + 3] == '\n') {
                char *data = _buffer.data();
                memcpy(buff, data, i);
                buff[i] = '\0';
                _buffer.erase(_buffer.begin(), _buffer.begin() + i + 4);
                parseMessage(buff);
                break;
            }
        }
    }
}

void Client::parseMessage(const char *data) {
    std::stringstream l(data);
    std::string method;
    int id = -1;
    int seqno = -1;
    int length = -1;
    l >> method >> id >> seqno >> length;
    // check data
    if (method == "READ") {
        bufferData(length);
    } else if (method == "NEW_TXN") {
        if (seqno != 0) {
            //TODO: Throw error at client
        }
        Transaction *t = new Transaction();
        respond("ACK", t->getId(), 0, 0, 0);
        close(_fd);
    } else if (method == "WRITE") {
        bufferData(length);
    } else if (method == "COMMIT") {
    } else if (method == "ABORT") {

    } else {
        std::cout << "invalid message " << id << std::endl;
    }
}

void Client::bufferData(int length) {
}

void Client::respond(const std::string &method, int id, int seqno, int error,
                     int len, const char *buff) {
    std::stringstream str;
    str << method << " " << id << " " << seqno << " " << error << " " << len
        << "\r\n\r\n";

    if (buff != nullptr) {
        str << buff;
    }
    str << "\r\n";
    const char *out = str.str().c_str();
    write(_fd, out, strlen(out));
}

int main() {
    int socketfd = socket(AF_INET, SOCK_STREAM, 0);
    // check socket
    sockaddr_in addr;
    uint16_t port = 7898;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(socketfd, (const sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cerr << "Could not bind to port: " << port << std::endl;
        return 1;
    }


    while (1) {
        listen(socketfd, 10);
        socklen_t len = 0;
        Client *c = new Client();
        c->_fd = accept(socketfd, (sockaddr *)&c->_addr, &len);
        c->_thread = std::thread(&Client::readThread, c);
    }
}
