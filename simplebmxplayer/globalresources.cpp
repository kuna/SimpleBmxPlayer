#include "globalresources.h"

StringPool* STRPOOL = 0;
DoublePool* DOUBLEPOOL = 0;
IntPool* INTPOOL = 0;
TimerPool* TIMERPOOL = 0;
HandlerPool* HANDLERPOOL = 0;

ImagePool* IMAGEPOOL = 0;
FontPool* FONTPOOL = 0;
SoundPool* SOUNDPOOL = 0;

/* some commonly used routine */
namespace {
	template<typename A, typename B>
	bool exists(std::map<A, B> &m, A& key) {
		return (m.find(key) != m.end())
	}
}

bool TimerPool::IsExists(const RString &key) {
	return _timerpool.find(key) != _timerpool.end();
}

Timer* TimerPool::Set(const RString &key, bool activate) {
	if (!IsExists(key)) {
		Timer t;
		if (activate)
			t.Start();
		_timerpool.insert(std::pair<RString, Timer>(key, t));
	}
	return &_timerpool[key];
}

Timer* TimerPool::Get(const RString &key) {
	if (!IsExists(key))
		return 0;
	return &_timerpool[key];
}