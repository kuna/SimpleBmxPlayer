#pragma once

#include <vector>
//#define _USEPOOL		// you can use SkinOption generally by disabling this definition
#ifdef _USEPOOL
#include "globalresources.h"
#endif

/*
 * Manages state of Skin
 * - These state is loaded from Global state.
 */
class SkinOption {
public:
	/*
	 * value to Global Timers
	 */
	typedef struct {
		/*
		 * we don't need to know total selection
		 * as we're only wants to know current selection.
		 * To get total selectable value, refer `Skin.h` structure.
		 */
		std::string optionname;
		std::string switchname;
	} CustomTimer;

	/*
	 * value to Global Ints
	 */
	typedef struct {
		std::string optionname;
		int value;
	} CustomValue;

	/*
	 * value to Global Strings
	 */
	typedef struct {
		std::string optionname;
		std::string path;
	} CustomFile;

private:
	std::vector<CustomTimer> switches;
	std::vector<CustomValue> values;
	std::vector<CustomFile> files;
public:
	bool LoadSkinOption(const char *filepath);
	bool SaveSkinOption(const char *filepath);
	void Clear();
	std::vector<CustomTimer>& GetSwitches();
	std::vector<CustomValue>& GetValues();
	std::vector<CustomFile>& GetFiles();
#ifdef _USEPOOL
	void SetEnvironmentFromOption();
#endif
};