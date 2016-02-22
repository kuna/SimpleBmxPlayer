#pragma once

#include "ActorBasic.h"
#include "global.h"
#include "timer.h"

class Player;

/*
 * special object for playing
 */
class SkinComboObject : public SkinRenderObject {
	SkinImageObject *judge;
	SkinNumberObject *combo;
	bool makeoffset;
public:
	SkinComboObject(SkinRenderTree *owner);
	/** @brief should we need to move judge image object left, or not? */
	void SetOffset(bool offset);
	void SetJudgeObject(SkinImageObject *);
	void SetComboObject(SkinNumberObject *);
	virtual void SetObject(XMLElement *e);
	virtual void Update();
	virtual void Render();
};

class SkinGrooveGaugeObject : public SkinImageObject {
	SkinImageObject *obj;
	/*
	* 0: groove, 1: hard blank, 2: ex blank
	*/
	ImageSRC src_combo_active[5];
	ImageSRC src_combo_inactive[5];
	Timer *t;			// general-purpose timer for blink
	int addx, addy;
	int *Gaugetype;
	double *v;
	int dotcnt;
public:
	SkinGrooveGaugeObject(SkinRenderTree *owner);
	virtual void SetObject(XMLElement *e);
	virtual void Render();
};

/** @brief specific object used during play */
class SkinNoteFieldObject : public SkinGroupObject {
public:
	int			side;			// decide what player this object render
	Player*		p;
public:
	SkinNoteFieldObject(SkinRenderTree* owner);
	void SetObject(XMLElement *e);
	// Render() uses basic function of SkinGroupObject::Render()
};

// draws a lane's note - repeatedly
class SkinNoteObject : public SkinRenderObject {
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
		ImageDSTFrame f;
		Display::Texture tex;			// COMMENT: do we need to have multiple image?
	} note;

	int			lane;
	Player*		p;				// get from NoteFieldObject

	int			rx, ry;			// relative pos

	/** @brief `pos = 1` means note on the top of the lane */
	void RenderNote(double pos);
	void RenderMineNote(double pos);
	/** @brief for longnote. */
	void RenderNote(double pos_start, double pos_end);
public:
	SkinNoteObject(SkinRenderTree *);
	virtual void SetObject(XMLElement *e);
	virtual void Update();
	virtual void Render();
	void SetPlayer(Player *p) { this->p = p; }
};

// draws object repeatedly
class SkinNoteLineObject : public SkinImageObject {
	Player *p;				// get from NoteFieldObject
public:
	SkinNoteLineObject(SkinRenderTree *);
	void RenderLine(double pos);
	virtual void Render();
	void SetPlayer(Player *p) { this->p = p; }
};

// nothing much, just an tedious object (beatmania/LR2 specific style)
class SkinNoteJudgeLineObject : public SkinImageObject {
public:
	SkinNoteJudgeLineObject(SkinRenderTree *);
};


/** @brief do nothing; just catch this object if you want to draw BGA. */
class SkinBgaObject : public SkinRenderObject {
private:
	int side;		// which player?
	Timer *miss;	// the player's miss timer
public:
	SkinBgaObject(SkinRenderTree *);
	virtual void SetObject(XMLElement *e);
	void RenderBGA(Display::Texture* tex);
	virtual void Update();
	virtual void Render();
};
