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
#include <set>
#include <signal.h>
#include <map>
#include <mutex>
#include <queue>
#include <condition_variable>

class File {
public:
	File(std::string filename) : _filename(filename), _created(false) {}
	std::string _filename;
	std::vector<char> _contents;
	bool _created;
	std::mutex _mutex;
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
	static std::mutex _usedIds_mutex;
	std::string _filename;
	File *_file;
	std::map<int, std::string> _writes;
	void write();
	void writeData(int seqno, const std::string &data);
	TransactionState _state;
	std::mutex _mutex;
};


std::set<int> Transaction::_usedIds;
std::mutex Transaction::_usedIds_mutex;
std::map<std::string, File *> _files;
std::map<int, Transaction *> _transactions;
std::mutex _transaction_mutex, _file_mutex;

Transaction::Transaction(std::string filename) : _filename(filename) {
	std::lock_guard<std::mutex> lock(_usedIds_mutex);
	int id = rand();
	while (_usedIds.find(id) != _usedIds.end()) {
		id = rand();
	}
	_usedIds.insert(id);
	_id = id;
	_state = Valid;
	_file = nullptr;
}

void Transaction::write() {
	std::lock_guard<std::mutex> lock(_file->_mutex);
	_file->_created = true;
	for (auto it = _writes.begin(); it != _writes.end(); ++it) {
		_file->_contents.insert(_file->_contents.end(), it->second.begin(), it->second.end());
	}
}

void Transaction::writeData(int seqno, const std::string &data) {
	_writes[seqno] = data;
}

class Client {
public:
	Client();
	~Client();
	void readThread();
	void parseMessage(const char *);
	void bufferData(int length);
	void respond(const std::string &method, int id, int seqno, int error, const char *buff = nullptr);
	void disconnect();
	Transaction *findTransaction(int id);

	int _fd;
	sockaddr_in _addr;
private:
	std::vector<char> _buffer;
	bool _connected;
};



Client::Client() {
	_connected = true;
	_fd = 0;
}

Client::~Client() {
	if (_fd != 0) {
		close(_fd);
	}
}

void Client::readThread() {
	//std::cout << "got a connection" << std::endl;
	while (_connected) {
		char buff[256];
		ssize_t len = read(_fd, buff, 255);
		if (len <= 0) {
			break;
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
	//std::cout << "connection close" << std::endl;
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
		//std::cout << "Reading file " << filename << std::endl;
		std::lock_guard<std::mutex> lock(_file_mutex);
		File *f = nullptr;
		if (_files.find(filename) != _files.end()) {
			f = _files[filename];
			f->_mutex.lock();
		}
		if (f == nullptr || f->_created == false) {
			if (f) {
				f->_mutex.unlock();
			}
			respond("ERROR", 0, 0, 206, "File not found");
			disconnect();
			return;
		}
		std::string out(f->_contents.data(), f->_contents.size());
		f->_mutex.unlock();
		respond("ACK", 0, 0, 0, out.c_str());
	} else if (method == "NEW_TXN") {
		if (seqno != 0) {
			//TODO: Throw error at client
		}
		bufferData(length);
		std::string filename(_buffer.data(), length);
		Transaction *t = new Transaction(filename);
		respond("ACK", t->getId(), 0, 0);
		std::lock_guard<std::mutex> lock(_file_mutex);
		File *f;
		if (_files.find(filename) == _files.end()) {
			f = new File(filename);
			_files[filename] = f;
		} else {
			f = _files[filename];
		}
		t->_file = f;
		_transaction_mutex.lock();
		_transactions[t->getId()] = t;
		_transaction_mutex.unlock();
	} else if (method == "WRITE") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		bufferData(length);
		std::string data(_buffer.data(), length);
		t->writeData(seqno, data);
		int idout = t->getId();
		t->_mutex.unlock();
		respond("ACK", idout, seqno, 0);
	} else if (method == "COMMIT") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		t->write();
		int idout = t->getId();
		t->_state = Commited;
		t->_mutex.unlock();
		respond("ACK", idout, seqno, 0);
	} else if (method == "ABORT") {
		Transaction *t = findTransaction(id);
		if (!t) {
			return;
		}
		t->_state = Aborted;
		t->_mutex.unlock();
	} else {
		respond("ERROR", 0, 0, 204, "Wrong message format");
	}
	disconnect();
}

Transaction *Client::findTransaction(int id) {
	std::lock_guard<std::mutex> lock(_transaction_mutex);
	if (_transactions.find(id) == _transactions.end()) {
		respond("ERROR", id, 0, 201, "Invalid transaction ID");
		disconnect();
		return nullptr;
	}
	Transaction *t = _transactions[id];
	t->_mutex.lock();
	if (t->_state == Commited || t->_state == Aborted) {
		t->_mutex.unlock();
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
		if (len <= 0) {
			return;
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

bool _endThreads = false;
std::set<Client *> _clients;
std::vector<std::thread> _workers;

std::mutex workMutex;
std::condition_variable workCondition;
std::queue<Client *> workQueue;
int bindfd = 0;

void cleanup() {
	if (_endThreads) {
		return;
	}
	_endThreads = true;
	workCondition.notify_all();
	for (auto &worker : _workers) {
		worker.join();
	}
	for (auto c : _clients) {
		delete c;
	}
	for (auto t : _transactions) {
		delete t.second;
	}
	for (auto f : _files) {
		delete f.second;
	}
	close(bindfd);
}

void my_handler(int param) {
	cleanup();
	exit(0);
}


void workThread() {
	while (!_endThreads) {
		Client *c = nullptr;
		{
			std::unique_lock<std::mutex> lock(workMutex);
			while (workQueue.size() == 0) {
				workCondition.wait(lock);
				if (_endThreads) {
					return;
				}
			}
			c = workQueue.front();
			workQueue.pop();
			_clients.erase(c);
		}
		c->readThread();
		delete c;
	}
}

int main(int argc, char *argv[]) {
	uint16_t port = 7899;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	signal (SIGINT, my_handler);
	bindfd = socket(AF_INET, SOCK_STREAM, 0);
	// check socket
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);

	if (bind(bindfd, (const sockaddr *)&addr, sizeof(addr)) < 0) {
		std::cerr << "Could not bind to port: " << port << std::endl;
		return 1;
	}

	for (int i = 0; i < 32; ++i) {
		_workers.push_back(std::thread(workThread));
	}

	while (1) {
		int ret = listen(bindfd, 10);
		if (ret < 0) {
			std::cerr << "Listen error: " << ret << std::endl;
			return 1;
		}
		socklen_t len = 0;
		Client *c = new Client();
		_clients.insert(c);
		int fd = accept(bindfd, (sockaddr *)&c->_addr, &len);
		if (fd <= 0) {
			std::cerr << "Accept error: " << fd << std::endl;
			break;
		}
		c->_fd = fd;
		{
			std::lock_guard<std::mutex> lock(workMutex);
			workQueue.push(c);
		}
		workCondition.notify_all();
	}
	cleanup();
}
