#pragma once

#include "global.h"
#include "font.h"
#include <vector>
#include "tinyxml2.h"
using namespace tinyxml2;

// Texture pos
struct ImageSRC {
	Timer *timer;
	int x, y, w, h;
	int divx, divy, cycle;
	int loop;

	ImageSRC();
	void SetSRCFromXNode(tinyxml2::XMLElement *e);
};

// State that tweens between each state
struct TweenState {
	Display::Rect dst;
	Display::Color color;
	Display::Rotate rotate;
	Display::PointF tilt;
	TweenState();
	TweenState(const TweenState &t);
};
// static information about current tweening
struct TweenInfo {
	TweenState state;
	int blend, acc, center, loop;
};
// Compiled tween state (by lua script or some ...)
struct ImageDST {
	Timer timer;
	std::map<int, TweenInfo>::iterator tween_cur;
	std::map<int, TweenInfo>::iterator tween_next;
	std::map<int, TweenInfo> tweens;
	void SetDSTFromCommand(const RString& cmd);
	ImageDST();
	// TODO
	//void SetDSTFromLua(const RString& lua);
	//void SetDSTFromLua(LuaFunction *luafunc);
};

/*
 * @brief a simple condition evaluator for fast performance
 * supporting format: divided by comma (only AND operator, maximum 10)
 * ex) condition="OnGameStart,On1PKey7Up"
 * COMMENT: depreciated, only for compatibility with LR2 style.
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
class ActorFrame;
class SkinBgaObject;
class SkinNoteFieldObject;

/** @brief very basic rendering object which does nothing. */
class Actor {
protected:
	/** @brief type of this object */
	int objtype;
	/** @brief handlers & functions when handler called */
	//std::map<RString, LuaFunction*> handlers;
	class ActorHandler : public HandlerAuto {
		virtual void Receive(const Message& msg) {
			// TODO
		}
	};
	ActorHandler handler;
	/** @brief current DST (compiled when proper handler called) */
	ImageSRC m_Src;					// Render From
	ImageDST m_Dst;					// Render To
	TweenInfo m_Tweeninfo;			// Current, calculated tween(rendering) state
	RenderCondition m_Condition;	// Is object currently drawable?
	/** @brief different from visible(condition). decided by Renderer(Update() method) */
	bool drawable;

	/** @brief used to check mouse collision event */
	bool TestCollsion(int x, int y);
	bool focusable;
	bool clickable;

	// TODO: we need this? (to get skin width/height)
	//SkinRenderTree* rtree;
public:
	Actor(SkinRenderTree* owner, int type = ACTORTYPE::NONE);
	int GetType();

	/** @brief Clear all attribute of actor */
	virtual void Clear();
	virtual void LoadFromXML(XMLElement *e);
	virtual void SetCondition(const RString &str);

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

	ActorFrame* ToGroup();
	SkinNoteFieldObject* ToNoteFieldObject();
};

class SkinUnknownObject : public Actor {
private:
public:
	SkinUnknownObject(SkinRenderTree* owner);
};

class SkinImageObject : public Actor {
protected:
	ImageSRC imgsrc;
	Display::Texture tex;
public:
	SkinImageObject(SkinRenderTree* owner, int type = ACTORTYPE::IMAGE);
	void SetSRC(const ImageSRC& imgsrc);
	void SetImage(Display::Texture* tex);

	void SetImageObject(XMLElement *e);
	virtual void SetObject(XMLElement *e);
	virtual void Update();
	virtual void Render();
};

class ActorFrame : public Actor {
protected:
	/** @brief base elements */
	std::vector<Actor*> _childs;
public:
	ActorFrame(SkinRenderTree* owner);
	~ActorFrame();
	void AddChild(Actor* obj);
	std::vector<Actor*>::iterator begin();
	std::vector<Actor*>::iterator end();
	void UpdateChilds();

	// CAUTION: this doesn't parses recursively
	virtual void SetObject(XMLElement *e);
	virtual void Update();
	virtual void Render();
};

class SkinTextObject : public Actor {
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
	virtual void SetObject(XMLElement *e);
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
	virtual void SetObject(XMLElement *e);
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
	virtual void SetObject(XMLElement *e);
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
	virtual void SetObject(XMLElement *e);
	virtual void Render();
	// TODO
	void EditValue(int dx, int dy);
};

class SkinButtonObject : public SkinImageObject {
private:
	ImageSRC hover;
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
class SkinListObject : public Actor {
private:
	Uint32				listsize;
	double				listscroll;
	Uint32				x, y, w, h;
	Uint32				select_dx, select_dy;
public:
	void RenderListItem(int idx, Actor *obj);
};


/** @brief it actually doesn't renders anything, but able to work with Render() method. */
class SkinScriptObject : public Actor {
	bool runoneveryframe;
	int runtime;
	RString script;
public:
	SkinScriptObject(SkinRenderTree *);
	void SetRunCondition(bool oneveryframe = false);
	void LoadFile(const RString &filepath);
	void SetScript(const RString &script);
	virtual void SetObject(XMLElement *e);
	virtual void Update();
	virtual void Render();
};