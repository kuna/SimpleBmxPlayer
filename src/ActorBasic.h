#pragma once

#include "global.h"
#include "font.h"
#include <vector>
#include "tinyxml2.h"
using namespace tinyxml2;
#include "Theme.h"


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







// Texture pos
struct ImageSRC {
	Timer *timer;
	int x, y, w, h;
	int divx, divy, cycle;
	int loop;

	ImageSRC();
	void Clear();
	void SetFromXml(const tinyxml2::XMLElement *e);
	Display::Rect Calculate();
};

// State that tweens between each state
struct TweenState {
	Display::Rect dst;
	Display::Color color;
	Display::Rotate rotate;
	Display::PointF shear;
	Display::PointF zoom;
};
// static information about current tweening
struct TweenInfo {
	TweenState state;
	int acc, loop;
	TweenInfo();
};
// Compiled tween state (by lua script or some ...)
struct ImageDST {
	Timer timer;
	std::map<int, TweenInfo> tweens;
	int blend, center;
	bool zwrite;
	int zpos;

	ImageDST();
	void Clear();
	void SetFromCmd(const RString& cmd);
	void CalculateTween(TweenInfo &t);
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








/** @brief very basic rendering object which does nothing. */
class Actor {
protected:
	/** @brief type of this object */
	int objtype;
	Actor* m_pParent;

	/** @brief handlers & functions when handler called */
	//std::map<RString, LuaFunction*> handlers;
	friend class ActorHandler;
	class ActorHandler : public Handler {
	private:
		Actor* pActor;
	public:
		ActorHandler(Actor* p) : pActor(p) {}
		virtual void Trigger(const RString& msg);
	};
	ActorHandler handler;

	/** @brief current DST (compiled when proper handler called) */
	Display::Texture *m_Tex;				// Texture (default: none)
	ImageSRC m_Src;						// Render From
	ImageDST m_Dst;						// Render To (compiled state)
	std::map<RString, RString> m_Cmds;	// Prepare to be compiled
	TweenInfo m_Tweeninfo;				// Current, calculated tween(rendering) state
	int m_SrcValue;						// basic integer value (used for src switching)
	
	RenderCondition m_Condition;		// Is object currently drawable?
	/** @brief different from visible(condition). decided by Renderer(Update() method) */
	bool drawable;

	/** @brief used to check mouse collision event */
	bool TestCollsion(int x, int y);
	bool focusable;
	bool clickable;

	// TODO: we need this? (to get skin width/height)
	//SkinRenderTree* rtree;
public:
	Actor(int type = ACTORTYPE::NONE);
	~Actor();
	int GetType();

	/** @brief Clear all attribute of actor */
	virtual void Clear();
	virtual void SetFromXml(const XMLElement *e);
	virtual void SetParent(Actor* pActor);
	Actor* GetParent() { return m_pParent; };

	/* @brief must call before update object or do something */
	virtual void Update();
	/* @brief draw object to screen with it's own basic style */
	virtual void Render();
	void SetRenderState();
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Click(int x, int y);
	/** @brief tests collsion and if true then do own work & return true. otherwise false. */
	virtual bool Hover(int x, int y);

	// get/set
	virtual void SetCondition(const RString &str);
	virtual int GetWidth();
	virtual int GetHeight();
	virtual int GetX();
	virtual int GetY();
	void SetBlend(int blend);
	void SetCenter(int center);
};

class ActorFrame : public Actor {
protected:
	/** @brief base elements */
	std::vector<Actor*> m_Children;
public:
	ActorFrame();
	~ActorFrame();
	std::vector<Actor*>::iterator begin();
	std::vector<Actor*>::iterator end();

	// resursively parses
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
};

class ActorSprite : public Actor {
public:
	ActorSprite(int type = ACTORTYPE::IMAGE);

	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
};

class ActorText : public Actor {
private:
	/* elements needed for drawing */
	int align;
	bool editable;
	Font *m_Font;
	RString *v;
public:
	ActorText();
	void SetEditable(bool editable);
	void SetAlign(int align);
	void SetValue(RString* s);
	void SetFont(const char* fontid);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Render();
	virtual int GetWidth();
	int GetTextWidth(const RString& s);
	void RenderText(const char* s);
};

class ActorNumber : public ActorText {
private:
	char buf[20];
	bool mode24;
	int length;
	int *v;
public:
	ActorNumber();
	void SetValue(int *i);
	void SetLength(int length);
	void Set24Mode(bool b);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Render();
	virtual int GetWidth();
	/** @brief make string for rendering and call SkinTextObject::RenderText() */
	void CacheInt(int n);
};

class ActorGraph : public ActorSprite {
private:
	/* elements needed for drawing */
	int type;
	int direction;
	double m_Value;
	double *m_Key;
public:
	ActorGraph();
	void SetType(int type);
	void SetDirection(int direction);
	void SetKey(const char* key);
	void SetValue(double v);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
	// TODO
	void EditValue(int dx, int dy);
};

class ActorSlider : public ActorSprite {
private:
	/* elements needed for drawing */
	int direction;
	int range;
	double m_Value;
	double *m_Key;
	//bool editable;	this part is manually controlled in program...?
public:
	ActorSlider();
	void SetDirection(int direction);
	void SetRange(int range);
	void SetKey(const char* key);
	void SetValue(double v);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
	// TODO
	void EditValue(int dx, int dy);
};

class ActorButton : public ActorSprite {
private:
	ImageSRC hover;
	// only we need more is: handler.
	//_Handler handler;
public:
	ActorButton();
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
class ActorList : public Actor {
private:
	Uint32				listsize;
	double				listscroll;
	Uint32				x, y, w, h;
	Uint32				select_dx, select_dy;
public:
	ActorList();
	void RenderListItem(int idx, Actor *obj);
};


/** @brief DEPRECIATED - compatibility for Xml */
class ActorScript : public Actor {
	RString script;
public:
	ActorScript();
	void LoadFile(const RString &filepath);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
};





// simple macro for registering actor
#define REGISTER_ACTOR_WITH_NAME(v, name)\
	Actor* CreateActor_##name() { return new name(); }\
	struct RegisterActor_##name {\
		RegisterActor_##name() { Theme::RegisterActor(v, CreateActor_##name); }\
	} _RegisterActor_##name;


#define REGISTER_ACTOR(name)\
	REGISTER_ACTOR_WITH_NAME(#name, name);