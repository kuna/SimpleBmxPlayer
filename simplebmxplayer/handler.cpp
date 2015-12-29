#include "handler.h"
#include <vector>

namespace Handler {
	std::vector<void(*)(void*)> events[MAX_HANDLER_COUNT];

	void AddHandler(int h, void(*f)(void*)) {
		events[h].push_back(f);
	}

	void RemoveHandler(int h, void(*f)(void*)) {
		for (auto it = events[h].begin(); it != events[h].end(); ++it) {
			if ((*it) == f) {
				events[h].erase(it);
				break;
			}
		}
	}

	void CallHandler(int h, void* arg) {
		for (auto it = events[h].begin(); it != events[h].end(); ++it) {
			(*it)(arg);
		}
	}
}