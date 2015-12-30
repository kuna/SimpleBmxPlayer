/*
 * class SkinNumber
 * - draw numbers (created from SkinElement)
 */
#pragma once

#include "image.h"
#include <vector>

/*
* used when you draw skin
*/
struct ImageSRC {
	int x, y, w, h, div_x, div_y, cycle, timer;
	void ToRect(SDL_Rect &r);
};
struct ImageDST {
	int time;
	int x, y, w, h, acc, a, r, g, b, angle;
	void ToRect(SDL_Rect &r);
};
class SkinRenderData {
public:
	Image *img;
	ImageSRC src;
	ImageDST dst;
	int blend;
public:
	void Render();
};

class SkinElement {
protected:
	Image *img;
	std::vector<ImageSRC> src;
	std::vector<ImageDST> dst_show;
	std::vector<ImageDST> dst_hide;
	int blend;
	int center;
	int timer;
	int looptime;
	int usefilter;
	int option[5];
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

	bool CheckOption();
	void GetRenderData(SkinRenderData &renderdata);
	void CalculateSrc(ImageSRC &src, ImageDST &dst, Uint32 tick);
	Image* GetPtr();

	virtual void Draw(SDL_Renderer *renderer) = 0;
};

class SkinNumberElement: public SkinElement {
private:

public:
	SkinNumberElement(SkinElement &e);
	virtual void Draw(SDL_Renderer *renderer);	// It automatically gets number from DST/Num/etc ...
};

class SkinComboElement : public SkinElement {
public:
	SkinComboElement(SkinElement &e);
	void SetCombo(int judge, int combo);
	virtual void Draw(SDL_Renderer *renderer);
};