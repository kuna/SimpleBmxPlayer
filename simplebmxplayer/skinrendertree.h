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
#include "tinyxml2.h"
using namespace tinyxml2;

namespace OBJTYPE {
	enum OBJTYPE {
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
		LIST = 16,
		/* some renderer specific objects ... */
		BGA = 20,
		PLAYLANE = 21,
	};
}
namespace ROTATIONCENTER {
	enum ROTATIONCENTER {
		TOPLEFT = 7,
		TOPCENTER = 8,
		TOPRIGHT = 9,
		CENTERLEFT = 4,
		CENTER = 5,
		CENTERRIGHT = 6,
		BOTTOMLEFT = 1,
		BOTTOMCENTER = 2,
		BOTTOMRIGHT = 3,
	};
}
namespace ACCTYPE {
	enum ACCTYPE {
		LINEAR = 0,
		ACCEL = 1,
		DECEL = 2,
		NONE = 3,
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
class SkinBgaObject;

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
	SkinRenderObject(SkinRenderTree* owner, int type = OBJTYPE::NONE);
	virtual void Clear();
	virtual void SetCondition(const RString &str);
	virtual void AddSRC(ImageSRC &src, const RString& condition = "", bool lua = false);
	virtual void AddDST(ImageDST &dst, const RString& condition = "", bool lua = false);
	virtual void Render();
	bool EvaluateCondition();

	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Click(int x, int y);
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Hover(int x, int y);

	bool IsGroup();
	bool IsGeneral();
	SkinUnknownObject* ToUnknown();
	SkinGroupObject* ToGroup();
	SkinImageObject* ToImage();
	SkinBgaObject* ToBGA();
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

/** @description
 * a very basic list object
 *
 * Generally, This object cannot be used only itself;
 * It should implemented in some other object.
 * This object just supplies position for listitem.
 *
 * It doesn't render something itself, and it doesn't parses child object.
 * May better to extend this class to draw something you want.
 * Use it on your own.
 */
class SkinListObject: public SkinRenderObject {
private:
	Uint32				listsize;
	double				listscroll;
	Uint32				x, y, w, h;
	Uint32				select_dx, select_dy;
public:
	void RenderListItem(int idx, SkinRenderObject *obj);
};

/** @brief specific object used during play */
class SkinPlayObject : public SkinRenderObject {
private:
	struct NOTE {
		ImageSRC normal;
		ImageSRC ln_end;
		ImageSRC ln_body;
		ImageSRC ln_start;
		ImageSRC mine;
		ImageDST dst;
		Image* img;			// COMMENT: do we need to have multiple image?
	};
	SkinImageObject *imgobj_judgeline;
	SkinImageObject *imgobj_line;
public:
	NOTE		Note[20];
	NOTE		AutoNote[20];
	Uint32		x, y, w, h;
public:
	SkinPlayObject(SkinRenderTree* owner);
	void ConstructLane(XMLElement *laneobj);
	void SetJudgelineObject(XMLElement *judgelineobj);
	void SetLineObject(XMLElement *lineobj);
	Uint32 GetLaneHeight();
	/** @brief check is this object is suitable for drawing lane. use for performance optimization */
	bool IsLaneExists(int laneindex);
	/** @brief `pos = 1` means note on the top of the lane */
	void RenderLane(int laneindex, double pos, bool mine = false);
	/** @brief for longnote. */
	void RenderLane(int laneindex, double pos_start, double pos_end);
};

/** @brief do nothing; just catch this object if you want to draw BGA. */
class SkinBgaObject : public SkinRenderObject {
public:
	SkinBgaObject(SkinRenderTree *);
	void RenderBGA(Image *bga);
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
	SkinPlayObject* NewPlayObject();
	SkinBgaObject* NewBgaObject();

	/** @brief load image at globalresources. */
	void RegisterImage(RString& id, RString& path);
	/** @brief release all resources */
	void ReleaseAllResources();
};

namespace SkinRenderHelper {
	void AddFrame(ImageDST &d, ImageDSTFrame &f);
	void ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e);
	SDL_Rect ToRect(ImageSRC &r, int time, int image_width = 0, int image_height = 0);
	SDL_Rect ToRect(ImageDSTFrame &r);
	bool CalculateFrame(ImageDST &dst, ImageDSTFrame &frame);
	ImageDSTFrame Tween(ImageDSTFrame& a, ImageDSTFrame &b, double t, int acctype);
	/** @brief in debug mode, border will drawn around object. */
	void Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend = 1, int rotationcenter = ROTATIONCENTER::CENTER);

	/** @brief replaces path string to a correct one */
	void ConvertPath(RString& path);
	/** @brief constructs resid-object mapping and loads resources from XmlSkin. */
	bool LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s);
	/** @brief constructs rendertree object from XmlSkin. must LoadResource first. */
	bool ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s);
}