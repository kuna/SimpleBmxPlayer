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

#pragma region SKINRENDEROBJECT
SkinRenderObject::SkinRenderObject(SkinRenderTree* owner, int type)
	: rtree(owner), dstcnt(0), tag(0), drawable(false),
	clickable(false), focusable(false), invertcondition(false), objtype(type) { }

int SkinRenderObject::GetType() { return objtype; }

void SkinRenderObject::Clear() {
	dstcnt = 0;
}

void SkinRenderObject::AddDST(ImageDST &dst, const RString &condition, bool lua) {
	this->dst[dstcnt] = dst;
	if (!lua)
		this->dst_condition[dstcnt].Set(condition);
	else
		this->dst_condition[dstcnt].SetLuacondition(condition);
	dstcnt++;
}

void SkinRenderObject::InvertCondition(bool b) { invertcondition = b; }

bool SkinRenderObject::EvaluateCondition() {
	if (invertcondition)
		return !condition.Evaluate();
	else
		return condition.Evaluate();
}

bool SkinRenderObject::IsGroup() {
	switch (objtype) {
	case ACTORTYPE::GROUP:
		return true;
	}
	return false;
}

bool SkinRenderObject::IsGeneral() {
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

SkinGroupObject* SkinRenderObject::ToGroup() {
	if (objtype == ACTORTYPE::GROUP)
		return (SkinGroupObject*)this;
	else
		return 0;
}

SkinNoteFieldObject* SkinRenderObject::ToNoteFieldObject() {
	if (objtype == ACTORTYPE::PLAYLANE)
		return (SkinNoteFieldObject*)this;
	else
		return 0;
}

// do nothing
void SkinRenderObject::Render() {  }

void SkinRenderObject::UpdateBasic() {
	// evaluate
	drawable = condition.Evaluate();
	if (invertcondition) drawable = !drawable;
	if (!drawable) return;
	// fill DST
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
	else {
		drawable = false;
	}
}

void SkinRenderObject::Update() {
	UpdateBasic();
}

bool SkinRenderObject::Click(int x, int y) {
	// must call after Update()
	if (drawable && clickable) {
		return false;
	}
	else {
		return false;
	}
}

bool SkinRenderObject::Hover(int x, int y) {
	// must call after Update()
	if (drawable && focusable) {
		return false;
	}
	else {
		return false;
	}
}

int SkinRenderObject::GetWidth() {
	return dst_cached.frame.w;
}

int SkinRenderObject::GetHeight() {
	return dst_cached.frame.h;
}

int SkinRenderObject::GetX() {
	return dst_cached.frame.x;
}

int SkinRenderObject::GetY() {
	return dst_cached.frame.y;
}

void SkinRenderObject::SetBasicObject(XMLElement *e) {
	// parse common attribute: DST, condition
	SkinRenderHelper::ConstructBasicRenderObject(this, e);
}

void SkinRenderObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
}

void SkinRenderObject::SetCondition(const RString &str) { condition.Set(str); }

#pragma endregion SKINRENDEROBJECT







SkinUnknownObject::SkinUnknownObject(SkinRenderTree* owner) : SkinRenderObject(owner, ACTORTYPE::UNKNOWN) {}







#pragma region SKINGROUPOBJECT
SkinGroupObject::SkinGroupObject(SkinRenderTree* owner)
	: SkinRenderObject(owner, ACTORTYPE::GROUP)
{
}

SkinGroupObject::~SkinGroupObject() {
}

void SkinGroupObject::UpdateChilds() {
	// after pushing offset
	// update all child object's properties
	// (recursively)
	rtree->PushRenderOffset(dst_cached.frame.x, dst_cached.frame.y);
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Update();
	}
	rtree->PopRenderOffset();
}

void SkinGroupObject::SetObject(XMLElement *e) {
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

void SkinGroupObject::Update() {
	UpdateBasic();
	if (drawable) UpdateChilds();
}

void SkinGroupObject::Render() {
	// TODO: use SDL_RenderSetViewport
	// recursively renders child element
	if (!drawable) return;
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Render();
	}
}

void SkinGroupObject::AddChild(SkinRenderObject *obj) {
	_childs.push_back(obj);
}

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::begin() { return _childs.begin(); }

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::end() { return _childs.end(); }






SkinCanvasObject::SkinCanvasObject(SkinRenderTree *owner)
	: SkinGroupObject(owner) {
	t = SDL_CreateTexture(Game::RENDERER,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, rtree->GetWidth(), rtree->GetHeight());
}

SkinCanvasObject::~SkinCanvasObject() { if (t) SDL_DestroyTexture(t); }

void SkinCanvasObject::SetAsRenderTarget() {
	if (!t) return;
	_org = SDL_GetRenderTarget(Game::RENDERER);
	SDL_SetRenderTarget(Game::RENDERER, t);
}

void SkinCanvasObject::ResetRenderTarget() {
	if (!t) return;
	SDL_SetRenderTarget(Game::RENDERER, _org);
}

void SkinCanvasObject::SetObject(XMLElement *e) {
	SetBasicObject(e);
	// set SRC
	SkinRenderHelper::ConstructSRCFromElement(src, e);
}

void SkinCanvasObject::Render() {
	// same method like ImageObject
	if (!t) return;
	if (!drawable) return;
	// first draw childs recursively
	SetAsRenderTarget();
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Render();
	}
	ResetRenderTarget();
	// then draw cached texture like image object
	SkinRenderHelper::Render(t, &src, &dst_cached.frame,
		dst_cached.dst->blend, dst_cached.dst->rotatecenter);
}

#pragma endregion SKINGROUPOBJECT







SkinImageObject::SkinImageObject(SkinRenderTree* owner, int type) : SkinRenderObject(owner, type) {
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
	: SkinRenderObject(owner, ACTORTYPE::TEXT), fnt(0), v(0), align(0), editable(false) {}

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
		if (n < 0) {
			char t1[] = "0123456789";
			char t2[] = "ABCDEFGHIJ";
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
	: SkinRenderObject(t, ACTORTYPE::SCRIPT), runoneveryframe(false), runtime(0) {}

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
