/*
 * @description
 * SkinRenderTree: creates proper render tree for each object.
 * Renderer-dependent.
 */
#pragma once

#include "image.h"
#include "timer.h"
#include "font.h"
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
		SCRIPT = 17,
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
	Timer *timer;
	int x, y, w, h;
	int divx, divy, cycle;
	int loop;
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

#define _MAX_RENDER_CONDITION 20

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
	int evaluate_count;
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
class SkinSliderObject;
class SkinGraphObject;
class SkinTextObject;
class SkinButtonObject;
class SkinBgaObject;
class SkinPlayObject;

/** @brief very basic rendering object which does nothing. */
class SkinRenderObject {
protected:
	/** @brief type of this object */
	int objtype;
	/** @brief condition for itself */
	RenderCondition condition;
	ImageDST dst[_MAX_RENDER_CONDITION];
	RenderCondition dst_condition[_MAX_RENDER_CONDITION];
	size_t dstcnt;
	/** @brief this value is filled when you call Update() method. */
	struct ImageDSTCached {
		ImageDST *dst;
		ImageDSTFrame frame;
	} dst_cached;
	/** @brief different from visible. decided by Renderer(Update() method) */
	bool drawable;

	void *tag;		// for various use

	bool TestCollsion(int x, int y);
	bool focusable;
	bool clickable;
	bool invertcondition;

	SkinRenderTree* rtree;
public:
	SkinRenderObject(SkinRenderTree* owner, int type = OBJTYPE::NONE);
	virtual void Clear();
	virtual void SetCondition(const RString &str);
	virtual void AddDST(ImageDST &dst, const RString& condition = "", bool lua = false);
	bool EvaluateCondition();
	/** @brief should we have to invert condition? maybe useful for IFNOT clause. */
	void InvertCondition(bool b);

	/* @brief must call before update object or do something */
	virtual void Update();
	/* @brief draw object to screen with it's own basic style */
	virtual void Render();
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Click(int x, int y);
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Hover(int x, int y);
	virtual int GetWidth();
	virtual int GetHeight();
	virtual int GetX();
	virtual int GetY();

	bool IsGroup();
	bool IsGeneral();

	SkinUnknownObject* ToUnknown();
	SkinGroupObject* ToGroup();
	SkinImageObject* ToImage();
	SkinBgaObject* ToBGA();
	SkinPlayObject* ToPlayObject();
};

class SkinUnknownObject : public SkinRenderObject {
private:
public:
	SkinUnknownObject(SkinRenderTree* owner);
};

class SkinImageObject : public SkinRenderObject {
protected:
	ImageSRC imgsrc;
	Image *img;
public:
	SkinImageObject(SkinRenderTree* owner, int type = OBJTYPE::IMAGE);
	void SetSRC(XMLElement *e);
	void SetImage(Image *img);
	virtual void Render();
};

class SkinGroupObject : public SkinImageObject {
private:
	/** @brief base elements */
	std::vector<SkinRenderObject*> _childs;
	/** @brief canvas. if 0, then only pushs offset for drawing childs. */
	SDL_Texture *t, *_org;
public:
	SkinGroupObject(SkinRenderTree* owner, bool createTexture = false);
	~SkinGroupObject();
	void AddChild(SkinRenderObject* obj);
	std::vector<SkinRenderObject*>::iterator begin();
	std::vector<SkinRenderObject*>::iterator end();
	virtual void Render();
	/** @brief reset coordination from group object */
	void SetAsRenderTarget();
	/** @brief reset coordination from group object */
	void ResetRenderTarget();
};

class SkinTextObject : public SkinRenderObject {
private:
	/* elements needed for drawing */
	int align;
	bool editable;
	Font *fnt;
	RString *v;
public:
	SkinTextObject(SkinRenderTree* owner);
	void SetFont(Font* f);
	void SetEditable(bool editable);
	void SetAlign(int align);
	void SetValue(RString* s);
	virtual void Render();
	void RenderText(const char* s);
};

class SkinNumberObject : public SkinTextObject {
private:
	int *v;
public:
	SkinNumberObject(SkinRenderTree* owner);
	void SetValue(int *i);
	virtual void Render();
	void RenderInt(int n);
};

class SkinGraphObject : public SkinImageObject {
private:
	/* elements needed for drawing */
	int type;
	int direction;
	double *v;
public:
	SkinGraphObject(SkinRenderTree* owner);
	void SetType(int type);
	void SetDirection(int direction);
	void SetValue(double* pv);
	virtual void Render();
	// TODO
	void EditValue(int dx, int dy);
};

class SkinSliderObject : public SkinImageObject {
private:
	/* elements needed for drawing */
	int direction;
	int range;
	double *v;
	//bool editable;	this part is manually controlled in program...?
public:
	SkinSliderObject(SkinRenderTree* owner);
	void SetDirection(int direction);
	void SetRange(int range);
	void SetValue(double* pv);
	virtual void Render();
	// TODO
	void EditValue(int dx, int dy);
};

class SkinButtonObject : public SkinImageObject {
private:
	ImageSRC hover;
	Image* img_hover;
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
class SkinPlayObject : public SkinGroupObject {
private:
	struct NOTE {
		ImageSRC normal;
		ImageSRC ln_end;
		ImageSRC ln_body;
		ImageSRC ln_start;
		ImageSRC mine;
		ImageDSTFrame f;
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
	void RenderNote(int laneindex, double pos, bool mine = false);
	/** @brief for longnote. */
	void RenderNote(int laneindex, double pos_start, double pos_end);
};

/** @brief do nothing; just catch this object if you want to draw BGA. */
class SkinBgaObject : public SkinRenderObject {
public:
	SkinBgaObject(SkinRenderTree *);
	void RenderBGA(Image *bga);
};

/** @brief it actually doesn't renders anything, but able to work with Render() method. */
class SkinScriptObject : public SkinRenderObject {
	bool runoneveryframe;
	int runtime;
	RString script;
public:
	SkinScriptObject(SkinRenderTree *);
	void SetRunCondition(bool oneveryframe = false);
	void LoadFile(const RString &filepath);
	void SetScript(const RString &script);
	virtual void Render();
};

/** @brief rendering tree which is used in real rendering - based on Xml skin structure */
class SkinRenderTree: public SkinGroupObject {
public:
	/** @brief stores all skin render objects */
	std::vector<SkinRenderObject*> _objpool;

	/** @brief resources used in this game */
	std::map<RString, Image*> _imagekey;
	/** @brief resources used in this game */
	std::map<RString, Font*> _fontkey;

	/** @brief decide group object's texture size */
	int _scr_w, _scr_h;

	/** @brief stores element ID-pointer. use GetElementById() to use this array. */
	std::map<RString, SkinRenderObject*> _idpool;
public:
	SkinRenderTree(int skinwidth, int skinheight);
	~SkinRenderTree();
	void SetSkinSize(int skinwidth, int skinheight);
	/** @brief only releases all rendering object */
	void ReleaseAll();
	int GetWidth();
	int GetHeight();
	SDL_Texture* GenerateTexture();
	SkinRenderObject* GetElementById(RString &id);

	SkinUnknownObject* NewUnknownObject();
	SkinGroupObject* NewGroupObject(bool clipping = false);
	SkinImageObject* NewImageObject();
	SkinSliderObject* NewSliderObject();
	SkinGraphObject* NewGraphObject();
	SkinTextObject* NewTextObject();
	SkinNumberObject* NewNumberObject();
	SkinBgaObject* NewBgaObject();
	SkinScriptObject* NewScriptObject();

	//SkinLifeGraphObject* NewLifeGraphObject();
	SkinPlayObject* NewPlayObject();

	/** @brief load image at globalresources. */
	void RegisterImage(RString& id, RString& path);
	/** @brief load font at globalresources. */
	void RegisterTTFFont(RString& id, RString& path, int size);
	/** @brief load font at globalresources. (using texturefont raw data) */
	void RegisterTextureFont(RString& id, RString& path);
	/** @brief load font at globalresources. (using texturefont raw data) */
	void RegisterTextureFontByData(RString& id, RString& textdata);
	/** @brief release all resources */
	void ReleaseAllResources();
};

namespace SkinRenderHelper {
	void AddFrame(ImageDST &d, ImageDSTFrame &f);
	void ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e);
	SDL_Rect ToRect(ImageSRC &r);
	SDL_Rect ToRect(ImageDSTFrame &r);
	bool CalculateFrame(ImageDST &dst, ImageDSTFrame &frame);
	ImageDSTFrame Tween(ImageDSTFrame& a, ImageDSTFrame &b, double t, int acctype);
	/** @brief in debug mode, border will drawn around object. */
	void Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend = 1, int rotationcenter = ROTATIONCENTER::CENTER);

	void PushRenderOffset(int x, int y);
	void PopRenderOffset();

	/** @brief replaces path string to a correct one */
	void ConvertPath(RString& path);
	/** @brief constructs resid-object mapping and loads resources from XmlSkin. */
	bool LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s);
	/** @brief constructs rendertree object from XmlSkin. must LoadResource first. */
	bool ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s);
}