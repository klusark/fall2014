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
#include <linux/limits.h>
#include <sqlite3.h>

bool verbose = false;

std::mutex _transaction_mutex, _file_mutex;
std::string outDir = "";
bool _endThreads = false;
sqlite3 *_db;
sqlite3_stmt *_transaction_stmt, *_update_stmt, *_write_stmt, *_delete_stmt;

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
	} else {
		std::string path = outDir + "/" + filename;
		struct stat st;
		bool created = false;
		int val = stat(path.c_str(), &st);
		if (val == 0) {
			created = true;
		}
		if (create || created) {
			f = new File(filename);
			f->_mutex.lock();
			_files[filename] = f;
			f->_created = created;
		}
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
	Transaction(std::string filename, bool create = true);
	int _id;
	int getId() { return _id; }
	static std::set<int> _usedIds;
	static std::mutex _usedIds_mutex;
	std::string _filename;
	File *_file;
	std::map<int, std::string> _writes;
	void write();
	void writeData(int seqno, const std::string &data);
	bool hasWrite(int seqno);
	void setState(TransactionState s);
	TransactionState _state;
	std::mutex _mutex;
};


std::set<int> Transaction::_usedIds;
std::mutex Transaction::_usedIds_mutex;
std::map<int, Transaction *> _transactions;

Transaction::Transaction(std::string filename, bool create) : _filename(filename) {
	_state = Valid;
	_file = nullptr;
	_id = 0;
	if (create) {
		int id = rand();
		_usedIds_mutex.lock();
		while (_usedIds.find(id) != _usedIds.end()) {
			id = rand();
		}
		_usedIds.insert(id);
		_usedIds_mutex.unlock();
		_id = id;
		sqlite3_bind_int(_transaction_stmt, 1, _id);
		sqlite3_bind_text(_transaction_stmt, 2, filename.c_str(),
							filename.length(), SQLITE_STATIC);
		sqlite3_bind_int(_transaction_stmt, 3, _state);
		sqlite3_step(_transaction_stmt);
		sqlite3_reset(_transaction_stmt);
	}
}
void Transaction::setState(TransactionState s) {
	_state = s;

	sqlite3_bind_int(_update_stmt, 1, _state);
	sqlite3_bind_int(_update_stmt, 2, _id);
	sqlite3_step(_update_stmt);
	sqlite3_reset(_update_stmt);

	if (s == Aborted) {
		_writes.clear();
		sqlite3_bind_int(_delete_stmt, 1, _id);
		sqlite3_step(_delete_stmt);
		sqlite3_reset(_delete_stmt);
	}
}

void Transaction::write() {
	std::lock_guard<std::mutex> lock(_file->_mutex);
	_file->_created = true;
	for (auto it = _writes.begin(); it != _writes.end(); ++it) {
		_file->write(it->second);
	}

	sqlite3_bind_int(_delete_stmt, 1, _id);
	sqlite3_step(_delete_stmt);
	sqlite3_reset(_delete_stmt);
}

void Transaction::writeData(int seqno, const std::string &data) {
	_writes[seqno] = data;

	sqlite3_bind_int(_write_stmt, 1, _id);
	sqlite3_bind_int(_write_stmt, 2, seqno);
	sqlite3_bind_text(_write_stmt, 3, data.c_str(), data.length(),
						SQLITE_STATIC);
	sqlite3_step(_write_stmt);
	sqlite3_reset(_write_stmt);
}

bool Transaction::hasWrite(int seqno) {
	return _writes.find(seqno) == _writes.end();
}

class Client {
public:
	Client();
	~Client();
	void readThread();
	void parseMessage(const char *);
	int bufferData(int length);
	void respondHeader(const std::string &method, int id, int seqno, int error, int content_length);
	void respond(const std::string &method, int id, int seqno, int error, const char *buff = nullptr);
	void respond(const std::string &method, int id, int seqno, int error, File *f);
	void disconnect();
	Transaction *findTransaction(int id, int seqno, bool noCommitErr = false);

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
	timeval tv;
	memset(&tv, 0, sizeof(tv));

	tv.tv_sec = 1;

	setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	while (_connected) {
		char buff[256];
		ssize_t len = read(_fd, buff, 255);
		if (len <= 0) {
			if (!_endThreads && _connected && errno == EAGAIN) {
				continue;
			}
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
}

void Client::parseMessage(const char *data) {
	std::stringstream l(data);
	std::string method;
	{
		bool error = false;
		int len = strlen(data);
		int numspaces = 0;
		for (int i = 0; i < len; ++i) {
			if (data[i] == ' ') {
				numspaces++;
				continue;
			}
			if (numspaces != 0) {
				if (!isdigit(data[i])) {
					error = true;
				}
			} else if (!(isalpha(data[i]) || data[i] == '_')) {
				error = true;
			}
		}
		if (numspaces != 3) {
			error = true;
		}
		if (error) {
			respond("ERROR", 0, 0, 204, "Wrong message format");
			disconnect();
			return;
		}
	}
	int id = -1;
	int seqno = -1;
	int length = -1;
	l >> method >> id >> seqno >> length;
	if (l.fail()) {
		respond("ERROR", 0, 0, 204, "Wrong message format");
		disconnect();
		return;
	}
	if (verbose) {
		std::cout << "Get: " << data << std::endl;
	}
	// check data
	if (method == "READ") {
		if (length <= 0 || length > PATH_MAX) {
			respond("ERROR", 0, 0, 206, "File not found");
			disconnect();
			return;
		}
		if (bufferData(length) < 0) {
			return;
		}
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
		if (length <= 0 || length > PATH_MAX) {
			respond("ERROR", 0, 0, 206, "File not found");
			disconnect();
			return;
		}
		if (bufferData(length) < 0) {
			return;
		}
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
		if (bufferData(length) < 0) {
			return;
		}
		std::string data(_buffer.data(), length);
		if (data.size() == 0) {
			t->_mutex.unlock();
			respond("ERROR", id, seqno, 204, "Wrong message format");
		} else {
			if (t->hasWrite(seqno)) {
				t->writeData(seqno, data);
				t->_mutex.unlock();
				respond("ACK", id, seqno, 0);
			} else {
				t->_mutex.unlock();
				respond("ERROR", id, seqno, 202, "Invalid operation.");
			}
		}
	} else if (method == "COMMIT") {
		if (length != 0) {
			respond("ERROR", 0, 0, 204, "Wrong message format");
			disconnect();
			return;
		}
		Transaction *t = findTransaction(id, seqno, true);
		if (!t) {
			return;
		}
		if (t->_state == Commited) {
			respond("ACK", id, seqno, 0);
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
			t->setState(Commited);
			t->_mutex.unlock();
			respond("ACK", id, seqno, 0);
		} else {
			t->_mutex.unlock();
		}
	} else if (method == "ABORT") {
		if (length != 0) {
			respond("ERROR", 0, 0, 204, "Wrong message format");
			disconnect();
			return;
		}
		Transaction *t = findTransaction(id, seqno);
		if (!t) {
			return;
		}
		t->setState(Aborted);
		t->_mutex.unlock();
		respond("ACK", id, seqno, 0);
	} else {
		respond("ERROR", 0, 0, 204, "Wrong message format");
		disconnect();
	}
}

Transaction *Client::findTransaction(int id, int seqno, bool noCommitErr) {
	std::lock_guard<std::mutex> lock(_transaction_mutex);
	if (_transactions.find(id) == _transactions.end()) {
		respond("ERROR", id, seqno, 201, "Invalid transaction ID");
		disconnect();
		return nullptr;
	}
	Transaction *t = _transactions[id];
	t->_mutex.lock();
	if ((!noCommitErr && t->_state == Commited) || t->_state == Aborted) {
		t->_mutex.unlock();
		respond("ERROR", id, seqno, 202, "Invalid operation");
		disconnect();
		return nullptr;
	}
	return t;
}

int Client::bufferData(int length) {
	int left = length;
	while (_buffer.size() < length) {
		char buff[256];
		int toread = 256;
		if (left < 256) {
			toread = left;
		}
		ssize_t len = read(_fd, buff, toread);
		if (len <= 0) {
			if (_connected && errno == EAGAIN) {
				continue;
			}
			return -1;
		}
		left -= len;
		_buffer.insert(_buffer.end(), buff, buff + len);
	}
	return 0;
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
	sqlite3_finalize(_transaction_stmt);
	sqlite3_finalize(_update_stmt);
	sqlite3_finalize(_write_stmt);
	sqlite3_finalize(_delete_stmt);
	sqlite3_close(_db);
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
	signal(SIGINT, my_handler);
	signal(SIGPIPE, SIG_IGN);

	std::string dblocation = outDir + "/.db";
	int rc = sqlite3_open(dblocation.c_str(), &_db);

	if (rc != 0 ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
	}

	srand(time(nullptr));

	const char *sql;
	sql = "CREATE TABLE IF NOT EXISTS Transactions("
			"id			INT PRIMARY KEY	NOT NULL,"
			"filename	TEXT			NOT NULL,"
			"state		INT				NOT NULL);";

	sqlite3_exec(_db, sql, nullptr, 0, nullptr);

	sql = "CREATE TABLE IF NOT EXISTS Writes("
			"tid	INT		NOT NULL,"
			"seqno	INT		NOT NULL,"
			"value	TEXT	NOT NULL);";
	sqlite3_exec(_db, sql, nullptr, 0, nullptr);

	sqlite3_prepare_v2(_db, "INSERT INTO Transactions VALUES(?,?,?);", -1,
						&_transaction_stmt, nullptr);

	sqlite3_prepare_v2(_db, "UPDATE Transactions SET state=(?) WHERE id=(?);",
						-1, &_update_stmt, nullptr);

	sqlite3_prepare_v2(_db, "INSERT INTO Writes VALUES(?,?,?);", -1,
						&_write_stmt, nullptr);

	sqlite3_prepare_v2(_db, "DELETE FROM Writes WHERE tid=(?);",
						-1, &_delete_stmt, nullptr);

	sqlite3_stmt *stmt;
	int ret = sqlite3_prepare_v2(_db, "SELECT * FROM Transactions;", -1, &stmt, nullptr);
	ret = sqlite3_step(stmt);
	while (ret == SQLITE_ROW) {
		int val = sqlite3_column_int(stmt, 0);
		Transaction::_usedIds.insert(val);
		Transaction *t = new Transaction((const char *)sqlite3_column_text(stmt, 1), false);
		t->_id = val;
		t->_state = (TransactionState)sqlite3_column_int(stmt, 2);
		ret = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);

	sqlite3_prepare_v2(_db, "SELECT * FROM Writes;", -1, &stmt, nullptr);
	ret = sqlite3_step(stmt);
	while (ret == SQLITE_ROW) {
		int id = sqlite3_column_int(stmt, 0);
		if (_transactions.find(id) != _transactions.end()) {
			Transaction *t = _transactions[id];
			t->writeData(sqlite3_column_int(stmt, 1), (const char *)sqlite3_column_text(stmt, 2));
		}
		ret = sqlite3_step(stmt);
	}
	sqlite3_finalize(stmt);

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
