#include "ActorBasic.h"
#include "global.h"
#include "globalresources.h"
#include "util.h"
#include "luamanager.h"
#include "game.h"
#include "file.h"
#include "logger.h"

// for rtree back-reference
#include "ActorRenderer.h"

// ------ SkinRenderCondition ----------------------

RenderCondition::RenderCondition()
	: condcnt(0), evaluate_count(0)
{
	memset(not, 0, sizeof(not));
	memset(cond, 0, sizeof(cond));
}

void RenderCondition::Set(const RString &condition) {
	std::vector<RString> conditions;
	split(condition, ",", conditions);
	for (auto it = conditions.begin(); it != conditions.end(); ++it) {
		Trim(*it);
		key[condcnt] = *it;
		if (key[condcnt][0] == '!') {
			not[condcnt] = true;
			key[condcnt] = key[condcnt].substr(1);
		}
		else {
			not[condcnt] = false;
		}
		/*
		* Create timer although it's not created yet.
		* - and stop them all if they're activated.
		* This will be better then using Get() method.
		*/
		cond[condcnt] = TIMERPOOL->Get(key[condcnt]);
		if (atoi(key[condcnt]) >= 900 && cond[condcnt]->IsUnknown())
			cond[condcnt]->Stop();
		condcnt++;
	}
}

void RenderCondition::SetLuacondition(const RString &condition) {
	luacondition = condition;
}

bool RenderCondition::Evaluate() {
	evaluate_count++;
	if (luacondition.size()) {
		// getting Lua object doesn't cost much; but evaluating does.
		// aware not to use lua conditional script.
		Lua* l = LUA->Get();
		RString e;
		LuaHelpers::RunScript(l, luacondition, "Skin_RenderCondition", e, 0, 1);
		bool r = (bool)lua_toboolean(l, -1);
		lua_pop(l, 1);
		LUA->Release(l);
		return r;
	}
	for (int i = 0; i < condcnt; i++) {
		// if condition is empty, then attempt to find once more
		// this causes a lot frame drop, so we do finding only 10 times
		//if (!cond[i] && evaluate_count < 10) cond[i] = TIMERPOOL->Get(key[i]);
		if (cond[i] == 0
			|| cond[i]->IsUnknown()
			|| (not[i] && cond[i]->IsStarted())
			|| (!not[i] && cond[i]->IsStopped())) {
			return false;
		}
	}
	return true;
}







// ------ Skin General rendering Objects -----------------

#pragma region ACTOR
Actor::Actor(SkinRenderTree* owner, int type) : objtype(type) { Clear(); }

int Actor::GetType() { return objtype; }

void Actor::Clear() {
	drawable = false;
	clickable = false;
	focusable = false;
}

bool Actor::IsGroup() {
	switch (objtype) {
	case ACTORTYPE::GROUP:
		return true;
	}
	return false;
}

bool Actor::IsGeneral() {
	switch (objtype) {
	case ACTORTYPE::GENERAL:
	case ACTORTYPE::IMAGE:
	case ACTORTYPE::NUMBER:
	case ACTORTYPE::GRAPH:
	case ACTORTYPE::SLIDER:
	case ACTORTYPE::TEXT:
	case ACTORTYPE::BUTTON:
	case ACTORTYPE::SCRIPT:
		return true;
	}
	return false;
}

ActorFrame* Actor::ToGroup() {
	if (objtype == ACTORTYPE::GROUP)
		return (ActorFrame*)this;
	else
		return 0;
}

SkinNoteFieldObject* Actor::ToNoteFieldObject() {
	if (objtype == ACTORTYPE::PLAYLANE)
		return (SkinNoteFieldObject*)this;
	else
		return 0;
}

// do nothing
void Actor::Render() {  }

void Actor::Update() {
	// evaluate condition
	if (!(drawable = m_Condition.Evaluate()))
		return;

	// Calculate Tween State
	Uint32 tick = m_Dst.timer.GetTick();
	while (tweenidx < m_Dst.tweens.size()) {
		if (m_Dst)
			tweenidx++;
	}
	dst_cached.dst = 0;
	for (int j = 0; j < dstcnt; j++) {
		if (dst_condition[j].Evaluate()) {
			dst_cached.dst = &dst[j];
			break;
		}
	}
	if (dst_cached.dst) {
		drawable = SkinRenderHelper::CalculateFrame(*dst_cached.dst, dst_cached.frame);
		if (drawable) {
			// take offset
			dst_cached.frame.x += rtree->GetOffsetX();
			dst_cached.frame.y += rtree->GetOffsetY();
		}
	}

	// check alpha(color) mod
	if (00) {
		drawable = false;
		return;
	}
}

bool Actor::Click(int x, int y) {
	// must call after Update()
	if (drawable && clickable) {
		return false;
	}
	else {
		return false;
	}
}

bool Actor::Hover(int x, int y) {
	// must call after Update()
	if (drawable && focusable) {
		return false;
	}
	else {
		return false;
	}
}

int Actor::GetWidth() {
	return m_Tweeninfo.state.dst.w;
}

int Actor::GetHeight() {
	return m_Tweeninfo.state.dst.h;
}

int Actor::GetX() {
	return m_Tweeninfo.state.dst.x;
}

int Actor::GetY() {
	return m_Tweeninfo.state.dst.y;
}

void Actor::LoadFromXML(XMLElement *e) {
	// let's load all attributes~~ swing ~~


}

void Actor::SetCondition(const RString &str) { condition.Set(str); }

#pragma endregion







#pragma region SKINGROUPOBJECT
ActorFrame::ActorFrame(SkinRenderTree* owner)
	: Actor(owner, ACTORTYPE::GROUP)
{
}

ActorFrame::~ActorFrame() {
}

void ActorFrame::UpdateChilds() {
	// after pushing offset
	// update all child object's properties
	// (recursively)
	rtree->PushRenderOffset(dst_cached.frame.x, dst_cached.frame.y);
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Update();
	}
	rtree->PopRenderOffset();
}

void ActorFrame::SetObject(XMLElement *e) {
	SetBasicObject(e);
	// if dst count = 0, then add basic dst
	if (dstcnt == 0) {
		ImageDSTFrame f = { 0, 0, 0, 0, 0, 255, 255, 255, 255, 0 };
		std::vector<ImageDSTFrame> vf;
		vf.push_back(f);
		ImageDST dst = { 0, 0, 0, 0, 0, 0, vf };
		AddDST(dst);
	}
}

void ActorFrame::Update() {
	UpdateBasic();
	if (drawable) UpdateChilds();
}

void ActorFrame::Render() {
	// TODO: use SDL_RenderSetViewport
	// recursively renders child element
	if (!drawable) return;
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Render();
	}
}

void ActorFrame::AddChild(Actor *obj) {
	_childs.push_back(obj);
}

std::vector<Actor*>::iterator
ActorFrame::begin() { return _childs.begin(); }

std::vector<Actor*>::iterator
ActorFrame::end() { return _childs.end(); }

#pragma endregion 







SkinImageObject::SkinImageObject(SkinRenderTree* owner, int type) : Actor(owner, type) {
	imgsrc = { 0, 0, 0, 0 };
	img = 0;
}

void SkinImageObject::SetImage(Image *img) {
	this->img = img;
}

void SkinImageObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	SetImageObject(e);
}

//
// for use of extended class
//
void SkinImageObject::SetImageObject(XMLElement *e) {
	SkinRenderHelper::ConstructSRCFromElement(imgsrc, e);
	if (e->Attribute("resid")) {
		Image *img = IMAGEPOOL->GetById(e->Attribute("resid"));
		if (!img) img = rtree->_imagekey[e->Attribute("resid")];
		SetImage(img);
	}
}

void SkinImageObject::Update() {
	UpdateBasic();
	// if image is not loop && tick overtime end then don't draw
	drawable &= imgsrc.loop || (!imgsrc.loop && imgsrc.timer &&
		imgsrc.timer->GetTick() < imgsrc.cycle);
}

void SkinImageObject::Render() {
	if (drawable && img)
		SkinRenderHelper::Render(img, &imgsrc, &dst_cached.frame,
		dst_cached.dst->blend, dst_cached.dst->rotatecenter);
}







SkinTextObject::SkinTextObject(SkinRenderTree* owner)
	: Actor(owner, ACTORTYPE::TEXT), fnt(0), v(0), align(0), editable(false) {}

void SkinTextObject::SetFont(const char* resid) {
	if (!resid || rtree->_fontkey.find(resid) == rtree->_fontkey.end()) {
		// use system font (fallback)
		// TODO: set to 0?
		fnt = FONTPOOL->GetByID("_system");
	}
	else {
		fnt = rtree->_fontkey[resid];
	}
}

void SkinTextObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	if (e->Attribute("value"))
		SetValue(STRPOOL->Get(e->Attribute("value")));
	SetAlign(e->IntAttribute("align"));
	SetFont(e->Attribute("resid"));
}

void SkinTextObject::SetValue(RString* s) { v = s; }

void SkinTextObject::SetEditable(bool editable) { this->editable = editable; }

void SkinTextObject::SetAlign(int align) { this->align = align; }

int SkinTextObject::GetWidth() { if (v) return GetTextWidth(*v); else return 0; }

int SkinTextObject::GetTextWidth(const RString& s) {
	if (fnt) {
		return fnt->GetWidth(s);
	}
	else {
		return 0;
	}
}

void SkinTextObject::Render() { if (v && drawable) RenderText(*v); }

void SkinTextObject::RenderText(const char* s) {
	if (fnt && drawable) {
		// check align
		int left_offset = 0;
		switch (align) {
		case 1:		// center
			left_offset = -(fnt->GetWidth(s)) / 2;
			break;
		case 2:		// right
			left_offset = -fnt->GetWidth(s);
			break;
		}
		fnt->SetAlphaMod(dst_cached.frame.a);
		fnt->SetColorMod(dst_cached.frame.r, dst_cached.frame.g, dst_cached.frame.b);
		fnt->Render(s,
			left_offset + rtree->GetOffsetX() + dst_cached.frame.x,
			rtree->GetOffsetY() + dst_cached.frame.y);
	}
}







SkinNumberObject::SkinNumberObject(SkinRenderTree* owner)
	: SkinTextObject(owner), v(0) {
	objtype = ACTORTYPE::NUMBER;
}

void SkinNumberObject::SetValue(int *i) { v = i; }

void SkinNumberObject::SetLength(int length) { this->length = length; }

int SkinNumberObject::GetWidth() {
	if (v && drawable) {
		CacheInt(*v);
		return GetTextWidth(buf);
	}
	else return 0;
}

void SkinNumberObject::Set24Mode(bool b) { mode24 = b; }

void SkinNumberObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	if (e->Attribute("value"))
		SetValue(INTPOOL->Get(e->Attribute("value")));
	SetAlign(e->IntAttribute("align"));
	SetLength(e->IntAttribute("length"));
	Set24Mode(e->IntAttribute("mode24"));
	SetFont(e->Attribute("resid"));
}

void SkinNumberObject::Render() {
	if (v && drawable) {
		CacheInt(*v);
		RenderText(buf);
	}
}

void SkinNumberObject::CacheInt(int n) {
	bool negative = n < 0;
	n = abs(n);
	if (!length) {
		itoa(n, buf, 10);
	}
	else {
		buf[length] = 0;
		for (int i = length - 1; i >= 0; i--) {
			if (n == 0 && i < length - 1) {
				buf[i] = '*';
			}
			else {
				buf[i] = '0' + n % 10;
			}
			n /= 10;
		}
	}
	if (mode24) {
		if (negative) {
			char t1[] = "0123456789*";
			char t2[] = "ABCDEFGHIJ#";
			for (int i = 0; i < strlen(buf); i++) {
				for (int j = 0; j < strlen(t1); j++) {
					if (buf[i] == t1[j]) {
						buf[i] = t2[j];
						break;
					}
				}
			}
			for (int i = strlen(buf); i >= 0; i--) {
				buf[i + 1] = buf[i];
			}
			buf[0] = '-';
		}
		else {
			for (int i = strlen(buf); i >= 0; i--) {
				buf[i + 1] = buf[i];
			}
			buf[0] = '+';
		}
	}
}







SkinGraphObject::SkinGraphObject(SkinRenderTree* owner) : SkinImageObject(owner, ACTORTYPE::GRAPH) {}

void SkinGraphObject::SetValue(double *d) { v = d; }

void SkinGraphObject::SetType(int type) { this->type = type; }

void SkinGraphObject::SetDirection(int direction) { this->direction = direction; }

void SkinGraphObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	SetImageObject(e);
	SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
}

void SkinGraphObject::Render() {
	if (drawable && img) {
		ImageDSTFrame f = dst_cached.frame;
		f.h *= *v;
		SkinRenderHelper::Render(img, &imgsrc, &f,
			dst_cached.dst->blend, dst_cached.dst->rotatecenter);
	}
}







SkinSliderObject::SkinSliderObject(SkinRenderTree* owner)
	: SkinImageObject(owner, ACTORTYPE::SLIDER), range(0), direction(0) {}

void SkinSliderObject::SetValue(double *d) { v = d; }

void SkinSliderObject::SetRange(int range) { this->range = range; }

void SkinSliderObject::SetDirection(int directio) { this->direction = directio; }

void SkinSliderObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	SetImageObject(e);
	SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
	SetRange(e->IntAttribute("range"));
	SetDirection(e->IntAttribute("direction"));
}

void SkinSliderObject::Render() {
	if (drawable && img) {
		ImageDSTFrame f = dst_cached.frame;
		f.y += *v * range;
		SkinRenderHelper::Render(img, &imgsrc, &f,
			dst_cached.dst->blend, dst_cached.dst->rotatecenter);
	}

}







SkinButtonObject::SkinButtonObject(SkinRenderTree* owner) : SkinImageObject(owner, ACTORTYPE::BUTTON) {}







#pragma region SCRIPTOBJECT
SkinScriptObject::SkinScriptObject(SkinRenderTree *t)
	: Actor(t, ACTORTYPE::SCRIPT), runoneveryframe(false), runtime(0) {}

void SkinScriptObject::SetObject(XMLElement *e) {
	if (e->Attribute("OnRender"))
		SetRunCondition(true);
	if (e->Attribute("src"))
		LoadFile(e->Attribute("src"));
	else if (e->GetText())
		SetScript(e->GetText());
}

void SkinScriptObject::SetRunCondition(bool oneveryframe) {
	runoneveryframe = oneveryframe;
}

void SkinScriptObject::LoadFile(const RString &filepath) {
	RString _filepath = filepath;
	FileHelper::ConvertPathToAbsolute(_filepath);
	RString out;
	if (GetFileContents(_filepath, out)) {
		script = out;
	}
	else {
		LOG->Warn("Failed to load Lua Script(%s)", filepath);
	}
}

void SkinScriptObject::SetScript(const RString &script_) {
	script = script_;
}

void SkinScriptObject::Update() {
	if (runtime && !runoneveryframe) return;

	Lua *l = LUA->Get();
	RString err;
	if (!LuaHelpers::RunScript(l, script, "Skin_Script", err)) {
		LOG->Warn("Error occured during Lua Script - %s", err.c_str());
	}
	LUA->Release(l);

	runtime++;
}

void SkinScriptObject::Render() {
	// do nothing on rendering
}

#pragma endregion SCRIPTOBJECT
