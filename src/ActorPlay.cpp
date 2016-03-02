#include "ActorPlay.h"
#include "logger.h"
#include "songplayer.h"
#include "player.h"

ActorJudge::ActorJudge()
	: ActorSprite(), combo(0) {}

void ActorJudge::Update() {
	ActorSprite::Update();
	if (!drawable) return;
	// combo position is relative to ActorJudge
	if (combo) {
		combo->Update();
		m_Tweeninfo.state.dst.x += -combo->GetWidth() / 2;
	}
}

void ActorJudge::Render() {
	ActorSprite::Render();
	if (drawable && combo) {
		// a little like ActorFrame does.
		DISPLAY->PushState();
		DISPLAY->SetOffset(m_Tweeninfo.state.dst.x, m_Tweeninfo.state.dst.y);
		combo->Render();
		DISPLAY->PopState();
	}
}

void ActorJudge::SetFromXml(const XMLElement *e) {
	combo = 0;
	ActorSprite::SetFromXml(e);
	const XMLElement *e_combo = e->FirstChildElement("number");
	if (e_combo) {
		combo = (ActorNumber*)Theme::MakeActor(e_combo, this);
	}
}







ActorGrooveGauge::ActorGrooveGauge()
	: ActorSprite(), dotcnt(0), m_Key(0), m_Value(0), addx(0), addy(0) { }

void ActorGrooveGauge::SetFromXml(const XMLElement *e) {
	// set basic property
	ActorSprite::SetFromXml(e);

	addx = e->IntAttribute("addx");
	addy = e->IntAttribute("addy");
	if (e->IntAttribute("side") == 0) {
		m_Key = DOUBLEPOOL->Get("P1Gauge");
		Gaugetype = INTPOOL->Get("P1GaugeType");
	}
	else {
		m_Key = DOUBLEPOOL->Get("P2Gauge");
		Gaugetype = INTPOOL->Get("P2GaugeType");
	}
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
	const XMLElement *active, *inactive;
	active = e->FirstChildElement("SRC_GROOVE_ACTIVE");
	inactive = e->FirstChildElement("SRC_GROOVE_INACTIVE");
	if (active) src_combo_active[0].SetFromXml(active);
	if (inactive) src_combo_inactive[0].SetFromXml(inactive);
	active = e->FirstChildElement("SRC_HARD_ACTIVE");
	inactive = e->FirstChildElement("SRC_HARD_INACTIVE");
	if (active) src_combo_active[1].SetFromXml(active);
	if (inactive) src_combo_inactive[1].SetFromXml(inactive);
	active = e->FirstChildElement("SRC_EX_ACTIVE");
	inactive = e->FirstChildElement("SRC_EX_INACTIVE");
	if (active) src_combo_active[2].SetFromXml(active);
	if (inactive) src_combo_inactive[2].SetFromXml(inactive);
}

void ActorGrooveGauge::Update() {
	ActorSprite::Update();

	// TODO: receive value if new one exists
	if (m_Key) {
		m_Value = *m_Key;
	}
}

void ActorGrooveGauge::Render() {
	// work like a graph
	if (drawable && m_Tex) {
		// before start, do basic settings
		Actor::SetRenderState();
		DISPLAY->SetTexture(m_Tex);

		Display::Rect f = m_Tweeninfo.state.dst;
		ImageSRC *currentsrc;
		int inactive = 0;
		int Gaugetype_now = *Gaugetype;
		int activedot = (int)(m_Value * dotcnt) - 1;	// till when we should display gauge as active cell
		int groovedot = (int)(0.8 * dotcnt);	// starting point of turning red when groove gauge
		int blink = m_Dst.timer.GetTick() % 4;	// used when blinking gauge
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
			Display::Rect sr = currentsrc->Calculate();
			DISPLAY->SetSRC(&sr);
			DISPLAY->SetDST(&f);
			DISPLAY->DrawPrimitives();

			f.x += addx;
			f.y += addy;
		}
	}
}







#pragma region PLAYOBJECT
ActorNoteField::ActorNoteField() : ActorFrame() {
	objtype = ACTORTYPE::PLAYLANE;
}

void ActorNoteField::SetFromXml(const XMLElement *e) {
	ActorFrame::SetFromXml(e);

	// fetch player object
	// if not battle, then ignore side attribute
	// (TODO)
	/*if (rtree->_keycount == 15 ||
		rtree->_keycount == 17)
		side = e->IntAttribute("side");
	else
		side = 0;*/
	p = PLAYER[side];
}






ActorNote::ActorNote()
	: ActorSprite(), laneidx(0) {
	memset(&note, 0, sizeof(note));
}

void ActorNote::SetFromXml(const XMLElement *lane)
{
	if (!lane) return;
	Actor::SetFromXml(lane);

	this->laneidx = lane->IntAttribute("index");
	int player = laneidx / 10;
	p = PLAYER[player];

	const XMLElement *src_element;
	src_element = lane->FirstChildElement("SRC_NOTE");
	if (src_element) note.normal.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_LN_START");
	if (src_element) note.ln_start.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_LN_BODY");
	if (src_element) note.ln_body.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_LN_END");
	if (src_element) note.ln_end.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_MINE");
	if (src_element) note.mine.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_NOTE");
	if (src_element) note.auto_normal.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_START");
	if (src_element) note.auto_ln_start.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_BODY");
	if (src_element) note.auto_ln_body.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_LN_END");
	if (src_element) note.auto_ln_end.SetFromXml(src_element);
	src_element = lane->FirstChildElement("SRC_AUTO_MINE");
	if (src_element) note.auto_mine.SetFromXml(src_element);
}

void ActorNote::Update() {
	Actor::Update();
	// copy dst information
	m_Tweenobj = m_Tweeninfo;
}

void ActorNote::Render() {
	if (!drawable) return;	// should be drawable
	if (!p) return;			// player object should exist

	double speed = p->GetSpeedMul();
	double pos = SONGPLAYER->GetCurrentPos();	// current scroll pos
	// basic note drawing
	// only draw notes that is not judged yet
	bool isln = false;
	double lnprevpos;
	for (auto it = p->GetNoteIter(laneidx); it != p->GetNoteEndIter(laneidx); ++it) {
		double notepos = it->second.pos - pos;
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
			RenderLongNote(lnprevpos, notepos);
			isln = false;
			break;
		}
		// only break loop if longnote is finished
		if (notepos > 1 && !isln) break;
	}
	// there might be longnote that isn't drawn (judge failed)
	// in LR2, that also should be drawn, so scan it
	BmsNoteLane::Iterator it_prev = p->GetNoteIter(laneidx);
	if (it_prev == p->GetNoteBeginIter(laneidx)) return;
	isln = false;
	for (--it_prev; it_prev != p->GetNoteBeginIter(laneidx); --it_prev) {
		double notepos = it_prev->second.pos - pos;
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
			RenderLongNote(notepos, lnprevpos);
			isln = false;
			break;
		}
	}
}

void ActorNote::RenderNote(double pos) {
	m_Src = note.normal;
	m_Tweeninfo.state.dst.y = m_Tweenobj.state.dst.y * (1 - pos);
	ActorSprite::Render();
}

void ActorNote::RenderMineNote(double pos) {
	m_Src = note.normal;
	m_Tweeninfo.state.dst.y = m_Tweenobj.state.dst.y * (1 - pos);
	ActorSprite::Render();
}

void ActorNote::RenderLongNote(double pos_start, double pos_end) {
	double p_start = m_Tweenobj.state.dst.y * (1 - pos_start);
	double p_end = m_Tweenobj.state.dst.y * (1 - pos_end);

	// render bodyframe
	m_Src = note.ln_body;
	m_Tweeninfo.state.dst.y = p_start + m_Tweenobj.state.dst.h;
	m_Tweeninfo.state.dst.h = p_start - p_end - m_Tweenobj.state.dst.h;
	ActorSprite::Render();

	// render endframe
	m_Src = note.ln_end;
	m_Tweeninfo.state.dst.y = p_end;
	m_Tweeninfo.state.dst.h = m_Tweenobj.state.dst.h;
	ActorSprite::Render();
	
	// render frame
	m_Src = note.ln_start;
	m_Tweeninfo.state.dst.y = p_start;
	m_Tweeninfo.state.dst.h = m_Tweenobj.state.dst.h;
	ActorSprite::Render();
}
#pragma endregion PLAYOBJECT






ActorBeatLine::ActorBeatLine() : ActorSprite() {}

void ActorBeatLine::SetFromXml(const XMLElement* e) {
	ActorSprite::SetFromXml(e);
	// get player object
	p = PLAYER[e->IntAttribute("side")];
}

void ActorBeatLine::RenderLine(double pos) {
	m_Tweeninfo.state.dst.y = m_Tweenobj.state.dst.y * (1 - pos);
	ActorSprite::Render();
}

void ActorBeatLine::Render() {
	if (!p) return;
	// render barrrrrrrrrrs
	BmsBms* bms = SONGPLAYER->GetBmsObject();
	double speed = p->GetSpeedMul();
	double pos = SONGPLAYER->GetCurrentPos();
	measureindex m = bms->GetBarManager().GetMeasureByBar(SONGPLAYER->GetCurrentBar());
	barindex bar = bms->GetBarManager().GetBarNumberByMeasure(m);
	for (; m < BmsConst::BAR_MAX_COUNT; m++) {
		double measurepos = bms->GetBarManager().GetPosByBar(bar) - pos;
		measurepos *= speed;
		if (measurepos > 1) break;
		RenderLine(measurepos);
		bar += bms->GetBarManager()[m];
	}
}

void ActorBeatLine::Update() {
	// copy tween information
	ActorSprite::Update();
	m_Tweenobj = m_Tweeninfo;
}






ActorJudgeLine::ActorJudgeLine() : ActorSprite() {}







#pragma region BGAOBJECT
ActorBga::ActorBga() : Actor(ACTORTYPE::BGA) {
	// get black tex
	m_TexBlack = TEXPOOL->Get("_black");
}

void ActorBga::SetFromXml(const XMLElement *e) {
	Actor::SetFromXml(e);
	side = e->IntAttribute("side");
	miss = PLAYERVALUE[side].pOnMiss;
}

void ActorBga::RenderBGA(Display::Texture *tex, bool transparent) {
	if (!tex) return;
	DISPLAY->SetDST(&m_Tweeninfo.state.dst);
	// if not transparent, then render black background once
	// TODO: draw target by keeping aspect ratio
	if (!transparent) {
		DISPLAY->SetTexture(m_TexBlack);
		DISPLAY->SetSRC(0);
		DISPLAY->DrawPrimitives();
	}
	DISPLAY->SetTexture(tex);
	DISPLAY->SetSRC(0);
	DISPLAY->DrawPrimitives();
}

void ActorBga::Update() { Actor::Update(); }

void ActorBga::Render() {
	if (!drawable) return;
	/*
	 * BGA is common resource so get it from BmsResource, it's easy.
	 * but one thing should be taken care of - 
	 * Miss timer is different from player.
	 */
	RenderBGA(SONGPLAYER->GetMainBGA(), false);
	RenderBGA(SONGPLAYER->GetLayer1BGA());
	RenderBGA(SONGPLAYER->GetLayer2BGA());
	if (miss->IsStarted() && miss->GetTick() < 1000)
		RenderBGA(SONGPLAYER->GetMissBGA(), false);
}
#pragma endregion BGAOBJECT


REGISTER_ACTOR_WITH_NAME("Judge", ActorJudge);
REGISTER_ACTOR_WITH_NAME("Bga", ActorBga);