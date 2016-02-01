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
	if (combo) combo->Update();
	if (judge) {
		if (makeoffset)
			rtree->PushRenderOffset(-combo->GetWidth() / 2, 0);
		else
			rtree->PushRenderOffset(0, 0);
		judge->Update();
		rtree->PopRenderOffset();
	}
	drawable = condition.Evaluate();
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
	judge = 0;
	combo = 0;
	XMLElement *e_judge = e->FirstChildElement("Image");
	XMLElement *e_combo = e->FirstChildElement("Number");
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
		int activedot = *v * dotcnt;	// till when we should display guage as active cell
		int groovedot = 0.8 * dotcnt;	// starting point of turning red when groove guage
		int blink = t->GetTick() % 4;	// used when blinking guage
		for (int i = 0; i < dotcnt; i++) {
			// check for current status to decide SRC
			inactive = (i > activedot) ? 1 : 0;
			// grooveGauge => print as hard Gauge from 80%
			if (Gaugetype_now == 0 && i >= groovedot)
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
SkinGroupObject(owner), imgobj_judgeline(0), imgobj_line(0) {
	objtype = ACTORTYPE::PLAYLANE;
}

void SkinNoteFieldObject::SetObject(XMLElement *e) {
	// fetch player object first
	side = e->IntAttribute("side");
	p = PLAYER[side];

	// similar with SkinRenderHelper::ConstructObject()
	// but in here, only parses Image / Note / JudgeLine / Line objects.
#define ISNAME(name) (strcmp(child->Name(), name) == 0)
#define APPENDCHILD(objtype)\
	SkinRenderObject *obj = rtree->New##objtype();\
	obj->SetObject(child);\
	AddChild(obj);
	for (XMLElement *child = e->FirstChildElement(); child; child = child->NextSiblingElement()) {
		if (ISNAME("Image")) {
			APPENDCHILD(SkinImageObject);
		}
		if (ISNAME("LINE")) {
			APPENDCHILD(SkinNoteLineObject);
		}
		if (ISNAME("JUDGELINE")) {
			APPENDCHILD(SkinNoteJudgeLineObject);
		}
		if (ISNAME("Note")) {
			APPENDCHILD(SkinImageObject);
			((SkinNoteObject*)obj)->SetPlayer(p);
		}
	}
}

void SkinNoteFieldObject::ConstructLane(XMLElement *lane) {
	using namespace SkinRenderHelper;
	if (!lane)
		return;

	int idx = lane->IntAttribute("index");
	if (idx < 0 || idx >= 20) {
		LOG->Warn("Invalid Lane found in <Play> object (%d), ignore.", idx);
		return;
	}
	XMLElement *src_element;
}

void SkinNoteFieldObject::SetJudgelineObject(XMLElement *judgelineobj) {
	if (!judgelineobj) return;
	if (!imgobj_judgeline)
		imgobj_judgeline = rtree->NewSkinImageObject();
	imgobj_judgeline->SetObject(judgelineobj);
}

void SkinNoteFieldObject::SetLineObject(XMLElement *lineobj) {
	if (!lineobj) return;
	if (!imgobj_line)
		imgobj_line = rtree->NewSkinImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_line, lineobj);
	imgobj_line->SetObject(lineobj);
}

void SkinNoteFieldObject::RenderLine(double pos) {
	/*
	 * since we don't create multiple line object,
	 * update line object as we render
	 */
	if (!drawable) return;
	rtree->PushRenderOffset(dst_cached.frame.x, -(int)dst_cached.frame.h * pos);
	imgobj_line->Update();
	rtree->PopRenderOffset();
	imgobj_line->Render();
}

void SkinNoteFieldObject::RenderJudgeLine() {
	/*
	* since we don't create multiple judgeline object,
	* update line object as we render
	*/
	if (!drawable) return;
	rtree->PushRenderOffset(dst_cached.frame.x, 0);
	imgobj_judgeline->Update();
	rtree->PopRenderOffset();
	imgobj_judgeline->Render();
}





SkinNoteObject::SkinNoteObject(SkinRenderTree *tree)
	: SkinRenderObject(tree) {
	memset(&note, 0, sizeof(note));
}

void SkinNoteObject::RenderNote(double pos) {
	ImageDSTFrame frame = note.f;
	frame.x += dst_cached.frame.x;
	frame.y -= dst_cached.frame.h * pos;

	frame.x += rtree->GetOffsetX();
	frame.y += rtree->GetOffsetY();
	SkinRenderHelper::Render(note.img, &note.normal, &frame);
}

void SkinNoteObject::RenderMineNote(double pos) {
	ImageDSTFrame frame = note.f;
	frame.x += dst_cached.frame.x;
	frame.y -= dst_cached.frame.h * pos;

	frame.x += rtree->GetOffsetX();
	frame.y += rtree->GetOffsetY();
	SkinRenderHelper::Render(note.img, &note.normal, &frame);
}

void SkinNoteObject::RenderNote(double pos_start, double pos_end) {
	ImageDSTFrame frame = note.f;
	frame.x += dst_cached.frame.x;
	ImageDSTFrame bodyframe = frame;
	ImageDSTFrame endframe = frame;
	frame.y -= dst_cached.frame.h * pos_start;
	endframe.y -= dst_cached.frame.h * pos_end;
	bodyframe.y = endframe.y + endframe.h;
	bodyframe.h = frame.y - endframe.y - endframe.h;

	frame.x += rtree->GetOffsetX();
	frame.y += rtree->GetOffsetY();
	bodyframe.x += rtree->GetOffsetX();
	bodyframe.y += rtree->GetOffsetY();
	endframe.x += rtree->GetOffsetX();
	endframe.y += rtree->GetOffsetY();
	SkinRenderHelper::Render(note.img, &note.ln_body, &bodyframe);
	SkinRenderHelper::Render(note.img, &note.ln_end, &endframe);
	SkinRenderHelper::Render(note.img, &note.ln_start, &frame);
}

void SkinNoteObject::SetObject(XMLElement *lane)
{
	if (!lane) return;
	using namespace SkinRenderHelper;
	XMLElement *src_element;

	note.f.a = 255;
	note.f.r = 255;
	note.f.g = 255;
	note.f.b = 255;
	note.f.angle = 0;
	note.img = rtree->_imagekey[lane->Attribute("resid")];
	src_element = lane->FirstChildElement("SRC_LN_START");
	if (src_element) ConstructSRCFromElement(note.ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_LN_BODY");
	if (src_element) ConstructSRCFromElement(note.ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_LN_END");
	if (src_element) ConstructSRCFromElement(note.ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_MINE");
	if (src_element) ConstructSRCFromElement(note.mine, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_NOTE");
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
	// TODO: consider speed
	if (!note.img) return;

	double pos = BmsHelper::GetCurrentPos();	// updated value
	bool isln = false;
	double lnprevpos;
	for (auto it = p->GetCurrentNoteIter(lane); it != p->GetEndNoteIter(lane); ++it) {
		double notepos = 
			BmsResource::BMS.GetBarManager().GetPosByBar(it->first) - pos;
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
				auto it_prev = it;
				--it_prev;
				lnprevpos = 
					BmsResource::BMS.GetBarManager().GetPosByBar(it_prev->first) - pos;
			}
			RenderNote(lnprevpos, notepos);
			isln = false;
			break;
		}
		// only break loop if longnote is finished
		if (notepos > 1 && !isln) break;
	}
}
#pragma endregion PLAYOBJECT






SkinNoteLineObject::SkinNoteLineObject(SkinRenderTree *t) : SkinImageObject(t) {}

void SkinNoteLineObject::RenderLine(double pos) {
	ImageDSTFrame f;
	f = dst_cached.frame;
	f.h *= pos;
	SkinRenderHelper::Render(img, &imgsrc, &f);
}

void SkinNoteLineObject::Render() {
	// render barrrrrrrrrr
	// TODO: consider speed
	using namespace BmsResource;
	double pos = BmsHelper::GetCurrentPos();
	measureindex m = BMS.GetBarManager().GetMeasureByBarNumber(BmsHelper::GetCurrentBar());
	barindex bar = BMS.GetBarManager().GetBarNumberByMeasure(m);
	for (; m < BmsConst::BAR_MAX_COUNT; m++) {
		double measurepos = BMS.GetBarManager().GetPosByBar(bar) - pos;
		if (measurepos > 1) break;
		RenderLine(measurepos);
		bar += BMS.GetBarManager()[m];
	}
}






SkinNoteJudgeLineObject::SkinNoteJudgeLineObject(SkinRenderTree *t) : SkinImageObject(t) {}







#pragma region BGAOBJECT
SkinBgaObject::SkinBgaObject(SkinRenderTree *t) : SkinRenderObject(t, ACTORTYPE::BGA) {}

void SkinBgaObject::SetObject(XMLElement *e) {
	side = e->IntAttribute("side");
	miss = PLAYERVALUE[side].pOnMiss;
}

void SkinBgaObject::RenderBGA(Image *img) {
	if (!img) return;
	ImageSRC src = { 0, 0, 0, 0, 0, 1, 1, 0, 0 };
	SkinRenderHelper::Render(img, &src, &dst_cached.frame, 0, 5);
}

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
