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
#include <condition_variable>
#include <set>

#ifndef _MSC_VER
#include "c++14compat.h"
#endif

class Person;
class Shop;

std::mutex cout_mutex;
std::mutex rand_mutex;

std::condition_variable start_condition;
int threads_ready;

std::default_random_engine generator((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
std::uniform_int_distribution<int> distribution(2, 5);
int dice() {
	// Use a mutex for random values as the standard does not guarantee it to be thread safe
	std::lock_guard<std::mutex> lock(rand_mutex);
	return distribution(generator);
}


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
	const std::string &getName() const { return _name; }

	const std::vector<std::shared_ptr<Shop>> &getShops() { return _shops; }

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
	Shop(const std::string &name);

	bool enterShop(const Person *p);
	void leaveShop(const Person *p);

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

Shop::Shop(const std::string &name) : _name(name), _numDeath(0), _numAuror(0) {
}

bool Shop::enterShop(const Person *p) {
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

void Shop::leaveShop(const Person *p) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (p->getPosition() == Auror) {
		--_numAuror;
	} else {
		--_numDeath;
	}
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
	{
		std::unique_lock<std::mutex> lock(cout_mutex);
		++threads_ready;
		start_condition.wait(lock);
	}
	while (!areTasksDone()) {
		if (_timeLeftInShop != 0) {
			_timeLeftInShop--;
		}
		if (_currentShop) {
			bool canEnter = false;
			bool doneShopping = false;
			bool justLeft = false;
			if (_shops.size() != 0 && _shops[0]->enterShop(this)) {
				canEnter = true;
			} else if (_shops.size() == 0) {
				doneShopping = true;
			}
			{
				std::lock_guard<std::mutex> lock(cout_mutex);
				if (canEnter) {
					std::cout << *this << " makes a reservation at the " << _shops[0]->getName() << " shop." <<  std::endl;
					std::cout << *this << " leaves the " << _currentShop->getName() << " shop to apparate to the next destination." << std::endl;
					justLeft = true;
				} else if (doneShopping) {
					std::cout << *this << " is done with shopping, so he leaves the " << _currentShop->getName() << " shop." << std::endl;
					justLeft = true;
				} else if (_timeLeftInShop == 0) {
					std::cout << *this << " is bored talking to the salesperson, so he leaves the " << _currentShop->getName() << " shop without a reservation for the next shop to go for a walk." << std::endl;
					justLeft = true;
				}
			}
			if (justLeft) {
				_currentShop->leaveShop(this);
				_currentShop = nullptr;
				if (canEnter) {
					_currentShop = _shops[0];
					_shops.erase(_shops.begin());
					{
						std::lock_guard<std::mutex> lock(cout_mutex);
						std::cout << *this << " enters the " << _currentShop->getName() << " shop." <<  std::endl;
					}
				}
			}
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
		++_second;
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Only supported argument is a single filename" << std::endl;
		return -1;
	}

	std::vector<std::unique_ptr<Person>> people;
	std::map<std::string, std::shared_ptr<Shop>> shops;
	std::set<std::string> usedNames;

	std::ifstream file;
	file.open(argv[1], std::ios::in);

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream l(line);
		std::string name, position;
		l >> name >> position;

		if (usedNames.find(name) == usedNames.end()) {
			usedNames.insert(name);
		} else {
			std::cerr << "Name already used: " << name << std::endl;
			return -1;
		}

		std::vector<std::string> s = std::vector<std::string>(
				std::istream_iterator<std::string>(l),
				std::istream_iterator<std::string>());

		std::vector<std::shared_ptr<Shop>> s2;
		std::set<std::string> usedShopNames;

		for (std::string shop : s) {
			if (usedShopNames.find(shop) == usedShopNames.end()) {
				usedShopNames.insert(shop);
			} else {
				std::cerr << "Shop name already used: " << shop << std::endl;
				return -1;
			}
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
			std::cerr << "Invalid position " << position << std::endl;
			return -1;
		}

		std::unique_ptr<Person> p = std::make_unique<Person>(name, pos, s2);
		people.push_back(std::move(p));

	}
	{
		std::lock_guard<std::mutex> lock(cout_mutex);
		for (auto &p : people) {
			std::cout << p->getName() << " " << p->getPosition() << " ";
			for (auto &s : p->getShops()) {
				std::cout << s->getName() << " ";
			}

			std::cout << std::endl;
		}
		std::cout << std::endl;
	}

	for (auto &p : people) {
		p->run();
	}

	while (1) {
		std::lock_guard<std::mutex> lock(cout_mutex);
		if (threads_ready == people.size()) {
			break;
		}
	}
	start_condition.notify_all();

	for (auto &p : people) {
		p->join();
	}
}
