// for game
// minimalized function - this CANNOT read all lr2skin.

#pragma once

#include "image.h"
#include "timer.h"
#include <vector>
#include <map>

/*
 * used when you draw skin
 */
struct ImageSRC {
	int x, y, w, h, div_x, div_y, cycle, timer;
};
struct ImageDST {
	int time;
	int x, y, w, h, acc, a, r, g, b, angle;
};
class SkinRenderData {
public:
	Image *img;
	ImageSRC src;
	ImageDST dst;
public:
	void Render();
};

class SkinElement {
public:
private:
	Image *img;
	std::vector<ImageSRC> src;
	std::vector<ImageDST> dst;
	int blend;
	int center;
	int timer;
	int looptime;
	int usefilter;
public:
	void AddSrc(char **args, Image *imgs);
	void AddDst(char **args);
	void AddSrc(ImageSRC& src);
	void AddDst(ImageDST& dst);

	/*
	 * returns is object have SRC & DST
	 * used when we need to determine whether we add new object or not
	 */
	bool IsValid();

	void GetRenderData(SkinRenderData &renderdata);
};

class SkinOption {
public:
	int skinoption[1000];
public:
	SkinOption();
	void LoadSkinOption(const char* filepath);
	void SetOption(int idx, int val);
	int GetOption(int idx);
};

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

private:
	// for parsing
	bool parser_condition[100];	// support 100 nested (current parsing context)
	int parser_level;			// current level
	void MakeElementFromLr2Skin(char **args, SkinOption &skinoption);
	void LoadResource();
public:
	Skin();
	~Skin();
	bool Parse(const char *filepath, SkinOption &skinoption);
	void Release();

	// convenience function
	bool GetSrcNote(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteStart(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteBody(Image* img, SDL_Rect *r);
	bool GetSrcLongNoteEnd(Image* img, SDL_Rect *r);

	std::vector<SkinElement>::iterator begin();
	std::vector<SkinElement>::iterator end();
};
