// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "image.h"
#include "timer.h"
#include "skinelement.h"
#include <vector>
#include <map>

/*
 * Skin's inner switch (DstOption)
 * It's global accessible
 */
namespace SkinDST {
	extern bool dst[10000];
	void On(int idx);
	void Off(int idx);
	bool Toggle(int idx);
	bool Get(int idx);
}

class Skin {
private:
	// initalized?
	char filepath[256];

	// skin headers
	// (store data about resources, etc ...)
	std::map<std::string, std::vector<std::string>> headers;

	// skin resources
	// (currently only supports Image type)
	Image imgs[50];

public:
	// skin elements
	std::vector<SkinElement> elements;
	SkinElement note[20];
	SkinElement lnstart[20];
	SkinElement lnbody[20];
	SkinElement lnend[20];
	ImageDST bga;

private:
	// for parsing
	int parser_condition[100];	// support 100 nested (current parsing context)
	int parser_level;			// current level
	void MakeElementFromLr2Skin(char **args);
	void LoadResource();
public:
	Skin();
	~Skin();
	bool Parse(const char *filepath);
	void Release();

	// convenience function
	bool GetSrcNote(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteStart(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteBody(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteEnd(Image* img, SDL_Rect *r);

	std::vector<SkinElement>::iterator begin();
	std::vector<SkinElement>::iterator end();
};
