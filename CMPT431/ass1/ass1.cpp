#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <map>
#include <thread>
#include <memory>
#include <mutex>
#include <chrono>
#include <thread>
#include <random>

#ifndef _MSC_VER
#include "c++14compat.h"
#endif

class Person;
class Shop;

std::mutex cout_mutex;

std::default_random_engine generator((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
std::uniform_int_distribution<int> distribution(1,6);
auto dice = std::bind ( distribution, generator );

enum Position
{
	Auror,
	DeathEater,
};

class Person
{
public:
	Person(const std::string &name, Position pos, const std::vector<std::shared_ptr<Shop>> &shops);
	void run();
	void join();
	Position getPosition() const { return _pos; }

	friend std::ostream& operator<< (std::ostream &out, Person &p);

private:
	static void threadInit();
	void threadRun();
	bool areTasksDone() {
		return _shops.size() == 0 && !_currentShop;
	}

	std::string _name;
	Position _pos;
	std::vector<std::shared_ptr<Shop>> _shops;
	std::thread _thread;
	uint32_t _timeLeftInShop;
	std::shared_ptr<Shop> _currentShop;
	uint32_t _second;
};

class Shop
{
public:
	Shop(const std::string &name) : _name(name), _numDeath(0), _numAuror(0) {
	}

	bool canEnterShop(const Person *p) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (p->getPosition() == Auror) {
			return _numDeath == 0;
		} else {
			return _numAuror == 0;
		}
	}
	bool enterShop(const Person *p) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (p->getPosition() == Auror) {
			if (_numDeath == 0) {
				_numAuror++;
				return true;
			}
		} else {
			if (_numAuror == 0) {
				_numDeath++;
				return true;
			}
		}
		return false;
	}

	void leaveShop(const Person *p) {
		std::lock_guard<std::mutex> lock(_mutex);
		if (p->getPosition() == Auror) {
			--_numAuror;
		} else {
			--_numDeath;
		}
	}

	const std::string &getName() const { return _name; }
private:
	std::string _name;
	int _numDeath;
	int _numAuror;
	std::mutex _mutex;
};

std::ostream& operator<< (std::ostream &out, Position p) {
	if (p == DeathEater) {
		out << "DeathEater";
	} else {
		out << "Auror";
	}
	return out;
}

std::ostream& operator<< (std::ostream &out, Person &p) {
	out << "tid " << p._thread.get_id() << ": second " << p._second <<": " << p._name << " (" << p._pos << ")";
	return out;
}

Person::Person(const std::string &name, Position pos, const std::vector<std::shared_ptr<Shop>> &shops) :
	_name(name), _pos(pos), _shops(shops), _currentShop(nullptr),
	_timeLeftInShop(0), _second(0) {
}

void Person::run() {
	_thread = std::thread(&Person::threadRun, this);
}

void Person::join() {
	_thread.join();
}


void Person::threadRun() {
	while (!areTasksDone()) {
		++_second;
		if (_timeLeftInShop != 0) {
			_timeLeftInShop--;
		} else if (_timeLeftInShop == 0 && _currentShop){
			_currentShop->leaveShop(this);
			bool canEnter = false;
			if (!_currentShop && _shops.size() != 0 && _shops[0]->canEnterShop(this)) {
				canEnter = true;
			}
			{
				std::lock_guard<std::mutex> lock(cout_mutex);
				if (canEnter) {
					std::cout << *this << " leaves the " << _currentShop->getName() << " shop to apparate to the next destination." << std::endl;
				} else {
					std::cout << *this << " is bored talking to the salesperson, so she leaves the " << _currentShop->getName() << " without a reservation for the next shop to go for a walk." << std::endl;
				}
			}
			_currentShop = nullptr;
		}

		if (!_currentShop && _shops.size() != 0 && _shops[0]->enterShop(this)) {
			_currentShop = _shops[0];
			{
				std::lock_guard<std::mutex> lock(cout_mutex);
				std::cout << *this << " makes a reservation at the " << _currentShop->getName() << " shop." <<  std::endl;
				std::cout << *this << " enters the " << _currentShop->getName() << " shop." <<  std::endl;
			}
			_shops.erase(_shops.begin());
			_timeLeftInShop = dice();
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Only supported argument is a single filename" << std::endl;
		return -1;
	}

	std::vector<std::unique_ptr<Person>> people;
	std::map<std::string, std::shared_ptr<Shop>> shops;

	std::ifstream file;
	file.open(argv[1], std::ios::in);

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream l(line);
		std::string name, position;
		l >> name >> position;

		std::vector<std::string> s = std::vector<std::string>(
				std::istream_iterator<std::string>(l),
				std::istream_iterator<std::string>());

		std::vector<std::shared_ptr<Shop>> s2;

		for (std::string shop : s) {
			if (shops.find(shop) == shops.end()) {
				shops[shop] = std::make_shared<Shop>(shop);
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

		{
			std::lock_guard<std::mutex> lock(cout_mutex);
			std::cout << name << " " << position << " ";
			std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, " "));
			std::cout << std::endl;
		}
		std::unique_ptr<Person> p = std::make_unique<Person>(name, pos, s2);
		people.push_back(std::move(p));

	}

	for (std::unique_ptr<Person> &p : people) {
		p->run();
	}

	for (std::unique_ptr<Person> &p : people) {
		p->join();
	}
}
