#include "globalresources.h"

StringPool* STRPOOL = 0;
DoublePool* DOUBLEPOOL = 0;
IntPool* INTPOOL = 0;
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

