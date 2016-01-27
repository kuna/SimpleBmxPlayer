#include "ActorBasic.h"
#include "global.h"
#include "image.h"
#include "timer.h"

/*
* special object for playing
*/
class SkinComboObject : public SkinRenderObject {
	SkinImageObject *judge;
	SkinNumberObject *combo;
	bool makeoffset;
public:
	SkinComboObject(SkinRenderTree *owner);
	void SetOffset(bool offset);
	void SetJudgeObject(SkinImageObject *);
	void SetComboObject(SkinNumberObject *);
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
	void SetObject(XMLElement *e);
	virtual void Render();
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
	void RenderLine(double pos);
	void RenderJudgeLine();
};

/** @brief do nothing; just catch this object if you want to draw BGA. */
class SkinBgaObject : public SkinRenderObject {
public:
	SkinBgaObject(SkinRenderTree *);
	void RenderBGA(Image *bga);
};
