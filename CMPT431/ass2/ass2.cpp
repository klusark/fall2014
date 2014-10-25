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
#include <fcntl.h>
#include <sys/stat.h>

bool verbose = false;

std::mutex _transaction_mutex, _file_mutex;
std::string outDir = "";

class File {
public:
	File(std::string filename) : _filename(filename), _created(false) {
		_path = outDir + "/" + _filename;
	}
	static File *getFile(const std::string &filename, bool create = true);
	void write(const std::string &data);
	int getLength();
	void readTo(int fd);
	std::string _filename;
	std::string _path;
	bool _created;
	std::mutex _mutex;
};

std::map<std::string, File *> _files;

File *File::getFile(const std::string &filename, bool create) {
	_file_mutex.lock();
	File *f = nullptr;
	if (_files.find(filename) != _files.end()) {
		f = _files[filename];
		f->_mutex.lock();
	} else if (create) {
		f = new File(filename);
		f->_mutex.lock();
		_files[filename] = f;
	}
	_file_mutex.unlock();
	return f;
}

void File::write(const std::string &data) {
	int fd = open(_path.c_str(), O_WRONLY|O_APPEND|O_CREAT, 0777);
	::write(fd, data.c_str(), data.length());
	close(fd);
}

void File::readTo(int fd) {
	int infd = open(_path.c_str(), O_RDONLY);
	char buff[1024];
	while (1) {
		size_t len = read(infd, buff, 1024);
		::write(fd, buff, len);
		if (len != 1024) {
			break;
		}
	}
	close(infd);
}

int File::getLength() {
	struct stat st;
	stat(_path.c_str(), &st);
	return st.st_size;
}

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
std::map<int, Transaction *> _transactions;

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
		_file->write(it->second);
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
	void respondHeader(const std::string &method, int id, int seqno, int error, int content_length);
	void respond(const std::string &method, int id, int seqno, int error, const char *buff = nullptr);
	void respond(const std::string &method, int id, int seqno, int error, File *f);
	void disconnect();
	Transaction *findTransaction(int id, int seqno);

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
	if (verbose) {
		std::cout << "Get: " << data << std::endl;
	}
	// check data
	if (method == "READ") {
		bufferData(length);
		std::string filename(_buffer.data(), length);
		File *f = File::getFile(filename, false);
		if (f == nullptr || f->_created == false) {
			if (f) {
				f->_mutex.unlock();
			}
			respond("ERROR", 0, 0, 206, "File not found");
			disconnect();
			return;
		}
		respond("ACK", 0, 0, 0, f);
	} else if (method == "NEW_TXN") {
		bufferData(length);
		std::string filename(_buffer.data(), length);
		if (filename.size() == 0 || seqno != 0) {
			respond("ERROR", id, seqno, 204, "Wrong message format");
		} else {
			Transaction *t = new Transaction(filename);
			File *f = File::getFile(filename, true);
			t->_file = f;
			f->_mutex.unlock();
			_transaction_mutex.lock();
			_transactions[t->getId()] = t;
			_transaction_mutex.unlock();
			respond("ACK", t->getId(), 0, 0);
		}
	} else if (method == "WRITE") {
		Transaction *t = findTransaction(id, seqno);
		if (!t) {
			return;
		}
		bufferData(length);
		std::string data(_buffer.data(), length);
		if (data.size() == 0) {
			t->_mutex.unlock();
			respond("ERROR", id, seqno, 204, "Wrong message format");
		} else {
			t->writeData(seqno, data);
			t->_mutex.unlock();
			respond("ACK", id, seqno, 0);
		}
	} else if (method == "COMMIT") {
		Transaction *t = findTransaction(id, seqno);
		if (!t) {
			return;
		}
		int last = 0;
		bool stillwrite = true;
		for (auto write : t->_writes) {
			if (write.first != last + 1) {
				for (int i = last + 1; i < write.first; ++i) {
					respond("ASK_RESEND", id, i, 0);
					stillwrite = false;
				}
			}
			last = write.first;
		}
		if (stillwrite) {
			t->write();
			t->_state = Commited;
			t->_mutex.unlock();
			respond("ACK", id, seqno, 0);
		} else {
			t->_mutex.unlock();
		}
	} else if (method == "ABORT") {
		Transaction *t = findTransaction(id, seqno);
		if (!t) {
			return;
		}
		t->_state = Aborted;
		t->_mutex.unlock();
		respond("ACK", id, seqno, 0);
	} else {
		respond("ERROR", 0, 0, 204, "Wrong message format");
	}
	disconnect();
}

Transaction *Client::findTransaction(int id, int seqno) {
	std::lock_guard<std::mutex> lock(_transaction_mutex);
	if (_transactions.find(id) == _transactions.end()) {
		respond("ERROR", id, seqno, 201, "Invalid transaction ID");
		disconnect();
		return nullptr;
	}
	Transaction *t = _transactions[id];
	t->_mutex.lock();
	if (t->_state == Commited || t->_state == Aborted) {
		t->_mutex.unlock();
		respond("ERROR", id, seqno, 202, "Invalid operation");
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
	_connected = false;
}

void Client::respond(const std::string &method, int id, int seqno, int error,
					 File *f) {
	int len = f->getLength();
	respondHeader(method, id, seqno, error, len);
	f->readTo(_fd);
	write(_fd, "\r\n", 2);
	f->_mutex.unlock();
}

void Client::respond(const std::string &method, int id, int seqno, int error,
					 const char *buff) {
	int len = 0;
	if (buff != nullptr) {
		len = strlen(buff);
	}
	respondHeader(method, id, seqno, error, len);
	write(_fd, buff, len);
	write(_fd, "\r\n", 2);
}

void Client::respondHeader(const std::string &method, int id, int seqno, int error,
					 int content_length) {
	std::stringstream str;
	str << method << " " << id << " " << seqno << " " << error << " " << content_length
		<< "\r\n\r\n";

	std::string s = str.str();
	if (verbose) {
		std::cout << "Send: " << s << std::endl;
	}
	write(_fd, s.c_str(), s.length());
}

bool _endThreads = false;
std::vector<std::thread> _workers;

std::mutex workMutex, clientsMutex;
std::condition_variable workCondition;
std::queue<int> workQueue;
int bindfd = 0;

void cleanup() {
	if (_endThreads) {
		return;
	}
	{
		std::lock_guard<std::mutex> lock(workMutex);
		_endThreads = true;
		workCondition.notify_all();
	}
	for (auto &worker : _workers) {
		worker.join();
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


void workThread(int threadid) {
	while (!_endThreads) {
		int fd = 0;
		{
			std::unique_lock<std::mutex> lock(workMutex);
			while (workQueue.size() == 0) {
				workCondition.wait(lock);
				if (_endThreads) {
					return;
				}
			}
			fd = workQueue.front();
			workQueue.pop();
		}
		if (verbose) {
			std::cout << "Thread working: " << threadid << std::endl;
		}
		Client c;
		c._fd = fd;
		c.readThread();
		if (verbose) {
			std::cout << "Thread done: " << threadid << std::endl;
		}
	}
}

void usage(const char *name) {
	std::cerr <<
				"Usage: " << name << " [OPTIONS]" << std::endl <<
				"  -h\t\tPrint this help" << std::endl <<
				"  -ip\t\tThe ip address to bind to" << std::endl <<
				"  -port\t\tThe porn to bind to" << std::endl <<
				"  -dir\t\tThe dir with files" << std::endl;
}

int main(int argc, char *argv[]) {
	std::string port = "8080";
	std::string address = "127.0.0.1";
	for (int i = 1; i < argc - 1; ++i) {
		std::string arg = argv[i];
		if (arg == "-dir") {
			outDir = argv[i + 1];
		} else if (arg == "-port") {
			port = argv[i + 1];
		} else if (arg == "-ip") {
			address = argv[i + 1];
		} else if (arg == "-h") {
			usage(argv[0]);
			return 1;
		}
	}
	if (outDir == "") {
		usage(argv[0]);
		return 1;
	}
	signal (SIGINT, my_handler);


	addrinfo hints;
	addrinfo *addr = nullptr;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(address.c_str(), port.c_str(), &hints, &addr) < 0) {
		std::cerr << "Could not get address: " << address << std::endl;
		return 1;
	}

	bindfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
	if (bindfd < 0) {
		std::cerr << "Could not create socket" << std::endl;
		return 1;
	}

	if (bind(bindfd, addr->ai_addr, addr->ai_addrlen) < 0) {
		std::cerr << "Could not bind to port: " << port << std::endl;
		return 1;
	}

	for (int i = 0; i < 32; ++i) {
		_workers.push_back(std::thread(workThread, i));
	}

	while (1) {
		int ret = listen(bindfd, 10);
		if (ret < 0) {
			std::cerr << "Listen error: " << ret << std::endl;
			return 1;
		}
		socklen_t len = 0;
		sockaddr_in addr;
		int fd = accept(bindfd, (sockaddr *)&addr, &len);
		if (fd <= 0) {
			std::cerr << "Accept error: " << fd << std::endl;
			break;
		}
		{
			std::lock_guard<std::mutex> lock(workMutex);
			workQueue.push(fd);
			workCondition.notify_one();
		}
	}
	cleanup();
}
