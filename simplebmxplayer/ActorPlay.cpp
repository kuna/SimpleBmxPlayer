#include "ActorPlay.h"
#include "ActorRenderer.h"
#include "logger.h"
#include "bmsresource.h"
#include "player.h"

SkinComboObject::SkinComboObject(SkinRenderTree *owner)
	: SkinRenderObject(owner, ACTORTYPE::GENERAL), makeoffset(true),
	combo(0), judge(0) {}

void SkinComboObject::Update() {
	// combo object itself isn't a group, so we don't do UpdateBasic();
	// (that is, don't calculate DST attribute)
	drawable = condition.Evaluate();
	if (!drawable) return;
	if (combo) combo->Update();
	if (judge) {
		if (makeoffset && combo)
			rtree->PushRenderOffset(-combo->GetWidth() / 2, 0);
		else
			rtree->PushRenderOffset(0, 0);
		judge->Update();
		rtree->PopRenderOffset();
	}
}

void SkinComboObject::Render() {
	if (drawable) {
		if (combo) {
			combo->Render();
		}
		if (judge) {
			judge->Render();
		}
	}
}

void SkinComboObject::SetOffset(bool offset) {
	makeoffset = offset;
}

void SkinComboObject::SetObject(XMLElement *e) {
	SetBasicObject(e);	// parse condition
	judge = 0;
	combo = 0;
	XMLElement *e_judge = e->FirstChildElement("sprite");
	XMLElement *e_combo = e->FirstChildElement("number");
	if (e_judge) {
		judge = rtree->NewSkinImageObject();
		judge->SetObject(e_judge);
	}
	if (e_combo) {
		combo = rtree->NewSkinNumberObject();
		combo->SetObject(e_combo);
	}
}

void SkinComboObject::SetComboObject(SkinNumberObject* obj) { combo = obj; }

void SkinComboObject::SetJudgeObject(SkinImageObject* obj) { judge = obj; }







SkinGrooveGaugeObject::SkinGrooveGaugeObject(SkinRenderTree* owner)
	: SkinImageObject(owner), dotcnt(0), v(0), addx(0), addy(0) { }

void SkinGrooveGaugeObject::SetObject(XMLElement *e) {
	using namespace SkinRenderHelper;
	// set basic property
	SetBasicObject(e);
	SetImageObject(e);
	addx = e->IntAttribute("addx");
	addy = e->IntAttribute("addy");
	if (e->IntAttribute("side") == 0) {
		v = DOUBLEPOOL->Get("P1Gauge");
		Gaugetype = INTPOOL->Get("P1GaugeType");
	}
	else {
		v = DOUBLEPOOL->Get("P2Gauge");
		Gaugetype = INTPOOL->Get("P2GaugeType");
	}
	t = TIMERPOOL->Get("OnScene");
	dotcnt = 50;
	// set SRC
	int w = e->IntAttribute("w");
	int h = e->IntAttribute("h");
	int dx = e->IntAttribute("divx");
	int dy = e->IntAttribute("divy");
	int dw = w / (dx ? dx : 1);
	int dh = h / (dy ? dy : 1);
	int dc = (dx ? dx : 1) * (dy ? dy : 1);
	int x = e->IntAttribute("x");
	int y = e->IntAttribute("y");
	// TEMP; TODO
	XMLElement *active, *inactive;
	active = e->FirstChildElement("SRC_GROOVE_ACTIVE");
	inactive = e->FirstChildElement("SRC_GROOVE_INACTIVE");
	if (active) ConstructSRCFromElement(src_combo_active[0], active);
	if (inactive) ConstructSRCFromElement(src_combo_inactive[0], inactive);
	active = e->FirstChildElement("SRC_HARD_ACTIVE");
	inactive = e->FirstChildElement("SRC_HARD_INACTIVE");
	if (active) ConstructSRCFromElement(src_combo_active[1], active);
	if (inactive) ConstructSRCFromElement(src_combo_inactive[1], inactive);
	active = e->FirstChildElement("SRC_EX_ACTIVE");
	inactive = e->FirstChildElement("SRC_EX_INACTIVE");
	if (active) ConstructSRCFromElement(src_combo_active[2], active);
	if (inactive) ConstructSRCFromElement(src_combo_inactive[2], inactive);
}

void SkinGrooveGaugeObject::Render() {
	// work like a graph
	if (drawable && v && img) {
		ImageDSTFrame f = dst_cached.frame;
		ImageSRC *currentsrc;
		int inactive = 0;
		int Gaugetype_now = *Gaugetype;
		int activedot = (int)(*v * dotcnt) - 1;	// till when we should display gauge as active cell
		int groovedot = (int)(0.8 * dotcnt);	// starting point of turning red when groove gauge
		int blink = t->GetTick() % 4;	// used when blinking gauge
		for (int i = 0; i < dotcnt; i++) {
			// check for current status to decide SRC
			inactive = (i > activedot) ? 1 : 0;
			// grooveGauge => print as hard Gauge from 80%
			if (Gaugetype_now == 0 && i + 1 >= groovedot)
				Gaugetype_now = 1;
			// should I blink?
			switch (activedot - i) {
			case 1:
				if (blink >= 1)
					inactive = true;
				break;
			case 2:
				if (blink >= 2)
					inactive = true;
				break;
			}
			// now we can set currentSRC
			if (inactive)
				currentsrc = &src_combo_inactive[Gaugetype_now];
			else
				currentsrc = &src_combo_active[Gaugetype_now];
			// do render
			SkinRenderHelper::Render(img, currentsrc, &f,
				dst_cached.dst->blend, dst_cached.dst->rotatecenter);

			f.x += addx;
			f.y += addy;
		}
	}
}







#pragma region PLAYOBJECT
SkinNoteFieldObject::SkinNoteFieldObject(SkinRenderTree* owner) :
SkinGroupObject(owner) {
	objtype = ACTORTYPE::PLAYLANE;
}

void SkinNoteFieldObject::SetObject(XMLElement *e) {
	SetBasicObject(e);

	// fetch player object first
	// if not battle, then ignore side attribute
	if (rtree->_keycount == 15 ||
		rtree->_keycount == 17)
		side = e->IntAttribute("side");
	else
		side = 0;
	p = PLAYER[side];

	// similar with SkinRenderHelper::ConstructObject()
	// but in here, only parses Image / Note / JudgeLine / Line objects.
#define ISNAME(name) (strcmp(child->Name(), name) == 0)
#define APPENDCHILD(objtype)\
	SkinRenderObject *obj = rtree->New##objtype();\
	obj->SetObject(child);\
	AddChild(obj);
	for (XMLElement *child = e->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (ISNAME("sprite")) {
			APPENDCHILD(SkinImageObject);
		}
		if (ISNAME("line")) {
			APPENDCHILD(SkinNoteLineObject);
			((SkinNoteLineObject*)obj)->SetPlayer(p);
		}
		if (ISNAME("judgeline")) {
			APPENDCHILD(SkinNoteJudgeLineObject);
		}
		if (ISNAME("note")) {
			APPENDCHILD(SkinNoteObject);
			((SkinNoteObject*)obj)->SetPlayer(p);
		}
	}
}






SkinNoteObject::SkinNoteObject(SkinRenderTree *tree)
	: SkinRenderObject(tree), lane(0) {
	memset(&note, 0, sizeof(note));
}

void SkinNoteObject::RenderNote(double pos) {
	ImageDSTFrame frame = note.f;
	frame.y *= 1 - pos;

	frame.x += rx;
	frame.y += ry;
	SkinRenderHelper::Render(note.img, &note.normal, &frame);
}

void SkinNoteObject::RenderMineNote(double pos) {
	ImageDSTFrame frame = note.f;
	frame.y *= 1 - pos;

	frame.x += rx;
	frame.y += ry;
	SkinRenderHelper::Render(note.img, &note.normal, &frame);
}

void SkinNoteObject::RenderNote(double pos_start, double pos_end) {
	ImageDSTFrame frame = note.f;
	ImageDSTFrame bodyframe = frame;
	ImageDSTFrame endframe = frame;
	frame.y *= 1 - pos_start;
	endframe.y *= 1 - pos_end;
	bodyframe.y = endframe.y + endframe.h;
	bodyframe.h = frame.y - endframe.y - endframe.h;

	frame.x += rx;
	frame.y += ry;
	bodyframe.x += rx;
	bodyframe.y += ry;
	endframe.x += rx;
	endframe.y += ry;
	SkinRenderHelper::Render(note.img, &note.ln_body, &bodyframe);
	SkinRenderHelper::Render(note.img, &note.ln_end, &endframe);
	SkinRenderHelper::Render(note.img, &note.ln_start, &frame);
}

void SkinNoteObject::SetObject(XMLElement *lane)
{
	// COMMENT: no dst in noteobject
	// SetBasicObject(lane);

	if (!lane) return;
	using namespace SkinRenderHelper;
	this->lane = lane->IntAttribute("index");
	note.f.x = lane->IntAttribute("x");
	note.f.y = lane->IntAttribute("y");
	note.f.w = lane->IntAttribute("w");
	note.f.h = lane->IntAttribute("h");
	note.f.a = 255;
	note.f.r = 255;
	note.f.g = 255;
	note.f.b = 255;
	note.f.angle = 0;
	note.img = rtree->_imagekey[lane->Attribute("resid")];
	XMLElement *src_element;
	src_element = lane->FirstChildElement("SRC_NOTE");
	if (src_element) ConstructSRCFromElement(note.normal, src_element);
	src_element = lane->FirstChildElement("SRC_LN_START");
	if (src_element) ConstructSRCFromElement(note.ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_LN_BODY");
	if (src_element) ConstructSRCFromElement(note.ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_LN_END");
	if (src_element) ConstructSRCFromElement(note.ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_MINE");
	if (src_element) ConstructSRCFromElement(note.mine, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_NOTE");
	if (src_element) ConstructSRCFromElement(note.auto_normal, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_START");
	if (src_element) ConstructSRCFromElement(note.auto_ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_BODY");
	if (src_element) ConstructSRCFromElement(note.auto_ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_END");
	if (src_element) ConstructSRCFromElement(note.auto_ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_MINE");
	if (src_element) ConstructSRCFromElement(note.auto_mine, src_element);
}

void SkinNoteObject::Render() {
	if (!note.img) return;
	if (!drawable) return;
	if (!p) return;

	double speed = p->GetSpeedMul();
	double pos = BmsHelper::GetCurrentPos();	// current scroll pos
	// basic note drawing
	// only draw notes that is not judged yet
	bool isln = false;
	double lnprevpos;
	for (auto it = p->GetNoteIter(lane); it != p->GetNoteEndIter(lane); ++it) {
		double notepos = 
			BmsResource::BMS.GetBarManager().GetPosByBar(it->first) - pos;
		notepos *= speed;
		if (notepos < 0) notepos = 0;
		switch (it->second.type) {
		case BmsNote::NOTE_NORMAL:
			RenderNote(notepos);
			break;
		case BmsNote::NOTE_MINE:
			RenderMineNote(notepos);
			break;
		case BmsNote::NOTE_LNSTART:
			lnprevpos = notepos;
			isln = true;
			break;
		case BmsNote::NOTE_LNEND:
			if (!isln) {
				lnprevpos = 0;
			}
			RenderNote(lnprevpos, notepos);
			isln = false;
			break;
		}
		// only break loop if longnote is finished
		if (notepos > 1 && !isln) break;
	}
	// there might be longnote that isn't drawn (judge failed)
	// in LR2, that also should be drawn, so scan it
	BmsNoteLane::Iterator it_prev = p->GetNoteIter(lane);
	if (it_prev == p->GetNoteBeginIter(lane)) return;
	isln = false;
	for (--it_prev; it_prev != p->GetNoteBeginIter(lane); --it_prev) {
		double notepos =
			BmsResource::BMS.GetBarManager().GetPosByBar(it_prev->first) - pos;
		notepos *= speed;
		// only break loop when out-of-screen
		if (notepos < 0 && !isln) break;
		if (notepos < 0) notepos = 0;
		switch (it_prev->second.type) {
		case BmsNote::NOTE_LNEND:
			lnprevpos = notepos;
			isln = true;
			break;
		case BmsNote::NOTE_LNSTART:
			if (!isln) break;
			RenderNote(notepos, lnprevpos);
			isln = false;
			break;
		}
	}
}

void SkinNoteObject::Update() {
	// only update drawable & relative position
	drawable = condition.Evaluate();
	rx = rtree->GetOffsetX();
	ry = rtree->GetOffsetY();
}
#pragma endregion PLAYOBJECT






SkinNoteLineObject::SkinNoteLineObject(SkinRenderTree *t) : SkinImageObject(t) {}

void SkinNoteLineObject::RenderLine(double pos) {
	ImageDSTFrame f;
	f = dst_cached.frame;
	f.y *= 1 - pos;
	SkinRenderHelper::Render(img, &imgsrc, &f);
}

void SkinNoteLineObject::Render() {
	if (!p) return;
	// render barrrrrrrrrrs
	using namespace BmsResource;
	double speed = p->GetSpeedMul();
	double pos = BmsHelper::GetCurrentPos();
	measureindex m = BMS.GetBarManager().GetMeasureByBarNumber(BmsHelper::GetCurrentBar());
	barindex bar = BMS.GetBarManager().GetBarNumberByMeasure(m);
	for (; m < BmsConst::BAR_MAX_COUNT; m++) {
		double measurepos = BMS.GetBarManager().GetPosByBar(bar) - pos;
		measurepos *= speed;
		if (measurepos > 1) break;
		RenderLine(measurepos);
		bar += BMS.GetBarManager()[m];
	}
}






SkinNoteJudgeLineObject::SkinNoteJudgeLineObject(SkinRenderTree *t) : SkinImageObject(t) {}







#pragma region BGAOBJECT
SkinBgaObject::SkinBgaObject(SkinRenderTree *t) : SkinRenderObject(t, ACTORTYPE::BGA) {}

void SkinBgaObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	side = e->IntAttribute("side");
	miss = PLAYERVALUE[side].pOnMiss;
}

void SkinBgaObject::RenderBGA(Image *img) {
	if (!img) return;
	ImageSRC src = { 0, 0, 0, 0, 0, 1, 1, 0, 0 };
	SkinRenderHelper::Render(img, &src, &dst_cached.frame, 0, 5);
}

void SkinBgaObject::Update() { UpdateBasic(); }

void SkinBgaObject::Render() {
	if (!drawable) return;
	/*
	 * BGA is common resource so get it from BmsResource, it's easy.
	 * but one thing should be taken care of - 
	 * Miss timer is different from player.
	 */
	// TODO: layer1/2 transparent #000000 color
	RenderBGA(BmsHelper::GetMainBGA());
	RenderBGA(BmsHelper::GetLayer1BGA());
	RenderBGA(BmsHelper::GetLayer2BGA());
	if (miss->IsStarted() && miss->GetTick() < 1000)
		RenderBGA(BmsHelper::GetMissBGA());
}
#pragma endregion BGAOBJECT
