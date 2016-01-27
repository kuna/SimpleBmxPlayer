#include "font.h"
#include <vector>
#include "tinyxml2.h"
using namespace tinyxml2;

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

#define _MAX_RENDER_CONDITION	20
#define _MAX_RENDER_DST			60

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
class SkinNumberObject;
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
	SkinRenderObject(SkinRenderTree* owner, int type = ACTORTYPE::NONE);
	int GetType();
	virtual void Clear();
	virtual void SetObject(XMLElement *e);
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
	SkinImageObject(SkinRenderTree* owner, int type = ACTORTYPE::IMAGE);
	void SetSRC(XMLElement *e);
	void SetSRC(const ImageSRC& imgsrc);
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
	void SetFont(const char* resid);
	void SetEditable(bool editable);
	void SetAlign(int align);
	void SetValue(RString* s);
	virtual void Render();
	virtual int GetWidth();
	int GetTextWidth(const RString& s);
	void RenderText(const char* s);
};

class SkinNumberObject : public SkinTextObject {
private:
	char buf[20];
	bool mode24;
	int length;
	int *v;
public:
	SkinNumberObject(SkinRenderTree* owner);
	void SetValue(int *i);
	void SetLength(int length);
	void Set24Mode(bool b);
	virtual void Render();
	virtual int GetWidth();
	/** @brief make string for rendering and call SkinTextObject::RenderText() */
	void CacheInt(int n);
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
class SkinListObject : public SkinRenderObject {
private:
	Uint32				listsize;
	double				listscroll;
	Uint32				x, y, w, h;
	Uint32				select_dx, select_dy;
public:
	void RenderListItem(int idx, SkinRenderObject *obj);
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