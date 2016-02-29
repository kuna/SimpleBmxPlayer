#include "ActorBasic.h"
#include "global.h"
#include "Pool.h"
#include "util.h"
#include "luahelper.h"
#include "game.h"
#include "file.h"
#include "logger.h"


void ImageSRC::Clear() {
	x = y = w = h = cycle = 0;
	divx = divy = 1;
	loop = 1;
	timer = 0;
}

void ImageSRC::SetFromXml(const XMLElement* e) {
	e->QueryIntAttribute("sx", &x);
	e->QueryIntAttribute("sy", &y);
	e->QueryIntAttribute("sw", &w);
	e->QueryIntAttribute("sh", &h);
	e->QueryIntAttribute("divx", &divx);
	e->QueryIntAttribute("divy", &divy);
	// TODO: loop? timer?
}

Display::Rect ImageSRC::Calculate() {
	// if SRC x, y < 0 then Set width/height with image's total width/height.
	int dx = w / divx;
	int dy = h / divy;
	Uint32 time = 0;
	if (timer) time = timer->GetTick();
	int frame = cycle ? time * divx * divy / cycle : 0;
	if (!loop && frame >= divx * divy)
		frame = divx * divy - 1;
	int x = frame % divx;
	int y = (frame / divx) % divy;
	return { x + x*dx, y + y*dy, dx, dy };
}

TweenInfo::TweenInfo() {
	acc = 0;
	loop = 0;
	state.color.a =
		state.color.r =
		state.color.g =
		state.color.b = 255;
	state.dst.x =
		state.dst.y =
		state.dst.w =
		state.dst.h = 0;
	state.rotate.x =
		state.rotate.y =
		state.rotate.z = 0;
	state.shear.x =
		state.shear.y = 0;
	state.zoom.x =
		state.zoom.y = 0;
}

void ImageDST::Clear() {
	tweens.clear();
	timer.Stop();
	blend = 0;
	center = 0;
	zwrite = false;
	zpos = 0;
}

void ImageDST::SetFromCmd(const RString& cmd) {
	// current time & set basic status
	int time = 0;

	// generate tweens from command
	// (TODO: convert to lua code - accessible to tweens() ?
	// -> accessible to ImageDST ... SetTime(), SetX() ...
	std::vector<RString> cmds, args;
	split(cmd, ";", cmds);
	for (int i = 0; i < cmds.size(); i++) {
		split(cmds[i], ",", args);
		const char* name = args[0];

		if (stricmp(name, "time") == 0) {
			time = atoi(args[1]);
			// if current time of tween doesn't exists,
			// then copy it from previous one
			if (tweens.begin() != tweens.end() && tweens.find(time) == tweens.end()) {
				auto iter = tweens.upper_bound(time);
				if (iter != tweens.begin()) {
					--iter;
					tweens[time] = iter->second;
				}
			}
		}
		else if (stricmp(name, "x") == 0) {
			tweens[time].state.dst.x = atoi(args[1]);
		}
		else if (stricmp(name, "y") == 0) {
			tweens[time].state.dst.y = atoi(args[1]);
		}
		else if (stricmp(name, "w") == 0) {
			tweens[time].state.dst.w = atoi(args[1]);
		}
		else if (stricmp(name, "h") == 0) {
			tweens[time].state.dst.h = atoi(args[1]);
		}
		else if (stricmp(name, "a") == 0) {
			tweens[time].state.color.a = atoi(args[1]);
		}
		else if (stricmp(name, "r") == 0) {
			tweens[time].state.color.r = atoi(args[1]);
		}
		else if (stricmp(name, "g") == 0) {
			tweens[time].state.color.g = atoi(args[1]);
		}
		else if (stricmp(name, "b") == 0) {
			tweens[time].state.color.b = atoi(args[1]);
		}
		else if (stricmp(name, "zoomx") == 0) {
			tweens[time].state.zoom.x = atof(args[1]);
		}
		else if (stricmp(name, "zoomy") == 0) {
			tweens[time].state.zoom.y = atof(args[1]);
		}
		else if (stricmp(name, "rotx") == 0) {
			tweens[time].state.rotate.x = atoi(args[1]);
		}
		else if (stricmp(name, "roty") == 0) {
			tweens[time].state.rotate.y = atoi(args[1]);
		}
		else if (stricmp(name, "rotz") == 0) {
			tweens[time].state.rotate.z = atoi(args[1]);
		}
		else if (stricmp(name, "shearx") == 0) {
			tweens[time].state.shear.x = atof(args[1]);
		}
		else if (stricmp(name, "sheary") == 0) {
			tweens[time].state.shear.y = atof(args[1]);
		}
		else if (stricmp(name, "acc") == 0) {
			tweens[time].acc = atoi(args[1]);
		}
		else if (stricmp(name, "loop") == 0) {
			tweens[time].loop = atoi(args[1]);
		}
	}
}

namespace {
	float CalcTween_Linear(float t) { return t; }
	float CalcTween_Accelerate(float t) { return t * t; }
	float CalcTween_Decelerate(float t) { return 1 - (1-t) * (1-t); }
	float CalcTween_None(float t) { return 0; }

	void Tween(const TweenState &t1, const TweenState &t2, double t, int acctype, TweenState &out) {
		float c = 0;
		switch (acctype) {
		case ACCTYPE::NONE:
			c = CalcTween_None(t);
			break;
		case ACCTYPE::ACCEL:
			c = CalcTween_Accelerate(t);
			break;
		case ACCTYPE::DECEL:
			c = CalcTween_Decelerate(t);
			break;
		default:
		case ACCTYPE::LINEAR:
			break;
		}
		/* 0.5 : kinda of lightweight round function */
#define TWEEN(a, b) (a * (1-c) + b * c + 0.5)
		out.color.a = TWEEN(t1.color.a, t2.color.a);
		out.color.r = TWEEN(t1.color.r, t2.color.r);
		out.color.g = TWEEN(t1.color.g, t2.color.g);
		out.color.b = TWEEN(t1.color.b, t2.color.b);
		out.dst.x = TWEEN(t1.dst.x, t2.dst.x);
		out.dst.y = TWEEN(t1.dst.y, t2.dst.y);
		out.dst.w = TWEEN(t1.dst.w, t2.dst.w);
		out.dst.h = TWEEN(t1.dst.h, t2.dst.h);
		out.rotate.x = TWEEN(t1.rotate.x, t2.rotate.x);
		out.rotate.y = TWEEN(t1.rotate.y, t2.rotate.y);
		out.rotate.z = TWEEN(t1.rotate.z, t2.rotate.z);
		out.shear.x = TWEEN(t1.shear.x, t2.shear.x);
		out.shear.y = TWEEN(t1.shear.y, t2.shear.y);
		out.zoom.x = TWEEN(t1.zoom.x, t2.zoom.x);
		out.zoom.y = TWEEN(t1.zoom.y, t2.zoom.y);
#undef TWEEN
	}
}

void ImageDST::CalculateTween(TweenInfo &t) {
	// if no more dst, then don't update
	if (tweens.size() == 0) return;

	// get current iterator
	uint32_t tick = timer.GetTick();
	auto it = tweens.upper_bound(tick);

	// if smaller then first DST time, don't update
	// (It should be hidden in default)
	if (it == tweens.begin()) return;

	// in case of last tween, check loop.
	// if no loop, then copy once and exit.
	// if loop exists, then process loop until we get proper time
	if (it == tweens.end()) {
		--it;
		if (it->second.loop) {
			uint32_t looptime = it->first - it->second.loop;
			tick = it->second.loop + tick % looptime;
		}
		else {
			t = it->second;
			tweens.clear();
			return;
		}
	}
	const TweenInfo *t1 = &it->second;
	--it;
	const TweenInfo *t2 = &it->second;

	// now just get tween & copy them to tweeninfo
	Tween(t1->state, t2->state, 0, t1->acc, t.state);
}


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
		cond[condcnt] = HANDLERPOOL->Get(key[condcnt]);
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
		// COMMENT: change it to LuaFunc()
		Lua* l = LUA->Get();
		RString e;
		LuaHelper::RunScript(l, luacondition, "Skin_RenderCondition", e, 0, 1, 0);
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
Actor::Actor(int type) : objtype(type), handler(this) { Clear(); }

int Actor::GetType() { return objtype; }

void Actor::Clear() {
	// set all values in default
	drawable = false;
	clickable = false;
	focusable = false;
	m_Tex = 0;
	m_Src.Clear();
	m_Dst.Clear();
}

// draw primitive
void Actor::Render() {
	if (m_Tex && drawable) {
		DISPLAY->PushState();

		// precalculate something - center pos
		Display::Point center = { m_Tweeninfo.state.dst.x, m_Tweeninfo.state.dst.y };
		switch (m_Dst.center) {
		case ROTATIONCENTER::TOPLEFT:
			break;
		case ROTATIONCENTER::TOPCENTER:
			center.x += m_Tweeninfo.state.dst.w / 2;
			break;
		case ROTATIONCENTER::TOPRIGHT:
			center.x += m_Tweeninfo.state.dst.w;
			break;
		case ROTATIONCENTER::CENTERLEFT:
			center.y += m_Tweeninfo.state.dst.h / 2;
			break;
		case ROTATIONCENTER::CENTERRIGHT:
			center.x += m_Tweeninfo.state.dst.w;
			center.y += m_Tweeninfo.state.dst.h / 2;
			break;
		case ROTATIONCENTER::BOTTOMLEFT:
			center.y += m_Tweeninfo.state.dst.h;
			break;
		case ROTATIONCENTER::BOTTOMCENTER:
			center.x += m_Tweeninfo.state.dst.w / 2;
			center.y += m_Tweeninfo.state.dst.h;
			break;
		case ROTATIONCENTER::BOTTOMRIGHT:
			center.x += m_Tweeninfo.state.dst.w;
			center.y += m_Tweeninfo.state.dst.h;
			break;
		case ROTATIONCENTER::CENTER:
		default:
			center.x += m_Tweeninfo.state.dst.w / 2;
			center.y += m_Tweeninfo.state.dst.h / 2;
			break;
		}

		// check and set z-writing
		Display::Rect r = m_Src.Calculate();
		DISPLAY->SetSRC(&r);
		DISPLAY->SetBlendMode(m_Dst.blend);
		DISPLAY->SetCenter(center.x, center.y);
		DISPLAY->SetDST(&m_Tweeninfo.state.dst);
		DISPLAY->SetZWrite(m_Dst.zwrite);
		DISPLAY->SetZPos(m_Dst.zpos);
		// rotate
		DISPLAY->SetRotateX(m_Tweeninfo.state.rotate.x);
		DISPLAY->SetRotateY(m_Tweeninfo.state.rotate.y);
		DISPLAY->SetRotateZ(m_Tweeninfo.state.rotate.z);
		// shear
		DISPLAY->SetShear(m_Tweeninfo.state.shear.x, m_Tweeninfo.state.shear.y);
		// colormod
		DISPLAY->SetColorMod(m_Tweeninfo.state.color.r,
			m_Tweeninfo.state.color.g,
			m_Tweeninfo.state.color.b,
			m_Tweeninfo.state.color.a);

		DISPLAY->SetTexture(m_Tex);
		DISPLAY->DrawPrimitives();

		DISPLAY->PopState();
	}
}

void Actor::Update() {
	// evaluate condition
	if (!(drawable = m_Condition.Evaluate()))
		return;

	// Calculate Tween State
	m_Dst.CalculateTween(m_Tweeninfo);

	// check alpha(color) mod
	if (m_Tweeninfo.state.color.a == 0)
		drawable = false;
}

void Actor::SetFromXml(const XMLElement *e) {
	const char *attr;

	// load basic attributes
	if (attr = e->Attribute("visible"))
		SetCondition(attr);
	if (attr = e->Attribute("value"))
		m_SrcValue = e->IntAttribute("value");
	// key isn't supported in basic actor
	//if (attr = e->Attribute("key"))
	//	;
	if (attr = e->Attribute("blend"))
		SetBlend(atoi(attr));
	if (attr = e->Attribute("zwrite"))
		m_Dst.zwrite = e->BoolAttribute("zwrite");
	if (attr = e->Attribute("zpos"))
		m_Dst.zpos = e->IntAttribute("zpos");

	// load SRC attribute
	m_Src.SetFromXml(e);

	// load Commands
	for (auto c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
		if (strnicmp(c->Name(), "On", 2) == 0) {
			std::string eventname = c->Name() + 2;
			RString cmdstr = "";
			for (auto cmd = c->FirstChildElement(); cmd; cmd = cmd->NextSiblingElement()) {
				cmdstr.append(cmd->Name());
				for (auto attr = cmd->FirstAttribute(); attr; attr = attr->Next()) {
					cmdstr.append(",");
					cmdstr.append(attr->Value());
				}
				cmdstr.append(";");
			}
			m_Cmds[eventname] = cmdstr;
		}
	}

	// Initalize DST
	Message msg;
	msg.name = "Init";
	handler.Receive(msg);
}

void Actor::SetParent(Actor *pActor) { m_pParent = pActor; }

bool Actor::Click(int x, int y) {
	// must call after Update()
	if (drawable && clickable) {
		// (TODO)
		return false;
	}
	else {
		return false;
	}
}

bool Actor::Hover(int x, int y) {
	// must call after Update()
	if (drawable && focusable) {
		// (TODO)
		return false;
	}
	else {
		return false;
	}
}

void Actor::SetBlend(int blend) {
	m_Dst.blend = blend;
}

void Actor::SetCenter(int v) {
	m_Dst.center = v;
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

void Actor::SetCondition(const RString &str) { m_Condition.Set(str); }

void Actor::ActorHandler::Receive(const Message& msg) {
	auto iter = pActor->m_Cmds.find(msg.name);
	if (iter != pActor->m_Cmds.end()) {
		pActor->m_Dst.SetFromCmd(iter->second);
	}
}

#pragma endregion







#pragma region SKINGROUPOBJECT
ActorFrame::ActorFrame()
	: Actor(ACTORTYPE::GROUP)
{
}

ActorFrame::~ActorFrame() {
	// delete all child elements
	for (int i = 0; i < m_Children.size(); i++)
		delete m_Children[i];
}

void ActorFrame::SetFromXml(const XMLElement *e) {
	// basic actor attribute
	Actor::SetFromXml(e);

	// parse & create child
	for (auto c = e->FirstChildElement(); c; c = c->NextSiblingElement()) {
		Actor *p = Theme::MakeActor(c, this);
		if (p) m_Children.push_back(p);
	}
}

void ActorFrame::Update() {
	// update myself
	Actor::Update();
	if (drawable) {
		// update childs
		for (int i = 0; i < m_Children.size(); i++)
			m_Children[i]->Update();
	}
}

void ActorFrame::Render() {
	if (!drawable) return;

	// before render, prepare matrix change
	DISPLAY->PushState();
	DISPLAY->SetRotateX(m_Tweeninfo.state.rotate.x);
	DISPLAY->SetRotateY(m_Tweeninfo.state.rotate.y);
	DISPLAY->SetRotateZ(m_Tweeninfo.state.rotate.z);
	DISPLAY->SetOffset(m_Tweeninfo.state.dst.x, m_Tweeninfo.state.dst.y);

	// recursively renders child element
	for (auto it = begin(); it != end(); ++it) {
		(*it)->Render();
	}

	// end
	DISPLAY->PopState();
}

std::vector<Actor*>::iterator
ActorFrame::begin() { return m_Children.begin(); }

std::vector<Actor*>::iterator
ActorFrame::end() { return m_Children.end(); }

#pragma endregion 







ActorSprite::ActorSprite(int type) : Actor(type) {
}

void ActorSprite::SetFromXml(const XMLElement *e) {
	Actor::SetFromXml(e);
	
	// get texture from Theme / Pool
	const char *texname = 0;
	if ((m_Tex = Theme::GetTexture(texname)) == 0)
		m_Tex = TEXPOOL->Get(texname);
}

void ActorSprite::Update() {
	Actor::Update();
	
	// if image is not loop && tick overtime end then don't draw
	if (drawable)
		drawable &= m_Src.loop || (!m_Src.loop && m_Src.timer &&
			m_Src.timer->GetTick() < m_Src.cycle);
}

void ActorSprite::Render() {
	Actor::Render();
}







ActorText::ActorText()
	: Actor(ACTORTYPE::TEXT), m_Font(0), v(0), align(0), editable(false) {}

void ActorText::SetFromXml(const XMLElement *e) {
	Actor::SetFromXml(e);
	if (e->Attribute("value"))
		SetValue(STRPOOL->Get(e->Attribute("value")));
	SetAlign(e->IntAttribute("align"));
	SetFont(e->Attribute("file"));
}

void ActorText::SetFont(const char* id) {
	if (id) {
		m_Font = Theme::GetFont(id);
		if (!m_Font) {
			m_Font = FONTPOOL->GetByID(id);
		}
	}
}

void ActorText::SetValue(RString* s) { v = s; }

void ActorText::SetEditable(bool editable) { this->editable = editable; }

void ActorText::SetAlign(int align) { this->align = align; }

int ActorText::GetWidth() { if (v) return GetTextWidth(*v); else return 0; }

int ActorText::GetTextWidth(const RString& s) {
	if (m_Font) {
		return m_Font->GetWidth(s);
	}
	else {
		return 0;
	}
}

void ActorText::Render() { if (v && drawable) RenderText(*v); }

void ActorText::RenderText(const char* s) {
	if (m_Font && drawable) {
		// check align
		int left_offset = 0;
		switch (align) {
		case 1:		// center
			left_offset = -(m_Font->GetWidth(s)) / 2;
			break;
		case 2:		// right
			left_offset = -m_Font->GetWidth(s);
			break;
		}
		DISPLAY->PushState();
		// TODO: SetRenderState
		m_Font->Render(s, m_Tweeninfo.state.dst.x + left_offset, m_Tweeninfo.state.dst.y);
		DISPLAY->PopState();
	}
}







ActorNumber::ActorNumber()
	: ActorText(), v(0) {
	objtype = ACTORTYPE::NUMBER;
}

void ActorNumber::SetValue(int *i) { v = i; }

void ActorNumber::SetLength(int length) { this->length = length; }

int ActorNumber::GetWidth() {
	if (v && drawable) {
		CacheInt(*v);
		return GetTextWidth(buf);
	}
	else return 0;
}

void ActorNumber::Set24Mode(bool b) { mode24 = b; }

void ActorNumber::SetFromXml(const XMLElement *e) {
	Actor::SetFromXml(e);
	if (e->Attribute("value"))
		SetValue(INTPOOL->Get(e->Attribute("value")));
	SetAlign(e->IntAttribute("align"));
	SetLength(e->IntAttribute("length"));
	Set24Mode(e->IntAttribute("mode24"));
	SetFont(e->Attribute("file"));
}

void ActorNumber::Render() {
	if (v && drawable) {
		CacheInt(*v);
		RenderText(buf);
	}
}

void ActorNumber::CacheInt(int n) {
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







ActorGraph::ActorGraph() : ActorSprite(ACTORTYPE::GRAPH), type(0), direction(0), m_Key(0), m_Value(0) {}

void ActorGraph::SetKey(const char* key) { if (key) m_Key = DOUBLEPOOL->Get(key); }

void ActorGraph::SetValue(double v) { m_Value = v; }

void ActorGraph::SetType(int type) { this->type = type; }

void ActorGraph::SetDirection(int direction) { this->direction = direction; }

void ActorGraph::SetFromXml(const XMLElement *e) {
	ActorSprite::SetFromXml(e);
	SetKey(e->Attribute("value"));
}

void ActorGraph::Update() {
	Actor::Update();

	// update m_value if key exists
	if (m_Key) m_Value = *m_Key;

	// TODO: type, direction
	m_Tweeninfo.state.dst.h *= m_Value;
}

void ActorGraph::Render() {
	Actor::Render();
}







ActorSlider::ActorSlider()
	: ActorSprite(ACTORTYPE::SLIDER), range(0), direction(0), m_Key(0), m_Value(0) {}

void ActorSlider::SetValue(double v) { m_Value = v; }

void ActorSlider::SetKey(const char* key) { if (key) m_Key = DOUBLEPOOL->Get(key); }

void ActorSlider::SetRange(int range) { this->range = range; }

void ActorSlider::SetDirection(int direction) { this->direction = direction; }

void ActorSlider::SetFromXml(const XMLElement *e) {
	ActorSprite::SetFromXml(e);
	SetKey(e->Attribute("key"));
	SetRange(e->IntAttribute("range"));
	SetDirection(e->IntAttribute("direction"));
}

void ActorSlider::Update() {
	Actor::Update();

	// if key exists, then update value
	if (m_Key) m_Value = *m_Key;

	// TODO: process direction
	m_Tweeninfo.state.dst.y += range * m_Value;
}

void ActorSlider::Render() {
	Actor::Render();
}







ActorButton::ActorButton() : ActorSprite(ACTORTYPE::BUTTON) {}







#pragma region SCRIPTOBJECT
ActorScript::ActorScript()
	: Actor(ACTORTYPE::SCRIPT) {}

void ActorScript::SetFromXml(const XMLElement *e) {
	if (e->Attribute("file"))
		LoadFile(e->Attribute("file"));
	else if (e->GetText()) {
		script = e->GetText();
	}

	// execute lua code (no return value)
	Lua* L = LUA->Get();
	LuaHelper::RunExpression(L, script);
	LUA->Release(L);
}

void ActorScript::LoadFile(const RString &filepath) {
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

void ActorScript::Update() {
}

void ActorScript::Render() {
}

#pragma endregion SCRIPTOBJECT





// register actors!
REGISTER_ACTOR(Actor);
REGISTER_ACTOR_WITH_NAME("frame", ActorFrame);
REGISTER_ACTOR_WITH_NAME("sprite", ActorSprite);
REGISTER_ACTOR_WITH_NAME("text", ActorText);
REGISTER_ACTOR_WITH_NAME("number", ActorNumber);
REGISTER_ACTOR_WITH_NAME("graph", ActorGraph);
REGISTER_ACTOR_WITH_NAME("slider", ActorSlider);
REGISTER_ACTOR_WITH_NAME("list", ActorList);
REGISTER_ACTOR_WITH_NAME("lua", ActorScript);