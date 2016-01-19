
#include "game.h"
#include "skinrendertree.h"
#include "globalresources.h"	// for evaluation condition
#include "luamanager.h"
#include "util.h"
#include "file.h"
#include "logger.h"
#include "tinyxml2.h"
using namespace tinyxml2;

// redefinition;
void ConstructSRCFromElement(ImageSRC &, XMLElement *);
void ConstructDSTFromElement(ImageDST &, XMLElement *);
namespace SkinRenderHelper {
	struct _Offset {
		int x, y;
	};
	std::vector<_Offset> _offset_stack;
	_Offset _offset_calculated = { 0, 0 };
	void RecalculateOffset() {
		_offset_calculated = { 0, 0 };
		for (auto it = _offset_stack.begin(); it != _offset_stack.end(); ++it) {
			_offset_calculated.x += it->x;
			_offset_calculated.y += it->y;
		}
	}
}

// ------ SkinRenderCondition ----------------------

RenderCondition::RenderCondition() : condcnt(0), evaluate_count(0) {
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

SkinRenderObject::SkinRenderObject(SkinRenderTree* owner, int type):
rtree(owner), dstcnt(0), tag(0), clickable(false), focusable(false), objtype(type) { }

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
	return !invertcondition ^ condition.Evaluate();
}

bool SkinRenderObject::IsGroup() {
	switch (objtype) {
	case OBJTYPE::GROUP:
		return true;
	}
	return false;
}

bool SkinRenderObject::IsGeneral() {
	switch (objtype) {
	case OBJTYPE::IMAGE:
	case OBJTYPE::NUMBER:
	case OBJTYPE::GRAPH:
	case OBJTYPE::SLIDER:
	case OBJTYPE::TEXT:
	case OBJTYPE::BUTTON:
	case OBJTYPE::SCRIPT:
		return true;
	}
	return false;
}

SkinUnknownObject* SkinRenderObject::ToUnknown() {
	if (objtype == OBJTYPE::UNKNOWN)
		return (SkinUnknownObject*)this;
	else
		return 0;
}

SkinGroupObject* SkinRenderObject::ToGroup() {
	if (objtype == OBJTYPE::GROUP)
		return (SkinGroupObject*)this;
	else
		return 0;
}

SkinImageObject* SkinRenderObject::ToImage() {
	if (objtype == OBJTYPE::IMAGE)
		return (SkinImageObject*)this;
	else
		return 0;
}

SkinBgaObject* SkinRenderObject::ToBGA() {
	if (objtype == OBJTYPE::BGA)
		return (SkinBgaObject*)this;
	else
		return 0;
}

SkinPlayObject* SkinRenderObject::ToPlayObject() {
	if (objtype == OBJTYPE::PLAYLANE)
		return (SkinPlayObject*)this;
	else
		return 0;
}

// do nothing
void SkinRenderObject::Render() {  }

void SkinRenderObject::Update() {
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
	}
	else {
		drawable = false;
	}
}

bool SkinRenderObject::Click(int x, int y) {
	// must call after Update()
	if (drawable && clickable) {

	}
	else {
		return false;
	}
}

bool SkinRenderObject::Hover(int x, int y) {
	// must call after Update()
	if (drawable && focusable) {

	}
	else {
		return false;
	}
}

void SkinRenderObject::SetCondition(const RString &str) { condition.Set(str); }

SkinUnknownObject::SkinUnknownObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::UNKNOWN) {}

#pragma region SKINGROUPOBJECT
SkinGroupObject::SkinGroupObject(SkinRenderTree* owner, bool createTex)
	: SkinImageObject(owner, OBJTYPE::GROUP) 
{
	if (createTex) {
		t = rtree->GenerateTexture();
	}
	else {
		t = NULL;
	}
}

void SkinGroupObject::SetAsRenderTarget() {
	if (t) {
		_org = SDL_GetRenderTarget(Game::RENDERER);
		SDL_SetRenderTarget(Game::RENDERER, t);
	}
	/*
	 * Offset: current DST's animated value
	 */
	ImageDST* _dst = 0;
	for (int j = 0; j < dstcnt; j++) {
		if (dst_condition[j].Evaluate()) {
			_dst = &dst[j];
			break;
		}
	}
	ImageDSTFrame frame;
	if (!_dst || !SkinRenderHelper::CalculateFrame(*_dst, frame)) {
		SkinRenderHelper::PushRenderOffset(0, 0);
	}
	else {
		SkinRenderHelper::PushRenderOffset(frame.x, frame.y);
	}
}

void SkinGroupObject::ResetRenderTarget() {
	if (t) {
		SDL_SetRenderTarget(Game::RENDERER, _org);
	}
	SkinRenderHelper::PopRenderOffset();
}

void SkinGroupObject::Render() {
	if (t) {
		/*
		 * SRC & DST are same; 
		 */
		ImageDST* _dst = 0;
		for (int j = 0; j < dstcnt; j++) {
			if (dst_condition[j].Evaluate()) {
				_dst = &dst[j];
				break;
			}
		}
		ImageDSTFrame frame;
		if (!_dst || !SkinRenderHelper::CalculateFrame(*_dst, frame))
			return;
		if (imgsrc.w <= 0) imgsrc.w = rtree->GetWidth();
		if (imgsrc.h <= 0) imgsrc.h = rtree->GetHeight();
		SDL_Rect src = SkinRenderHelper::ToRect(imgsrc);
		SDL_Rect r = SkinRenderHelper::ToRect(frame);
		SDL_RenderCopy(Game::RENDERER, t, &src, &r);
	}
}

SkinGroupObject::~SkinGroupObject() {
	if (t) {
		SDL_DestroyTexture(t);
		t = 0;
	}
}

void SkinGroupObject::AddChild(SkinRenderObject *obj) {
	_childs.push_back(obj);
}

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::begin() { return _childs.begin(); }

std::vector<SkinRenderObject*>::iterator
SkinGroupObject::end() { return _childs.end(); }
#pragma endregion SKINGROUPOBJECT

SkinImageObject::SkinImageObject(SkinRenderTree* owner, int type) : SkinRenderObject(owner, type) {
	imgsrc = { 0, 0, 0, 0 };
	img = 0;
}

void SkinImageObject::SetImage(Image *img) {
	this->img = img;
}

void SkinImageObject::SetSRC(XMLElement *e) {
	ConstructSRCFromElement(imgsrc, e);
	if (e->Attribute("resid"))
		SetImage(rtree->_imagekey[e->Attribute("resid")]);
}

void SkinImageObject::Render() {
	if (drawable && img)
		SkinRenderHelper::Render(img, &imgsrc, &dst_cached.frame, 
		dst_cached.dst->blend, dst_cached.dst->rotatecenter);
}

SkinNumberObject::SkinNumberObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::NUMBER) {}

SkinTextObject::SkinTextObject(SkinRenderTree* owner) : SkinRenderObject(owner, OBJTYPE::TEXT) {}

SkinGraphObject::SkinGraphObject(SkinRenderTree* owner) : SkinImageObject(owner, OBJTYPE::GRAPH) {}

void SkinGraphObject::SetValue(double *d) { v = d; }

void SkinGraphObject::SetType(int type) { this->type = type;  }

void SkinGraphObject::SetDirection(int direction) { this->direction = direction; }

void SkinGraphObject::Render() {
	if (drawable && img) {
		ImageDSTFrame f = dst_cached.frame;
		f.h *= *v;
		SkinRenderHelper::Render(img, &imgsrc, &f,
			dst_cached.dst->blend, dst_cached.dst->rotatecenter);
	}
}

SkinSliderObject::SkinSliderObject(SkinRenderTree* owner)
	: SkinImageObject(owner, OBJTYPE::SLIDER), range(0), direction(0) {}

void SkinSliderObject::SetValue(double *d) { v = d; }

void SkinSliderObject::SetRange(int range) { this->range = range; }

void SkinSliderObject::SetDirection(int directio) { this->direction = directio; }

void SkinSliderObject::Render() {
	if (drawable && img) {
		ImageDSTFrame f = dst_cached.frame;
		f.y += *v * range;
		SkinRenderHelper::Render(img, &imgsrc, &f,
			dst_cached.dst->blend, dst_cached.dst->rotatecenter);
	}

}

SkinButtonObject::SkinButtonObject(SkinRenderTree* owner) : SkinImageObject(owner, OBJTYPE::BUTTON) {}

#pragma region PLAYOBJECT
SkinPlayObject::SkinPlayObject(SkinRenderTree* owner) : 
SkinGroupObject(owner), imgobj_judgeline(0), imgobj_line(0) {
	objtype = OBJTYPE::PLAYLANE;
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
}

void SkinPlayObject::SetLineObject(XMLElement *lineobj) {
	if (!lineobj) return;
	if (!imgobj_line)
		imgobj_line = rtree->NewImageObject();
	SkinRenderHelper::ConstructBasicRenderObject(imgobj_line, lineobj);
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
#pragma endregion PLAYOBJECT

#pragma region BGAOBJECT
SkinBgaObject::SkinBgaObject(SkinRenderTree *t) : SkinRenderObject(t, OBJTYPE::BGA) {}

void SkinBgaObject::RenderBGA(Image *bga) {
	if (!bga) return;
	if (!drawable) return;
	ImageSRC src = { 0, 0, 0, 0, 0, 1, 1, 0, 0 };
	SkinRenderHelper::Render(bga, &src, &dst_cached.frame, 0, 5);
}
#pragma endregion BGAOBJECT

#pragma region SCRIPTOBJECT
SkinScriptObject::SkinScriptObject(SkinRenderTree *t)
	: SkinRenderObject(t, OBJTYPE::SCRIPT), runoneveryframe(false), runtime(0) {}

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

void SkinScriptObject::Render() {
	if (runtime && !runoneveryframe) return;

	Lua *l = LUA->Get();
	RString err;
	if (!LuaHelpers::RunScript(l, script, "Skin_Script", err)) {
		LOG->Warn("Error occured during Lua Script - %s", err.c_str());
	}
	LUA->Release(l);

	runtime++;
}
#pragma endregion SCRIPTOBJECT

// ----- SkinRenderTree -------------------------------------

#pragma region SKINRENDERTREE
SkinRenderTree::SkinRenderTree(int w, int h) : SkinGroupObject(this) { SetSkinSize(w, h); }

void SkinRenderTree::SetSkinSize(int w, int h) { _scr_w = w, _scr_h = h; }

SkinRenderTree::~SkinRenderTree() {
	ReleaseAll();
	ReleaseAllResources();
}

void SkinRenderTree::ReleaseAll() {
	for (auto it = _objpool.begin(); it != _objpool.end(); ++it) {
		delete *it;
	}
	_objpool.clear();
	_idpool.clear();
}

void SkinRenderTree::RegisterImage(RString &id, RString &path) {
	Image *img = IMAGEPOOL->Load(path);
	if (img) {
		_imagekey.insert(std::pair<RString, Image*>(id, img));
	}
}

void SkinRenderTree::ReleaseAllResources() {
	for (auto it = _imagekey.begin(); it != _imagekey.end(); ++it)
		IMAGEPOOL->Release(it->second);
	_imagekey.clear();
}

SDL_Texture* SkinRenderTree::GenerateTexture() {
	return SDL_CreateTexture(Game::RENDERER,
		SDL_PIXELFORMAT_RGBA8888,
		SDL_TEXTUREACCESS_TARGET, _scr_w, _scr_h);
}

int SkinRenderTree::GetWidth() { return _scr_w; }

int SkinRenderTree::GetHeight() { return _scr_h; }

SkinUnknownObject* SkinRenderTree::NewUnknownObject() {
	SkinUnknownObject* obj = new SkinUnknownObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGroupObject* SkinRenderTree::NewGroupObject(bool clipping) {
	SkinGroupObject* obj = new SkinGroupObject(this, clipping);
	_objpool.push_back(obj);
	return obj;
}

SkinImageObject* SkinRenderTree::NewImageObject() {
	SkinImageObject* obj = new SkinImageObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinSliderObject* SkinRenderTree::NewSliderObject() {
	SkinSliderObject* obj = new SkinSliderObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinGraphObject* SkinRenderTree::NewGraphObject() {
	SkinGraphObject* obj = new SkinGraphObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinTextObject* SkinRenderTree::NewTextObject() {
	SkinTextObject* obj = new SkinTextObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinNumberObject* SkinRenderTree::NewNumberObject() {
	SkinNumberObject* obj = new SkinNumberObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinPlayObject* SkinRenderTree::NewPlayObject() {
	SkinPlayObject* obj = new SkinPlayObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinBgaObject* SkinRenderTree::NewBgaObject() {
	SkinBgaObject* obj = new SkinBgaObject(this);
	_objpool.push_back(obj);
	return obj;
}

SkinScriptObject* SkinRenderTree::NewScriptObject() {
	SkinScriptObject* obj = new SkinScriptObject(this);
	_objpool.push_back(obj);
	return obj;
}
#pragma endregion SKINRENDERTREE

// ------ SkinRenderHelper -----------------------------

#pragma region SKINRENDERHELPER

#define ISNAME(e, s) (strcmp(e->Name(), s) == 0)

/* private */
void ConstructSRCFromElement(ImageSRC &src, XMLElement *e) {
	src.x = e->IntAttribute("x");
	src.y = e->IntAttribute("y");
	src.w = e->IntAttribute("w");
	src.h = e->IntAttribute("h");
	src.timer = TIMERPOOL->Get("OnScene");
	const char* timer_name = e->Attribute("timer");
	if (timer_name)
		src.timer = TIMERPOOL->Get(timer_name);
	src.cycle = e->IntAttribute("cycle");
	src.divx = e->IntAttribute("divx");
	if (src.divx < 1) src.divx = 1;
	src.divy = e->IntAttribute("divy");
	if (src.divy < 1) src.divy = 1;
	src.loop = 1;
	if (e->Attribute("loop"))
		src.loop = e->IntAttribute("loop");
}

/* private */
void ConstructDSTFromElement(ImageDST &dst, XMLElement *e) {
	const char* timer = e->Attribute("timer");
	if (timer) {
		dst.timer = TIMERPOOL->Get(timer);
	}
	else {
		dst.timer = TIMERPOOL->Get("OnScene");		// very basic timer, if none setted.
	}
	dst.blend = e->IntAttribute("blend");
	dst.rotatecenter = e->IntAttribute("rotatecenter");
	dst.acctype = e->IntAttribute("acc");
	dst.loopstart = 0;
	XMLElement *ef = e->FirstChildElement("Frame");
	while (ef) {
		ImageDSTFrame f;
		f.time = ef->IntAttribute("time");
		f.x = ef->IntAttribute("x");
		f.y = ef->IntAttribute("y");
		f.w = ef->IntAttribute("w");
		f.h = ef->IntAttribute("h");
		f.a = 255;	ef->QueryIntAttribute("a", &f.a);
		f.r = 255;	ef->QueryIntAttribute("r", &f.r);
		f.g = 255;	ef->QueryIntAttribute("g", &f.g);
		f.b = 255;	ef->QueryIntAttribute("b", &f.b);
		f.angle = ef->IntAttribute("angle");
		if (ef->Attribute("loopstart")) {
			dst.loopstart = f.time;
		}
		dst.loopend = f.time;
		SkinRenderHelper::AddFrame(dst, f);
		ef = ef->NextSiblingElement("Frame");
	}
	
	// CASE: no loop (div by 0 error)
	if (dst.loopstart == dst.loopend)
		dst.loopstart = 0;
}

/* private */
void ConstructTreeFromElement(SkinRenderTree &rtree, SkinGroupObject *group, XMLElement *e) {
	while (e) {
		SkinRenderObject *obj = 0;

		if (ISNAME(e, "Init")) {
			/*
			 * Init object is only called once;
			 * it doesn't added to render tree if it's conditions is bad
			 * (TODO)
			 */
		}
		else if (ISNAME(e, "If")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Ifnot")) {
			SkinGroupObject *g = rtree.NewGroupObject();
			g->InvertCondition(true);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Group")) {
			SkinGroupObject *g = rtree.NewGroupObject(true);
			g->SetSRC(e);
			ConstructTreeFromElement(rtree, g, e->FirstChildElement());
			obj = g;
		}
		else if (ISNAME(e, "Image")) {
			SkinImageObject *img = rtree.NewImageObject();
			img->SetSRC(e);
			obj = img;
		}
		else if (ISNAME(e, "Slider")) {
			if (e->Attribute("value")) {
				SkinSliderObject *slider = rtree.NewSliderObject();
				slider->SetSRC(e);
				slider->SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
				slider->SetRange(e->IntAttribute("range"));
				slider->SetDirection(e->IntAttribute("direction"));
				obj = slider;
			}
		}
		else if (ISNAME(e, "Graph")) {
			if (e->Attribute("value")) {
				SkinGraphObject *graph = rtree.NewGraphObject();
				graph->SetSRC(e);
				graph->SetValue(DOUBLEPOOL->Get(e->Attribute("value")));
				obj = graph;
			}
		}
		/*else if (ISNAME(e, "Number")) {

		}*/
		else if (ISNAME(e, "Lua")) {
			SkinScriptObject *script = rtree.NewScriptObject();
			if (e->Attribute("OnRender"))
				script->SetRunCondition(true);
			if (e->Attribute("src"))
				script->LoadFile(e->Attribute("src"));
			else if (e->GetText())
				script->SetScript(e->GetText());
			obj = script;
		}
		/*
		 * Special object parsing start
		 */
		else if (ISNAME(e, "Play")) {
			SkinPlayObject *p = rtree.NewPlayObject();
			p->x = e->IntAttribute("x");
			p->y = e->IntAttribute("y");
			p->w = e->IntAttribute("w");
			p->h = e->IntAttribute("h");
			XMLElement *lane = e->FirstChildElement("Note");
			while (lane) {
				p->ConstructLane(lane);
				lane = lane->NextSiblingElement("Note");
			}
			p->SetJudgelineObject(e->FirstChildElement("JUDGELINE"));
			p->SetLineObject(e->FirstChildElement("LINE"));
			// TODO: fetch lane effect, or other lane relative images as child
			ConstructTreeFromElement(rtree, p, e->FirstChildElement());

			obj = p;
		}
		else if (ISNAME(e, "Bga")) {
			SkinBgaObject *bga = rtree.NewBgaObject();
			obj = bga;
		}
		else {
			// parsed as unknown object
			// only parses SRC/DST condition
			obj = rtree.NewUnknownObject();
		}

		if (obj) {
			group->AddChild(obj);
			// parse common attribute: DST, condition
			SkinRenderHelper::ConstructBasicRenderObject(obj, e);
		}

		// search for next element
		e = e->NextSiblingElement();
	}
}

bool SkinRenderHelper::ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *e = s.skinlayout.FirstChildElement("Skin")->FirstChildElement();
	ConstructTreeFromElement(rtree, &rtree, e);

	return true;
}

bool SkinRenderHelper::LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s) {
	XMLElement *res = s.skinlayout.FirstChildElement("Resource");
	if (res) {
		XMLElement *img = res->FirstChildElement("Image");
		while (img) {
			RString path = img->Attribute("path");
			RString id = img->Attribute("name");
			rtree.RegisterImage(id, path);
			img = img->NextSiblingElement("Image");
		}
	}
	else {
		return false;
	}

	return true;
}

void SkinRenderHelper::ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e) {
	const char* condition = e->Attribute("condition");
	if (condition) obj->SetCondition(condition);
	ImageSRC _s;
	ConstructSRCFromElement(_s, e);
	XMLElement *dst = e->FirstChildElement("DST");
	while (dst) {
		ImageDST _d;
		ConstructDSTFromElement(_d, dst);
		const char* _cond = e->Attribute("condition");
		obj->AddDST(_d, _cond ? _cond : "");
		dst = e->NextSiblingElement("DST");
	}
}

void SkinRenderHelper::AddFrame(ImageDST &d, ImageDSTFrame &f) {
	d.frame.push_back(f);
}

SDL_Rect SkinRenderHelper::ToRect(ImageSRC &d) {
	// if SRC x, y < 0 then Set width/height with image's total width/height.
	int dx = d.w / d.divx;
	int dy = d.h / d.divy;
	Uint32 time = 0;
	if (d.timer) time = d.timer->GetTick();
	int frame = d.cycle ? time * d.divx * d.divy / d.cycle : 0;
	if (!d.loop && frame >= d.divx * d.divy)
		frame = d.divx * d.divy - 1;
	int x = frame % d.divx;
	int y = (frame / d.divx) % d.divy;
	return { d.x + x*dx, d.y + y*dy, dx, dy };
}

SDL_Rect SkinRenderHelper::ToRect(ImageDSTFrame &d) {
	return { d.x, d.y, d.w, d.h };
}

bool SkinRenderHelper::CalculateFrame(ImageDST &dst, ImageDSTFrame &frame) {
	if (dst.timer == 0 || dst.frame.size() == 0) {
		// this shouldnt happened
		return false;
	}

	// timer should alive
	if (!dst.timer->IsStarted())
		return false;
	uint32_t time = dst.timer->GetTick();

	// get current frame
	if (dst.loopstart > 0 && time > dst.loopstart) {
		time = (time - dst.loopstart) % (dst.loopend - dst.loopstart) + dst.loopstart;
	}
	int nframe = 0;
	for (; nframe < dst.frame.size(); nframe++) {
		if (dst.frame[nframe].time > time)
			break;
	}
	nframe--;
	// if smaller then first DST time, then hide
	// (TODO) work with current speed, if time is 0 (acc is NONE)
	if (nframe < 0)
		return false;

	// has next frame?
	// if not, return current frame 
	// if it does, Tween it.
	if (nframe >= dst.frame.size() - 1) {
		frame = dst.frame[nframe];
	}
	else {
		// very-very rare case, but we should prevent `div by 0`
		if (dst.frame[nframe + 1].time == dst.frame[nframe].time) {
			frame = dst.frame[nframe];
		}
		else {
			double t = (double)(time - dst.frame[nframe].time) / (dst.frame[nframe+1].time - dst.frame[nframe].time);
			frame = Tween(dst.frame[nframe], dst.frame[nframe + 1], t, dst.acctype);
		}
	}

	return true;
}

ImageDSTFrame SkinRenderHelper::Tween(ImageDSTFrame &a, ImageDSTFrame &b, double t, int acctype) {
	switch (acctype) {
	case ACCTYPE::LINEAR:
#define TWEEN(a, b) ((a)*(1-t) + (b)*t)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
		break;
	case ACCTYPE::DECEL:
#define T (sqrt(1-(t-1)*(t-1)))
#define TWEEN(a, b) ((a)*(1-T) + (b)*T)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
#undef T
		break;
	case ACCTYPE::ACCEL:
#define T (-sqrt(1-t*t)+1)
#define TWEEN(a, b) ((a)*(1-T) + (b)*T)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
#undef T
		break;
	case ACCTYPE::NONE:
#define TWEEN(a, b) (a)
		return{
			TWEEN(a.time, b.time),
			TWEEN(a.x, b.x),
			TWEEN(a.y, b.y),
			TWEEN(a.w, b.w),
			TWEEN(a.h, b.h),
			TWEEN(a.a, b.a),
			TWEEN(a.r, b.r),
			TWEEN(a.g, b.g),
			TWEEN(a.b, b.b),
			TWEEN(a.angle, b.angle),
		};
#undef TWEEN
		break;
	}
}

void SkinRenderHelper::PushRenderOffset(int x, int y) {
	_offset_stack.push_back({ x, y });
	RecalculateOffset();
}

void SkinRenderHelper::PopRenderOffset() {
	_offset_stack.pop_back();
	RecalculateOffset();
}

void SkinRenderHelper::Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend, int rotationcenter) {
	// if alpha == 0 then don't draw
	if (frame->a == 0) return;

	SDL_SetTextureAlphaMod(img->GetPtr(), frame->a);
	SDL_SetTextureColorMod(img->GetPtr(), frame->r, frame->g, frame->b);

	switch (blend) {
	case 0:
		/*
		 * LR2 is strange; NO blending does BLENDING, actually.
		 *
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_NONE);
		break;
		*/
	case 1:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_BLEND);
		break;
	case 2:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_ADD);
		break;
	case 4:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_MOD);
		break;
	default:
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_BLEND);
	}

	if (src->w <= 0) src->w = img->GetWidth();
	if (src->h <= 0) src->h = img->GetHeight();
	SDL_Rect src_rect = ToRect(*src);
	SDL_Rect dst_rect = ToRect(*frame);

	SDL_Point rot_center = { 0, 0 };
	switch (rotationcenter) {
	case ROTATIONCENTER::TOPLEFT:
		rot_center = { 0, 0 };
		break;
	case ROTATIONCENTER::TOPCENTER:
		rot_center = { dst_rect.w / 2, 0 };
		break;
	case ROTATIONCENTER::TOPRIGHT:
		rot_center = { dst_rect.w, 0 };
		break;
	case ROTATIONCENTER::CENTERLEFT:
		rot_center = { 0, dst_rect.h / 2 };
		break;
	case 0:
	case ROTATIONCENTER::CENTER:
		rot_center = { dst_rect.w / 2, dst_rect.h / 2 };
		break;
	case ROTATIONCENTER::CENTERRIGHT:
		rot_center = { dst_rect.w, dst_rect.h / 2 };
		break;
	case ROTATIONCENTER::BOTTOMLEFT:
		rot_center = { 0, dst_rect.h };
		break;
	case ROTATIONCENTER::BOTTOMCENTER:
		rot_center = { dst_rect.w / 2, dst_rect.h };
		break;
	case ROTATIONCENTER::BOTTOMRIGHT:
		rot_center = { dst_rect.w, dst_rect.h };
		break;
	}

	// in LR2, negative size doesn't mean flipping.
	if (dst_rect.h < 0) {
		dst_rect.y += dst_rect.h;
		dst_rect.h *= -1;
	}
	if (dst_rect.w < 0) {
		dst_rect.x += dst_rect.h;
		dst_rect.w *= -1;
	}

	// Offset
	dst_rect.x += _offset_calculated.x;
	dst_rect.y += _offset_calculated.y;

	SDL_RendererFlip flip = SDL_RendererFlip::SDL_FLIP_NONE;
	SDL_RenderCopyEx(Game::RENDERER, img->GetPtr(), &src_rect, &dst_rect,
		frame->angle, &rot_center, flip);
}

#pragma endregion SKINRENDERHELPER