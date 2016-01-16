/*
 * @description
 * SkinRenderTree: creates proper render tree for each object.
 * Renderer-dependent.
 */
#pragma once

#include "image.h"
#include "timer.h"
#include "skin.h"
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
		EXTERN = 3,
		UNKNOWN = 4,
		BASE = 5,
		IMAGE = 10,
		NUMBER = 11,
		GRAPH = 12,
		SLIDER = 13,
		TEXT = 14,
		BUTTON = 15,
		/* some renderer specific objects ... */
		BGA = 20,
	};
}
struct ImageSRC {
	RString resid;				// id can direct not only image but font.
	int x, y, w, h;
	int divx, divy, cycle;
};
struct ImageDSTFrame {
	int time;
	int x, y, w, h, a, r, g, b, angle;
};
struct ImageDST {
	Timer *timer;
	int blend;
	int loopstart, loopend;
	int rotatecenter;
	int acctype;
	std::vector<ImageDSTFrame> frame;
};

#define _MAX_RENDER_CONDITION 10

/** @brief a simple condition evaluator for fast performance 
 * supporting format: divided by comma (only AND operator, maximum 10)
 * ex) condition="OnGameStart,On1PKey7Up"
 */
class RenderCondition {
private:
	RString luacondition;
	RString key[10];
	bool not[10];
	Timer *cond[10];
	int condcnt;
public:
	RenderCondition();
	void Set(const RString &conditionstring);
	void SetLuacondition(const RString &luacond);
	bool Evaluate();
};

/* redeclaration */
class SkinRenderTree;
class SkinUnknownObject;
class SkinGroupObject;
class SkinImageObject;

/** @brief very basic rendering object which does nothing. */
class SkinRenderObject {
protected:
	/** @brief type of this object */
	int objtype;
	/** @brief condition for itself */
	RenderCondition condition;
	ImageSRC src[_MAX_RENDER_CONDITION];
	RenderCondition src_condition[_MAX_RENDER_CONDITION];
	Timer *timer_src[_MAX_RENDER_CONDITION];
	size_t srccnt;
	ImageDST dst[_MAX_RENDER_CONDITION];
	RenderCondition dst_condition[_MAX_RENDER_CONDITION];
	Timer *timer_dst[_MAX_RENDER_CONDITION];
	size_t dstcnt;
	void *tag;		// for various use

	bool TestCollsion(int x, int y);
	bool focusable;
	bool clickable;
	SkinRenderTree* rtree;
public:
	SkinRenderObject(SkinRenderTree* owner, int type = NONE);
	virtual void Clear();
	virtual void SetCondition(const RString &str);
	virtual void AddSRC(ImageSRC &src, const RString& condition = "", bool lua = false);
	virtual void AddDST(ImageDST &dst, const RString& condition = "", bool lua = false);
	virtual void Render();

	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Click(int x, int y);
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Hover(int x, int y);

	bool IsGroup();
	bool IsGeneral();
	SkinUnknownObject* ToUnknown();
	SkinGroupObject* ToGroup();
	SkinImageObject* ToImage();
};

class SkinUnknownObject : public SkinRenderObject {
private:
public:
	SkinUnknownObject(SkinRenderTree* owner);
};

class SkinGroupObject : public SkinRenderObject {
private:
	/** @brief base elements */
	std::vector<SkinRenderObject*> _childs;
public:
	SkinGroupObject(SkinRenderTree* owner);
	void AddChild(SkinRenderObject* obj);
	std::vector<SkinRenderObject*>::iterator begin();
	std::vector<SkinRenderObject*>::iterator end();
};

class SkinImageObject : public SkinRenderObject {
private:
	Image *img[_MAX_RENDER_CONDITION];
public:
	SkinImageObject(SkinRenderTree* owner);
	virtual void Render();
};

class SkinNumberObject : public SkinRenderObject {
private:
	//Font *fnt;
public:
	SkinNumberObject(SkinRenderTree* owner);
	//virtual void Render();
};

class SkinGraphObject : public SkinRenderObject {
private:
	/* elements needed for drawing */
	int type;
	int direction;
public:
	SkinGraphObject(SkinRenderTree* owner);
	void SetType(int type);
	void SetDirection(int direction);
	//virtual void Render();
};

class SkinSliderObject : public SkinRenderObject {
private:
	/* elements needed for drawing */
	int value;
	int maxvalue;
	int direction;
	int range;
	//bool editable;	this part is manually controlled in program.
public:
	SkinSliderObject(SkinRenderTree* owner);
	void SetMaxValue(int maxvalue);
	void SetAlign(int align);
	//virtual void Render();
};

class SkinTextObject : public SkinRenderObject {
private:
	/* elements needed for drawing */
	char text[256];
	int align;
	bool editable;
public:
	SkinTextObject(SkinRenderTree* owner);
	void SetEditable(bool editable);
	void SetAlign(int align);
	//virtual void Render();
};

class SkinButtonObject : public SkinRenderObject {
private:
	// only we need more is: handler.
	//_Handler handler;
public:
	SkinButtonObject(SkinRenderTree* owner);
	//virtual void Render();
};

/** @brief rendering tree which is used in real rendering - based on Xml skin structure */
class SkinRenderTree: public SkinGroupObject {
public:
	/** @brief stores all skin render objects */
	std::vector<SkinRenderObject*> _objpool;

	/** @brief resources used in this game */
	std::map<RString, Image*> _imagekey;
public:
	SkinRenderTree();
	~SkinRenderTree();
	/** @brief only releases all rendering object */
	void ReleaseAll();

	SkinUnknownObject* NewUnknownObject();
	SkinGroupObject* NewGroupObject();
	SkinImageObject* NewImageObject();

	/** @brief load image at globalresources. */
	void RegisterImage(RString& id, RString& path);
	/** @brief release all resources */
	void ReleaseAllResources();
};

namespace SkinRenderHelper {
	void AddFrame(ImageDST &d, ImageDSTFrame &f);
	SDL_Rect ToRect(ImageSRC &r, int time);
	SDL_Rect ToRect(ImageDSTFrame &r);
	bool CalculateFrame(ImageDST &dst, ImageDSTFrame &frame);
	ImageDSTFrame Tween(ImageDSTFrame& a, ImageDSTFrame &b, double t, int acctype);
	/** @brief in debug mode, border will drawn around object. */
	void Render(Image *img, ImageSRC *src, ImageDSTFrame *frame);

	/** @brief replaces path string to a correct one */
	void ConvertPath(RString& path);
	/** @brief constructs resid-object mapping and loads resources from XmlSkin. */
	bool LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s);
	/** @brief constructs rendertree object from XmlSkin. must LoadResource first. */
	bool ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s);
}