#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
#include "LuaHelper.h"
#include <assert.h>
using namespace tinyxml2;
//
// skin converter
//


namespace SkinConverter {
	// private function; for searching including file
	void SearchInclude(std::vector<std::string>& inc, XMLElement* node) {
		for (XMLElement* n = node;
			n;
			n = n->NextSiblingElement())
		{
			if (strcmp(n->Name(), "include") == 0) {
				inc.push_back(SkinUtil::GetAbsolutePath(n->Attribute("path")));
			}
			SearchInclude(inc, n->FirstChildElement());
		}
	}

	// depreciated; only for basic test, or 
	bool ConvertLR2SkinToXml(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".xml");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->Save(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".xml");
					// TODO: depart texturefont files?
					csvskin->Save(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return r;
	}

	bool ConvertLR2SkinToLua(const char* lr2skinpath) {
		_LR2SkinParser *parser = new _LR2SkinParser();
		SkinMetric* skinmetric = new SkinMetric();
		Skin* skinmetric_code = new Skin();
		// skin include path is relative to csvskin path,
		// so need to register basepath
		std::string basedir = SkinUtil::GetParentDirectory(lr2skinpath);
		SkinUtil::SetBasePath(basedir);

		bool r = parser->ParseLR2Skin(lr2skinpath, skinmetric);
		r &= parser->ParseCSV(lr2skinpath, skinmetric_code);
		if (r) {
			std::string dest_xml = SkinUtil::ReplaceExtension(lr2skinpath, ".skin.xml");
			std::string dest_lua = SkinUtil::ReplaceExtension(lr2skinpath, ".lua");
			// save metric data first
			skinmetric->Save(dest_xml.c_str());
			skinmetric_code->SaveToLua(dest_lua.c_str());
			// check for included files
			std::vector<std::string> include_csvs;
			SearchInclude(include_csvs, skinmetric_code->skinlayout.FirstChildElement());
			// convert all included files
			for (int i = 0; i < include_csvs.size(); i++) {
				Skin* csvskin = new Skin();
				if (parser->ParseCSV(include_csvs[i].c_str(), csvskin)) {
					std::string dest_path = SkinUtil::ReplaceExtension(include_csvs[i], ".lua");
					// TODO: depart texturefont files?
					csvskin->SaveToLua(dest_path.c_str());
				}
				delete csvskin;
			}
		}
		delete skinmetric;
		delete parser;

		return false;
	}
}

/*
* local converter
*/
namespace {
#define TAGMAXCOUNT 50
	// TODO: not processed objects (combo)
	// TODO: notefield requires information per lane (think about this more ...)
	// TODO: can we send lua code in tween object?
	const char* nosrctags[TAGMAXCOUNT] = {
		// these object generally means group object
		// which has no SRC attributes at all.
		"notefield", "combo", "listview",
	};
	const char *objecttags[TAGMAXCOUNT] = {
		// generals
		"sprite", "text", "number", "graph", "button", "slider",
		// ingenerals
		"bga", "groovegauge", "judgeline", "note", "scratch", "line",
	};
	const char* srctags[TAGMAXCOUNT] = {
		"SRC_NOTE",
		"SRC_LN_START",
		"SRC_LN_BODY",
		"SRC_LN_END",
		"SRC_MINE",
		"SRC_AUTO_NOTE",
		"SRC_AUTO_LN_START",
		"SRC_AUTO_LN_BODY",
		"SRC_AUTO_LN_END",
		"SRC_AUTO_MINE",
		"SRC_GROOVE_ACTIVE",
		"SRC_GROOVE_INACTIVE",
		"SRC_HARD_ACTIVE",
		"SRC_HARD_INACTIVE",
		"SRC_EX_ACTIVE",
		"SRC_EX_INACTIVE",
	};
	const char* dsttags[TAGMAXCOUNT] = {
		"DST",
	};
	const char* srcattr[TAGMAXCOUNT] = {
		// SRC supports only general attributes
		// so filter it in here
		"x", "y", "w", "h", "divx", "divy", "timer", "loop",
	};

	// find out is tag included in this attribute
	bool CheckTagGroup(const char* testtag, const char** taglist) {
		for (int i = 0; i < TAGMAXCOUNT && taglist[i]; i++) {
			if (strcmp(taglist[i], testtag) == 0)
				return true;
		}
		return false;
	}
}

void XmlToLuaConverter::AppendIndentBody(int indent) {
	for (int i = 0; i < indent; i++)
		body.push_back('\t');
}
void XmlToLuaConverter::AppendBody(const std::string& str) {
	AppendIndentBody(indent);
	body.append(str);
	body.append("\n");
}
void XmlToLuaConverter::AppendHead(const std::string& str) {
	head.append(str);
	head.append("\n");
}
// process single SRC
void XmlToLuaConverter::AppendSRC(const XMLElement *e) {
	AppendIndentBody(indent);
	// if it starts with name SRC~,
	// then it's SRC specific object - use its own name.
	if (strncmp(e->Name(), "SRC", 3) == 0)
		body.append(ssprintf("%s={", e->Name()));
	else
		body.append("Src={");
	// iterate through attributes ...
	for (const XMLAttribute *attr = e->FirstAttribute();
		attr;
		attr = attr->Next())
	{
		const char *name = attr->Name();
		// skip attribute if its not general attribute
		if (!CheckTagGroup(name, srcattr))
			continue;
		if (strcmp(name, "timer") == 0)
			body.append(ssprintf("%s=\"%s\",", name, attr->Value()));
		else
			body.append(ssprintf("%s=%s,", name, attr->Value()));
	}
	if (body.back() == ',') body.pop_back();
	body.append("};\n");
}
// process single DST
void XmlToLuaConverter::AppendDST(const XMLElement *dst) {
	if (!dst) return;
	// timer is get changed into event handler
	const char *timer = dst->Attribute("timer");
	if (!timer)
		timer = "Init";
	AppendIndentBody(indent);
	body.append(ssprintf("On%s=DST{\n", timer));
	indent++;
	// add basic dst attribute (loop, acc, rotatecenter, blend)
	if (dst->Attribute("acc"))
		AppendBody(ssprintf("acc=%s,", dst->Attribute("acc")));
	if (dst->Attribute("rotatecenter"))
		AppendBody(ssprintf("rotatecenter=%s,", dst->Attribute("acc")));
	if (dst->Attribute("blend"))
		AppendBody(ssprintf("blend=%s,", dst->Attribute("acc")));
	// add dst frames
	for (const XMLElement *f = dst->FirstChildElement();
		f;
		f = f->NextSiblingElement())
	{
		AppendIndentBody(indent);
		for (const XMLAttribute *attr = f->FirstAttribute();
			attr;
			attr = attr->Next())
		{
			const char *aname = attr->Name();
			body.append(ssprintf("%s=%s,", aname, attr->Value()));
		}
		body.push_back('\n');
	}
	body.pop_back();
	// add loop on the last
	if (dst->Attribute("loop")) {
		body.append(ssprintf("loop=%s", dst->Attribute("loop")));
	}
	indent--;
	body.append("};\n");
}
void XmlToLuaConverter::AppendComment(const XMLComment *cmt) {
	AppendIndentBody(indent);
	const char *pn, *p = cmt->Value();
	do {
		pn = strchr(p, '\n');
		std::string l;
		if (!pn)
			l = p;
		else
			l = std::string(p, pn - p);
		body.append(ssprintf("-- %s\n", l.c_str()));
		p = pn + 1;
	} while (pn);
};
// process general object/groups
void XmlToLuaConverter::AppendObject(const XMLElement *e, const char* name) {
	// some object - like playfield / combo - is quite nasty.
	// take care of them carefully.
	AppendIndentBody(indent);
	body.append(ssprintf("Def.%s{\n", name));
	indent++;
	if (e->Attribute("resid"))
		AppendBody(ssprintf("file=\"%s\";", e->Attribute("resid")));
	// process some object-specific attributes
	const char *attrval;
	if (attrval = e->Attribute("condition"))
		AppendBody(ssprintf("class=\"%s\";", attrval));
	if (attrval = e->Attribute("value"))
		AppendBody(ssprintf("valuekey=\"%s\";", attrval));
	if (attrval = e->Attribute("align"))
		AppendBody(ssprintf("align=%s;", attrval));
	if (attrval = e->Attribute("edit"))
		AppendBody(ssprintf("edit=%s;", attrval));
	if (attrval = e->Attribute("length"))
		AppendBody(ssprintf("length=%s;", attrval));
	if (attrval = e->Attribute("range"))
		AppendBody(ssprintf("range=%s;", attrval));
	if (attrval = e->Attribute("direction"))
		AppendBody(ssprintf("direction=%s;", attrval));
	// add SRC if this tag isn't group object
	if (!CheckTagGroup(e->Name(), nosrctags))
		AppendSRC(e);
	// parse inner element
	Parse(e->FirstChildElement());
	indent--;
	AppendBody("};");
	objid++;
}
void XmlToLuaConverter::AppendElement(const XMLElement *e) {
	const char* name = e->Name();
	if (strcmp(name, "skin") == 0) {
		Parse(e->FirstChild());
	}
	// in case of include
	//
	else if (strcmp(name, "include") == 0) {
		AppendBody(ssprintf("LoadObject(\"%s\");", e->Attribute("path")));
	}
	// in case of resource,
	// parse it and register it as cached resource
	else if (strcmp(name, "image") == 0) {
		std::string r = ssprintf("LoadImage(\"%s\", {path=\"%s\"});",
			e->Attribute("name"), e->Attribute("path"));
		if (depth == 0)
			AppendHead(r);
		else
			AppendBody(r);
	}
	else if (strcmp(name, "font") == 0) {
		std::string r = ssprintf("LoadFont(\"%s\", {path=\"%s\", size=%s, border=%s});",
			e->Attribute("name"), e->Attribute("path"),
			e->Attribute("size"), e->Attribute("border"));
		if (depth == 0)
			AppendHead(r);
		else
			AppendBody(r);
	}
	else if (strcmp(name, "texturefont") == 0) {
		//AppendBody(ssprintf("-- converter message: use path rather than data attribute"));
		AppendHead(ssprintf("texturefont_data=[[%s]]", e->GetText()));
		AppendHead(ssprintf("LoadTFont(\"%s\", {data=texturefont_data});", e->Attribute("name")));
	}

	// in case of conditional clause
	else if (strcmp(name, "condition") == 0) {
		AppendBody("(function()");
		for (const XMLElement *cond = e->FirstChildElement();
			cond;
			cond = cond->NextSiblingElement())
		{
			if (strcmp(cond->Name(), "else") == 0) {
				AppendBody("}; else return Def.frame{");
			}
			else if (strcmp(cond->Name(), "elseif") == 0) {
				AppendBody(ssprintf("}; elseif CND(\"%s\") then return Def.frame{", cond->Attribute("condition")));
			}
			else {
				AppendBody(ssprintf("if CND(\"%s\") then return Def.frame{", cond->Attribute("condition")));
			}
			indent++;
			Parse(cond->FirstChild());
			indent--;
			/*
			* I think we don't need to create group, as we just add elements to here ...
			* --but need in case of group object, like this:
			* no, it's redundant unless if clause is positioned inside group object.
			*
			AppendHead("t[#t+1] = (function()");
			indent++;
			AppendHead("local t = {}");
			Parse(cond->FirstChild());
			AppendHead("return t");
			indent--;
			AppendHead("end");
			*/
		}
		AppendBody("}; end");
		AppendBody("end)();");
	}
	// in case of Lua
	else if (strcmp(name, "lua") == 0) {
		const char *pn, *p = e->GetText();
		do {
			pn = strchr(p, '\n');
			std::string l;
			if (!pn)
				l = p;
			else
				l = std::string(p, pn - p);
			AppendBody(l.c_str());
			p = pn + 1;
		} while (pn);
	}
	// in case of group / object / src / dst
	else if (CheckTagGroup(name, objecttags) || CheckTagGroup(name, nosrctags)) {
		depth++;
		AppendObject(e, name);
		depth--;
	}
	else if (CheckTagGroup(name, srctags)) {
		AppendSRC(e);
	}
	else if (CheckTagGroup(name, dsttags)) {
		AppendDST(e);
	}
	// unexpected tag
	else {
		printf("XmlToLua - Unexpected tag(%s), ignore.\n", name);
	}
};
void XmlToLuaConverter::Parse(const XMLNode *node) {
	for (const XMLNode *n = node;
		n;
		n = n->NextSibling())
	{
		if (n->ToComment()) {
			AppendComment(n->ToComment());
		}
		else if (n->ToElement()) {
			AppendElement(n->ToElement());
		}
	}
};
void XmlToLuaConverter::StartParse(const XMLNode *node) {
	Clear();
	indent++;
	Parse(node);
	indent--;
}
std::string XmlToLuaConverter::GetLuaCode() {
	return
		"-- Auto generated lua code\n" +
		head +
		"\n\n"
		"-- skin table structure\n"
		"local t = Def.ActorGroup{\n" +
		body +
		"}\n" +
		"return t";
};
void XmlToLuaConverter::Clear() {
	indent = 0;
	objid = 0;
	head = "";
	body = "";
	indent = 0;
	depth = 0;
};

// ---------- Lua Converter end -----------------------------





namespace SkinTest {
	// ////////////////////////////////////////////////////////
	// lua testing part
	// ////////////////////////////////////////////////////////
#ifdef LUAMANAGER
	//
	// emulates some lua functions
	//

	void InitLua() {
		LUA = new LuaManager();
		Lua *l = LUA->Get();
		LuaUtil::RegisterBasic(l);
		LuaHelper::RunScriptFile(l, "../system/test/dummy.lua");
		LUA->Release(l);
	}

	void CloseLua() {
		delete LUA;
	}

	// private end, public start

	/*
	** DO NOT run this function in multi thread
	** although this func will be automatically locked...
	*/
	bool TestLuaSkin(const char* luapath) {
		Lua* l = LUA->Get();
		printf("stack before run: %d\n", lua_gettop(l));
		bool r = LuaHelper::RunScriptFile(l, luapath, 1);
		printf("stack after run: %d\n", lua_gettop(l));
		{
			// parse table with STree object
			STree tree;
			if (lua_istable(l, -1)) {
				tree.ParseLua(l);
				printf("parsing result:\n%s\n", tree.NodeToString().c_str());
			}
			else {
				printf("invalid argument(not table), cannot parse.\n");
			}
			lua_pop(l, 1);
		}
		printf("stack finally: %d\n", lua_gettop(l));
		LUA->Release(l);
		return r;
	}
#endif
}