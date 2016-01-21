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
namespace {
	enum ACCTYPE {
		ACC_NONE = 0,
		ACC_LINEAR = 1,
		ACC_EASEIN = 2,
		ACC_EASEOUT = 3,
		ACC_EASEINOUT = 4,
	};
}
struct ImageSRC {
	int x, y, w, h;
	void ToRect(SDL_Rect &r);
};
struct ImageDST {
	int time;
	int x, y, w, h, acc, a, r, g, b, angle, rotatecenter;
	void ToRect(SDL_Rect &r);
};
struct SkinDebugInfo {
	Uint32 line;
	Uint32 errorcode;
};

/*
 * This element has all options about how it should draw
 * Value is supplied from program

 * TODO: src can be changed from condition so we need to look carefully
 * TODO: slider/number/text all has different option, how we can integrate them?

 */
class SkinElementGroup;
class SkinElement {
public:
	/*
	 * linked image
	 * DO NOT RELEASE THIS OBJECT YOURSELF!!
	 * let Skin object release them itself.
	 */
	Image *img;

	/*
	 * src keyframe
	 * (also can be animated; which is not going to be used in most case)
	 */
	ImageSRC src;
	int divx, divy, cycle;

	/*
	 * animation keyframes used when show/hide
	 * (if dst alpha==0, then it's automatically hide)
	 */
	std::vector<ImageDST> dst;

	/*
	 * common dst option
	 */
	int blend;
	int looptime_dst;
	int rotatecenter;

	/*
	 * timers
	 */
	Timer timer_src;	// SRC uses basic timer, so we don't need this.
	Timer timer_dst;
	bool isShowing;

	/*
	 * child/parent/metadata
	 */
	SkinElement *parent;
	SkinElementGroup child;
	std::string id;
	std::vector<std::string> classnames;
	std::string tagname;

	/*
	 * all skin elements should have its own value to act interactive
	 * (ex: you can change NUMBER object into SLIDER with no side effect)
	 * Q: how we could know it is slider or button or something else ...?
	 * -> that's not important, we only need to use virtual draw()/set() method, not something else.
	 */
	std::string value_str;
	int value_num;

	SkinDebugInfo _debuginfo;	// contains debugging info (like line number or object index)
protected:
	// an inner implemented method used when drawing sprite
	void Draw(SDL_Renderer *renderer, ImageSRC &src, ImageDST &dst);
public:
	/*
	 * only used for constructing SkinElement()
	 */
	SkinElement(SkinDebugInfo &debuginfo);
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
	std::vector<ImageSRC>& GetSrcArray();

	/*
	 * General commands - Show/Hide/Draw/Set
	 */
	void SetTag(const std::string& tagname);
	void AddClassName(const std::string& classname);
	void SetID(const std::string& id);
	void Show();
	void Hide();						// reverse time (current time -> 0)
	void Set(int value);				// string have no effect in this case
	void Set(const std::string& value);	// int will have no effect in this case
	virtual void Draw(SDL_Renderer *renderer);
};

/*
* Under these things are just for convinence
* These classes provides convient ways to print some general objects easily.
*/

class SkinNumberElement : public SkinElement {
private:
	/* elements needed for drawing */
	int num;
	int length;				// number's length
	int align;				// align

	ImageSRC src_base;		// used when showing blank number
	ImageSRC src_plus, src_minus;
	ImageSRC src_dot;

	/* src is accessed like this:
	 * if src size is 10, then use only plus one
	 * if src size is 20, then use red in minus
	 */
public:
	SkinNumberElement(SkinDebugInfo &info);
	virtual void Draw(SDL_Renderer *renderer);
};

class SkinGraphElement : public SkinElement {
private:
	/* elements needed for drawing */
	float num;
	int type;
	int direction;
public:
	SkinGraphElement(SkinDebugInfo &info);
	void SetType(int type);
	void SetDirection(int direction);
	virtual void Draw(SDL_Renderer *renderer);
};

class SkinSliderElement : public SkinElement {
private:
	/* elements needed for drawing */
	int value;
	int maxvalue;
	int direction;
	int range;
	//bool editable;	this part is manually controlled in program.
public:
	SkinSliderElement(SkinDebugInfo &info);
	void SetMaxValue(int maxvalue);
	void SetAlign(int align);
	virtual void Draw(SDL_Renderer *renderer);
};

class SkinTextElement : public SkinElement {
private:
	/* elements needed for drawing */
	char text[256];
	int align;
	bool editable;
public:
	SkinTextElement(SkinDebugInfo &info);
	void SetEditable(bool editable);
	void SetAlign(int align);
	virtual void Draw(SDL_Renderer *renderer);
};

class SkinButtonElement : public SkinElement {
private:
	/* elements needed for drawing */
	bool isclicked;
public:
	SkinButtonElement(SkinDebugInfo &info);
	bool CheckClicked();
	virtual void Draw(SDL_Renderer *renderer);
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

/*
 * You can easily control groups
 * by class SkinElementGroup()
 */
class SkinElementGroup {
private:
	std::vector<SkinElement*> elements;
public:
	SkinElementGroup();
	SkinElementGroup(SkinElement*);

	std::vector<SkinElement*>& GetAllElements();
	int GetElements(const std::string& id, SkinElementGroup *g);		// g == 0 then check existence
	int GetElements(const std::string& classname, SkinElementGroup *g);	// g == 0 then check existence
	void AddElement(SkinElement* e);
	void RemoveClass(const std::string& classname);						// remove element which has that classname
	void SelectClass(const std::string& classname);
	void RemoveTag(const std::string& tag);
	void SelectTag(const std::string& tag);

	// Get just one element ( last element)
	SkinElement* GetOne();

	void Show();
	void Hide();

	void Clear();
};