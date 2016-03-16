#pragma once

#include "ActorBasic.h"
#include "global.h"
#include "timer.h"

class Player;

/*
 * special object for playing
 */
class ActorJudge : public ActorSprite {
	class JudgeHandler : public Handler {
	public:
		int combo;
		int side;
		ActorJudge* pActor;
		RString msgname;
		JudgeHandler(ActorJudge* p);
		virtual void Trigger(const RString& id);
	};
	ActorNumber *combo;
	Player *player;
	int playeridx;
	RString handlername;
	JudgeHandler handler_combo;
public:
	ActorJudge();
	~ActorJudge();
	/** @brief should we need to move judge image object left, or not? */
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
};

/* DEPRECIATED form? */
class ActorGrooveGauge : public ActorSprite {
	ActorSprite *obj;
	/*
	* 0: groove, 1: hard blank, 2: ex blank
	*/
	ImageSRC src_combo_active[5];
	ImageSRC src_combo_inactive[5];
	int addx, addy;
	int *Gaugetype;
	int dotcnt;

	double *m_Key;
	double m_Value;
public:
	ActorGrooveGauge();
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
};

/** @brief specific object used during play */
class ActorNoteField : public ActorFrame {
public:
	int			side;			// decide what player this object render
	Player*		p;
public:
	ActorNoteField();
	void SetFromXml(const XMLElement *e);
	// Render() uses basic function of ActorFrame::Render()
};

// draws a lane's note - repeatedly
class ActorNote : public ActorSprite {
private:
	struct NOTE {
		ImageSRC normal;
		ImageSRC ln_end;
		ImageSRC ln_body;
		ImageSRC ln_start;
		ImageSRC mine;
		ImageSRC auto_normal;
		ImageSRC auto_ln_end;
		ImageSRC auto_ln_body;
		ImageSRC auto_ln_start;
		ImageSRC auto_mine;
		ImageDST f;
		Display::Texture tex;			// COMMENT: do we need to have multiple image?
	} note;

	int			laneidx;
	Player*		p;				// get from parent
	TweenInfo	m_Tweenobj;		// copy of m_Tweeninfo

	/** @brief `pos = 1` means note on the top of the lane */
	void RenderNote(double pos);
	void RenderMineNote(double pos);
	/** @brief for longnote. */
	void RenderLongNote(double pos_start, double pos_end);
public:
	ActorNote();
	virtual void SetFromXml(const XMLElement *e);
	virtual void Update();
	virtual void Render();
	void SetPlayer(Player *p) { this->p = p; }
};

// draws object repeatedly
class ActorBeatLine : public ActorSprite {
	Player *p;					// get from parent
	TweenInfo	m_Tweenobj;		// copy of m_Tweeninfo
public:
	ActorBeatLine();
	void RenderLine(double pos);
	virtual void SetFromXml(const XMLElement *e);
	virtual void Render();
	virtual void Update();
	void SetPlayer(Player *p) { this->p = p; }
};

// nothing much, just an tedious object (beatmania/LR2 specific style)
class ActorJudgeLine : public ActorSprite {
public:
	ActorJudgeLine();
};


/** @brief do nothing; just catch this object if you want to draw BGA. */
class ActorBga : public Actor {
private:
	int side;		// which player?
	SwitchValue m_MissTimer;
	Display::Texture* m_TexBlack;
public:
	ActorBga();
	virtual void SetFromXml(const XMLElement *e);
	void RenderBGA(Display::Texture* tex, bool transparent = true);
	virtual void Update();
	virtual void Render();
};
