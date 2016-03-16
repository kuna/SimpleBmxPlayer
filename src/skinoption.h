#pragma once

#include <vector>
#define _USEPOOL		// you can use SkinOption generally by disabling this definition

/*
 * Manages state of Skin
 * - These state is loaded from Global state.
 */
class SkinOption {
public:
	/*
	 * value to Global Ints
	 */
	typedef struct {
		int value;				// value of <optionname> metrics.
		std::string desc;		// general desc for displaying
		std::string eventname;	// triggers event if it's necessary.
	} Option;
	typedef struct {
		std::string optionname;
		std::string desc;
		int optionidx;
		std::vector<Option> options;
	} CustomValue;

	/*
	 * value to Global Strings
	 */
	typedef struct {
		std::string optionname;
		std::string desc;
		std::string path;
	} CustomFile;

private:
	std::vector<CustomValue> values;
	std::vector<CustomFile> files;
public:
	bool LoadSkinOption(const char *filepath);
	bool SaveSkinOption(const char *filepath);
	void Clear();
	std::vector<CustomValue>& GetValues();
	std::vector<CustomFile>& GetFiles();
#ifdef _USEPOOL
	void SetEnvironmentFromOption();
	void GetEnvironmentFromOption();
	void DeleteEnvironmentFromOption();
#endif
};
