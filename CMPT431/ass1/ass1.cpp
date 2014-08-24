#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <map>
#include <pthread.h>
#include <unistd.h>
//#include <semaphore.h>

class Person;
class Shop;

enum Position
{
	Auror,
	DeathEater,
};

class Person
{
public:
	Person(const std::string &name, Position pos, const std::vector<Shop *> &shops);
	void run();
	void join();
	Position getPosition() const { return _pos; }

private:
	static void *threadInit(void *in);
	void threadRun();
	bool areTasksDone() {
		return _shops.size() == 0 && !_currentShop;
	}

	std::string _name;
	Position _pos;
	std::vector<Shop *> _shops;
	pthread_t _thread;
	uint32_t _timeLeftInShop;
	Shop *_currentShop;
};

class Shop
{
public:
	Shop(const std::string &name) : _name(name), _numDeath(0), _numAuror(0) {
		//sem_init(&_deathSem, 0, 0);
		//sem_init(&_aurorSem, 0, 0);
		pthread_mutex_init(&_mutex, nullptr);
	}

	bool enterShop(Person *p) {
		pthread_mutex_lock(&_mutex);
		bool val = false;
		if (p->getPosition() == Auror) {
			if (_numDeath == 0) {
				_numAuror++;
				val = true;
			}
		} else {
			if (_numAuror == 0) {
				_numDeath++;
				val = true;
			}
		}
		pthread_mutex_unlock(&_mutex);
		return val;
	}

	void leaveShop(Person *p) {
		pthread_mutex_lock(&_mutex);
		if (p->getPosition() == Auror) {
			--_numAuror;
		} else {
			--_numDeath;
		}
		pthread_mutex_unlock(&_mutex);
	}

	const std::string &getName() const { return _name; }
private:
	std::string _name;
	//sem_t _deathSem;
	//sem_t _aurorSem;
	int _numDeath;
	int _numAuror;
	pthread_mutex_t _mutex;
};

Person::Person(const std::string &name, Position pos, const std::vector<Shop *> &shops) : 
	_name(name), _pos(pos), _shops(shops), _currentShop(nullptr) {
}

void Person::run() {
	pthread_create(&_thread, nullptr, threadInit, (void *)this);
}

void Person::join() {
	pthread_join(_thread, nullptr);
}

void *Person::threadInit(void *in) {
	static_cast<Person *>(in)->threadRun();

	return nullptr;
}

void Person::threadRun() {
	while (!areTasksDone()) {
		if (_timeLeftInShop != 0) {
			_timeLeftInShop--;
		} else if (_timeLeftInShop == 0 && _currentShop){
			_currentShop->leaveShop(this);
			std::cout << _name << " leave shop " << _currentShop->getName() << std::endl;
			_currentShop = nullptr;
		}

		if (!_currentShop && _shops.size() != 0 && _shops[0]->enterShop(this)) {
			_currentShop = _shops[0];
			std::cout << _name << " enter shop " << _currentShop->getName() <<  std::endl;
			_shops.erase(_shops.begin());
			_timeLeftInShop = 10;
		}
		usleep(100000);
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Only supported argument is a single filename" << std::endl;
		return -1;
	}

	std::vector<Person *> people;
	std::map<std::string, Shop *> shops;

	std::ifstream file;
	file.open(argv[1], std::ios::in);

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream l(line);
		std::string name, position;
		l >> name >> position;
		std::vector<std::string> s = std::vector<std::string>(std::istream_iterator<std::string>(l), std::istream_iterator<std::string>());
		std::vector<Shop *> s2;

		for (std::string shop : s) {
			if (shops.find(shop) == shops.end()) {
				shops[shop] = new Shop(shop);
			}
			s2.push_back(shops[shop]);
		}

		Position pos;
		if (position == "DeathEater") {
			pos = DeathEater;
		} else if (position == "Auror") {
			pos = Auror;
		} else {
			std::cout << "Invalid position " << position << std::endl;
			return -1;
		}
		std::cout << name << " " << position << " ";
		std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, " "));
		std::cout << std::endl;
		Person *p = new Person(name, pos, s2);
		p->run();
		people.push_back(p);
	}

	for (Person *p : people) {
		p->join();
	}
}
