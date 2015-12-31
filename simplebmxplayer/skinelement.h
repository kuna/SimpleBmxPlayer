/*
 * class SkinNumber
 * - draw numbers (created from SkinElement)
 */
#pragma once

#include "image.h"
#include "timer.h"
#include <vector>

/*
* used for SkinElement
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
struct SkinDebugInfo {
	Uint32 line;
	Uint32 errorcode;
};

class SkinElement {
protected:
	/*
	 * linked image path
	 * DO NOT RELEASE THIS OBJECT YOURSELF!!
	 * let Skin object release them itself.
	 */
	Image *img;

	/*
	 * src keyframe
	 * (also can be animated; which is not going to be used in most case)
	 */
	std::vector<ImageSRC> src;

	/*
	 * animation keyframes used when show/hide
	 */
	std::vector<ImageDST> dst_show;
	std::vector<ImageDST> dst_hide;

	/*
	 * common dst option
	 */
	int blend;
	int center;
	int timer;
	int looptime;
	int usefilter;
	int option[5];

	/*
	 * timers
	 */
	Timer timer_src;
	Timer timer_dst;
	bool isShowing;

	/*
	 * child/parent
	 */
	SkinElement *parent;
	std::vector<SkinElement*> child;

	SkinDebugInfo _debuginfo;	// contains debugging info
public:
	/*
	 * only used for constructing SkinElement()
	 */
	SkinElement(bool isShowing, SkinDebugInfo &debuginfo);
	void AddSrc(char **args, Image *imgs);
	void AddDst(char **args);
	void AddSrc(ImageSRC& src);
	void AddDst(ImageDST& dst);

	/*
	 * Most function call Tick() method automatically when they needed
	 * So don't worry. (like CalculateSrc())
	 */
	void Tick();
	/*
	 * You can get Image & src & dst data from this -
	 */
	void CalculateSrc(ImageSRC &src, ImageDST &dst);
	Image* GetPtr();

	/*
	 * You may need to get value of SRC/DST directly in some cases.
	 */
	std::vector<ImageDST>& GetDstArray();
	std::vector<ImageDST>& GetSrcArray();

	/*
	 * General commands - call Show/Hide/Draw.
	 */
	void Show();
	void Hide();
};

/*
 * Under these things are just for convinence
 * These classes provides convient ways to print some general objects easily.
 */

class SkinNumberElement: public SkinElement {
private:
	/* elements needed for drawing */
	float num;
public:
	SkinNumberElement(SkinElement &e);
	void Set(float _num);
};

/*
 * This class is a containers; contains 2 SkinElement
 * Don't inherit
 */
class SkinComboElement {
private:
	SkinElement combosprite;
	SkinNumberElement combonumber;
public:
	SkinComboElement(SkinElement &combosprite, SkinNumberElement &combonumber);
	void Show();
	void Hide();
};