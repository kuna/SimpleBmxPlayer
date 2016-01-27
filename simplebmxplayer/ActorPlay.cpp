#include "ActorPlay.h"
#include "ActorPlay.h"


SkinComboObject::SkinComboObject(SkinRenderTree *owner)
	: SkinRenderObject(owner, OBJTYPE::GENERAL), makeoffset(true),
	combo(0), judge(0) {}

void SkinComboObject::Render() {
	// same as image, but x position is little differnt
	int offset_x = 0;
	if (condition.Evaluate()) {
		if (combo) {
			combo->Update();
			combo->Render();
			if (makeoffset) offset_x = -combo->GetWidth() / 2;
		}
		if (judge) {
			SkinRenderHelper::PushRenderOffset(offset_x, 0);
			judge->Update();
			judge->Render();
			SkinRenderHelper::PopRenderOffset();
		}
	}
}

void SkinComboObject::SetOffset(bool offset) {
	makeoffset = offset;
}

void SkinComboObject::SetComboObject(SkinNumberObject* obj) { combo = obj; }

void SkinComboObject::SetJudgeObject(SkinImageObject* obj) { judge = obj; }

SkinGrooveGaugeObject::SkinGrooveGaugeObject(SkinRenderTree* owner)
	: SkinImageObject(owner), dotcnt(0), v(0), addx(0), addy(0) { }

void SkinGrooveGaugeObject::SetObject(XMLElement *e) {
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
SkinPlayObject::SkinPlayObject(SkinRenderTree* owner) :
SkinGroupObject(owner), imgobj_judgeline(0), imgobj_line(0) {
	objtype = OBJTYPE::PLAYLANE;
	memset(Note, 0, sizeof(Note));
}

void SkinPlayObject::ConstructLane(XMLElement *lane) {
	if (!lane)
		return;

	int idx = lane->IntAttribute("index");
	if (idx < 0 || idx >= 20) {
		LOG->Warn("Invalid Lane found in <Play> object (%d), ignore.", idx);
		return;
	}
	XMLElement *src_element;
	Note[idx].img = rtree->_imagekey[lane->Attribute("resid")];
	src_element = lane->FirstChildElement("SRC_NOTE");
	if (src_element) {
		ConstructSRCFromElement(Note[idx].normal, src_element);
	}
	src_element = lane->FirstChildElement("SRC_LN_START");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_LN_BODY");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_LN_END");
	if (src_element) ConstructSRCFromElement(Note[idx].ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_MINE");
	if (src_element) ConstructSRCFromElement(Note[idx].mine, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_NOTE");
	Note[idx].img = rtree->_imagekey[lane->Attribute("resid")];
	if (src_element) {
		ConstructSRCFromElement(AutoNote[idx].normal, src_element);
	}
	src_element = lane->FirstChildElement("SRC_AUTO_LN_START");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_start, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_BODY");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_body, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_END");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].ln_end, src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_MINE");
	if (src_element) ConstructSRCFromElement(AutoNote[idx].mine, src_element);
#define SETFRAME(attr)	Note[idx].f.##attr = lane->IntAttribute(#attr);
	SETFRAME(x);
	SETFRAME(y);
	SETFRAME(w);
	SETFRAME(h);
	Note[idx].f.a = 255;
	Note[idx].f.r = 255;
	Note[idx].f.g = 255;
	Note[idx].f.b = 255;
	Note[idx].f.angle = 0;
#undef SETFRAME
}

void SkinPlayObject::SetJudgelineObject(XMLElement *judgelineobj) {
	if (!judgelineobj) return;
	if (!imgobj_judgeline)
		imgobj_judgeline = rtree->NewImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_judgeline, judgelineobj);
	imgobj_judgeline->SetSRC(judgelineobj);
}

void SkinPlayObject::SetLineObject(XMLElement *lineobj) {
	if (!lineobj) return;
	if (!imgobj_line)
		imgobj_line = rtree->NewImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_line, lineobj);
	imgobj_line->SetSRC(lineobj);
}

Uint32 SkinPlayObject::GetLaneHeight() { return h; }

bool SkinPlayObject::IsLaneExists(int laneindex) { return Note[laneindex].img; }

void SkinPlayObject::RenderNote(int laneindex, double pos, bool mine) {
	if (IsLaneExists(laneindex)) {
		ImageDSTFrame lane;
		if (!SkinRenderHelper::CalculateFrame(dst[0], lane))
			return;
		ImageDSTFrame frame = Note[laneindex].f;
		frame.x += lane.x;
		frame.y -= h * pos;
		SkinRenderHelper::Render(Note[laneindex].img, &Note[laneindex].normal, &frame);
	}
}

void SkinPlayObject::RenderNote(int laneindex, double pos_start, double pos_end) {
	if (IsLaneExists(laneindex)) {
		ImageDSTFrame lane;
		if (!SkinRenderHelper::CalculateFrame(dst[0], lane))
			return;
		ImageDSTFrame frame = Note[laneindex].f;
		frame.x += lane.x;
		ImageDSTFrame bodyframe = frame;
		ImageDSTFrame endframe = frame;
		frame.y -= h * pos_start;
		endframe.y -= h * pos_end;
		bodyframe.y = endframe.y + endframe.h;
		bodyframe.h = frame.y - endframe.y - endframe.h;
		SkinRenderHelper::Render(Note[laneindex].img, &Note[laneindex].ln_body, &bodyframe);
		SkinRenderHelper::Render(Note[laneindex].img, &Note[laneindex].ln_end, &endframe);
		SkinRenderHelper::Render(Note[laneindex].img, &Note[laneindex].ln_start, &frame);
	}
}

void SkinPlayObject::RenderLine(double pos) {
	ImageDSTFrame lane;
	if (!SkinRenderHelper::CalculateFrame(dst[0], lane))
		return;
	SkinRenderHelper::PushRenderOffset(lane.x, -(int)h * pos);
	imgobj_line->Update();
	imgobj_line->Render();
	SkinRenderHelper::PopRenderOffset();
}

void SkinPlayObject::RenderJudgeLine() {
	ImageDSTFrame lane;
	if (!SkinRenderHelper::CalculateFrame(dst[0], lane))
		return;
	SkinRenderHelper::PushRenderOffset(lane.x, 0);
	imgobj_judgeline->Update();
	imgobj_judgeline->Render();
	SkinRenderHelper::PopRenderOffset();
}
#pragma endregion PLAYOBJECT

#pragma region BGAOBJECT
SkinBgaObject::SkinBgaObject(SkinRenderTree *t) : SkinRenderObject(t, ACTORTYPE::BGA) {}

void SkinBgaObject::RenderBGA(Image *bga) {
	if (!bga) return;
	if (!drawable) return;
	ImageSRC src = { 0, 0, 0, 0, 0, 1, 1, 0, 0 };
	SkinRenderHelper::Render(bga, &src, &dst_cached.frame, 0, 5);
}
#pragma endregion BGAOBJECT
