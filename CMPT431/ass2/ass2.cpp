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
#include <signal.h>
#include <map>

class File {
public:
	File(std::string filename) : _filename(filename), _created(false) {}
	std::string _filename;
	std::vector<char> _contents;
	bool _created;
};

enum TransactionState {
	Valid,
	Commited,
	Aborted,
};

class Transaction {
public:
	Transaction(std::string filename);
	int _id;
	int getId() { return _id; }
	static std::set<int> _usedIds;
	std::string _filename;
	File *_file;
	std::map<int, std::string> _writes;
	void write();
	TransactionState _state;
};


std::set<int> Transaction::_usedIds;
std::map<std::string, File *> _files;
std::map<int, Transaction *> _transactions;

Transaction::Transaction(std::string filename) : _filename(filename) {
	std::default_random_engine generator;
	std::uniform_int_distribution<int> distribution(1, 0x7fffffff);
	int id = distribution(generator);
	while (_usedIds.find(id) != _usedIds.end()) {
		id = distribution(generator);
	}
	_usedIds.insert(id);
	_id = id;
	_state = Valid;
}

void Transaction::write() {
	_file->_created = true;
	for (auto it = _writes.begin(); it != _writes.end(); ++it) {
		_file->_contents.insert(_file->_contents.end(), it->second.begin(), it->second.end());
	}
}

class Client {
public:
	sockaddr_in _addr;
	std::thread _thread;
	int _fd;
	std::vector<char> _buffer;
	bool _connected;
	Client();
	void readThread();
	void parseMessage(const char *);
	void bufferData(int length);
	void respond(const std::string &method, int id, int seqno, int error, const char *buff = nullptr);
	void disconnect();
	Transaction *findTransaction(int id);
};




Client::Client() {
	_connected = true;
}

void Client::readThread() {
	std::cout << "got a connection" << std::endl;
	while (_connected) {
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
	std::cout << "connection close" << std::endl;
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
		std::string filename(_buffer.data(), length);
		std::cout << "Reading file " << filename << std::endl;
		File *f = nullptr;
		if (_files.find(filename) != _files.end()) {
			f = _files[filename];
		}
		if (f == nullptr || f->_created == false) {
			respond("ERROR", 0, 0, 206, "File not found");
			disconnect();
			return;
		}
		std::string out(f->_contents.data(), f->_contents.size());
		respond("ACK", 0, 0, 0, out.c_str());
	} else if (method == "NEW_TXN") {
		if (seqno != 0) {
			//TODO: Throw error at client
		}
		bufferData(length);
		std::string filename(_buffer.data(), length);
		Transaction *t = new Transaction(filename);
		_transactions[t->getId()] = t;
		respond("ACK", t->getId(), 0, 0);
		File *f;
		if (_files.find(filename) == _files.end()) {
			f = new File(filename);
			_files[filename] = f;
		} else {
			f = _files[filename];
		}
		t->_file = f;
	} else if (method == "WRITE") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		bufferData(length);
		std::string data(_buffer.data(), length);
		t->_writes[seqno] = data;
		respond("ACK", t->getId(), seqno, 0);
	} else if (method == "COMMIT") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		t->write();
		respond("ACK", t->getId(), seqno, 0);
		t->_state = Commited;
	} else if (method == "ABORT") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		t->_state = Aborted;
	} else {
		respond("ERROR", 0, 0, 204, "Wrong message format");
	}
	disconnect();
}

Transaction *Client::findTransaction(int id) {
	if (_transactions.find(id) == _transactions.end()) {
		respond("ERROR", 0, 0, 201, "Invalid transaction ID");
		disconnect();
		return nullptr;
	}
	Transaction *t = _transactions[id];
	if (t->_state == Commited || t->_state == Aborted) {
		respond("ERROR", 0, 0, 202, "Invalid operation");
		disconnect();
		return nullptr;
	}
	return t;
}

void Client::bufferData(int length) {
	int left = length;
	while (_buffer.size() < length) {
		char buff[256];
		int toread = 256;
		if (left < 256) {
			toread = left;
		}
		ssize_t len = read(_fd, buff, toread);
		if (len == -1) {
			//TODO: Throw an error
		}
		left -= len;
		_buffer.insert(_buffer.end(), buff, buff + len);
	}
}

void Client::disconnect() {
	close(_fd);
	_connected = false;
}

void Client::respond(const std::string &method, int id, int seqno, int error,
					 const char *buff) {
	std::stringstream str;
	int len = 0;
	if (buff != nullptr) {
		len = strlen(buff);
	}
	str << method << " " << id << " " << seqno << " " << error << " " << len
		<< "\r\n\r\n";

	if (buff != nullptr) {
		str << buff;
	}
	str << "\r\n";
	std::string s = str.str();
	write(_fd, s.c_str(), s.length());
}

void cleanup() {
}

void my_handler(int param) {
	cleanup();
	exit(0);
}

int main(int argc, char *argv[]) {
	uint16_t port = 7899;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	signal (SIGINT, my_handler);
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	// check socket
	sockaddr_in addr;
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
