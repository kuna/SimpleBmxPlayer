/*
 * @description
 * SkinRenderTree: creates proper render tree for each object.
 * Renderer-dependent.
 */
#pragma once

#include "image.h"
#include "timer.h"
#include <vector>

namespace {
	enum ACCTYPE {
		ACC_NONE = 0,
		ACC_LINEAR = 1,
		ACC_EASEIN = 2,
		ACC_EASEOUT = 3,
		ACC_EASEINOUT = 4,
	};
	enum OBJECTTYPE {
		NONE = 0,		/* same as unknown */
		GENERAL = 1,	/* handled by basic rendering function */
		GROUP = 2,
		SPECIAL = 3,
		EXTERN = 4,
		UNKNOWN = 5,
	};
}
struct ImageSRC {
	int id;				// id can direct not only image but font.
	int x, y, w, h;
	int divx, divy, cycle;
	void ToRect(SDL_Rect &r, int time);
};
struct ImageDSTFrame {
	int time;
	int x, y, w, h, a, r, g, b, angle;
};
struct ImageDST {
	int blend;
	int loopstart, loopend;
	int rotatecenter;
	std::vector<ImageDSTFrame> frame;
	void AddFrame(ImageDSTFrame &f);
	void ToRect(SDL_Rect &r, int time);
};

#define _MAX_RENDER_CONDITION 10

/** @brief very basic rendering object which does nothing. */
class SkinRenderObject {
private:
	/** @brief type of this object */
	int objtype;
	/** @brief condition for itself */
	RString condition;
	ImageSRC src[_MAX_RENDER_CONDITION];
	RString src_condition[_MAX_RENDER_CONDITION];
	Timer *timer_src[_MAX_RENDER_CONDITION];
	size_t srccnt;
	ImageDST dst[_MAX_RENDER_CONDITION];
	RString dst_condition[_MAX_RENDER_CONDITION];
	Timer *timer_dst[_MAX_RENDER_CONDITION];
	size_t dstcnt;
	void *tag;		// for various use

	bool TestCollsion(int x, int y);
	bool focusable;
	bool clickable;
public:
	SkinRenderObject();
	virtual void Clear();
	virtual void AddSRC(ImageSRC &src, const RString& condition = "");
	virtual void AddDST(ImageDST &dst, const RString& condition = "");
	virtual void Render();

	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Click(int x, int y);
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Hover(int x, int y);
};

class SkinRenderObjectGroup : public SkinRenderObject {
private:
	/** @brief base elements */
	std::vector<SkinRenderObject*> _childs;
public:
	void AddChild(SkinRenderObject* obj);
	std::vector<SkinRenderObject*>::iterator begin();
	std::vector<SkinRenderObject*>::iterator end();
};

class SkinNumberElement : public SkinRenderObject {
private:
	//Font *fnt;
public:
	//virtual void Render();
};

class SkinGraphElement : public SkinRenderObject {
private:
	/* elements needed for drawing */
	int type;
	int direction;
public:
	void SetType(int type);
	void SetDirection(int direction);
	//virtual void Render();
};

class SkinSliderElement : public SkinRenderObject {
private:
	/* elements needed for drawing */
	int value;
	int maxvalue;
	int direction;
	int range;
	//bool editable;	this part is manually controlled in program.
public:
	void SetMaxValue(int maxvalue);
	void SetAlign(int align);
	//virtual void Render();
};

class SkinTextElement : public SkinRenderObject {
private:
	/* elements needed for drawing */
	char text[256];
	int align;
	bool editable;
public:
	void SetEditable(bool editable);
	void SetAlign(int align);
	//virtual void Render();
};

class SkinButtonElement : public SkinRenderObject {
private:
	// only we need more is: handler.
	//_Handler handler;
public:
	//virtual void Render();
};

/** @brief rendering tree which is used in real rendering - based on Xml skin structure */
class SkinRenderTree: public SkinRenderObjectGroup {
public:
	/** @brief stores all skin render objects */
	std::vector<SkinRenderObject*> _objpool;
public:
	~SkinRenderTree();
	void ReleaseAll();

	SkinRenderObject* NewNoneObject();
};