#include "skin.h"
#include "skinconverter.h"
#include "skinutil.h"
using namespace SkinUtil;

// ---------------------------------------------------------

/*
 * Creates render tree for rendering
 * This also cuts and reduces class conditions
 */

bool Skin::Load(const char *filepath) {
	if (skinlayout.LoadFile(filepath) != 0) {
		return false;
	}
	return true;
}

bool Skin::Save(const char *filepath) {
	return (skinlayout.SaveFile(filepath) == XML_SUCCESS);
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

	class XmlToLuaConverter {
	private:
		// head: values which will be logically interpreted to create body
		std::string head;
		std::string body;
		// current indentation
		int indent;
		// current parse depth (now useless?)
		int depth;
		// current object id
		int objid;
	public:
		void AppendIndentBody(int indent) {
			for (int i = 0; i < indent; i++)
				body.push_back('\t');
		}
		void AppendBody(const std::string& str) {
			AppendIndentBody(indent);
			body.append(str);
			body.append("\n");
		}
		void AppendHead(const std::string& str) {
			head.append(str);
			head.append("\n");
		}
		// process single SRC
		void AppendSRC(const XMLElement *e) {
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
					body.append(ssprintf("%s:\"%s\",", name, attr->Value()));
				else
					body.append(ssprintf("%s:%s,", name, attr->Value()));
			}
			if (body.back() == ',') body.pop_back();
			body.append("};\n");
		}
		// process single DST
		void AppendDST(const XMLElement *dst) {
			if (!dst) return;
			// timer is get changed into event handler
			const char *timer = dst->Attribute("timer");
			if (!timer)
				timer = "Init";
			AppendIndentBody(indent);
			body.append(ssprintf("On%s=DST(\n", timer));
			indent++;
			// add basic dst attribute (loop, acc, rotatecenter, blend)
			AppendIndentBody(indent);
			body.append("{");
			if (dst->Attribute("loop"))
				body.append(ssprintf("loop:%s", dst->Attribute("loop")));
			if (body.back() != '{')
				body.append(",");
			if (dst->Attribute("acc"))
				body.append(ssprintf("acc:%s", dst->Attribute("acc")));
			body.append("};\n");
			for (const XMLElement *f = dst->FirstChildElement();
				f;
				f = f->NextSiblingElement())
			{
				AppendIndentBody(indent);
				body.append("{");
				for (const XMLAttribute *attr = f->FirstAttribute();
					attr;
					attr = attr->Next())
				{
					const char *aname = attr->Name();
					if (strcmp(aname, "timer") == 0)
						body.append(ssprintf("%s:\"%s\",", aname, attr->Value()));
					else
						body.append(ssprintf("%s:%s,", aname, attr->Value()));
				}
				body.pop_back();
				body.append("};\n");
			}
			indent--;
			body.pop_back();
			body.append(");\n");
		}
		void AppendComment(const XMLComment *cmt) {
			AppendIndentBody(indent);
			const char *pn, *p = cmt->Value();
			do {
				pn = strchr(p, '\n');
				std::string l;
				if (!pn)
					l = p;
				else
					l = std::string(p, pn-p);
				body.append(ssprintf("-- %s\n", l.c_str()));
				p = pn+1;
			} while (pn);
		};
		// process general object/groups
		void AppendObject(const XMLElement *e, const char* name) {
			// some object - like playfield / combo - is quite nasty.
			// take care of them carefully.
			AppendIndentBody(indent);
			body.append(ssprintf("Def.%s(\"%d\") .. {\n", name, e->IntAttribute("resid")));
			indent++;
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
		void AppendElement(const XMLElement *e) {
			const char* name = e->Name();
			if (strcmp(name, "skin") == 0) {
				Parse(e->FirstChild());
			}
			// in case of include
			//
			else if (strcmp(name, "include") == 0) {
				AppendBody(ssprintf("LoadActor(\"%s\");", e->Attribute("path")));
			}
			// in case of resource,
			// parse it and register it as cached resource
			else if (strcmp(name, "image") == 0) {
				std::string r = ssprintf("LoadImageResource(\"%s\", {path=\"%s\"});",
					e->Attribute("name"), e->Attribute("path"));
				if (depth == 0)
					AppendHead(r);
				else
					AppendBody(r);
			}
			else if (strcmp(name, "font") == 0) {
				std::string r = ssprintf("LoadFontResource(\"%s\", {path=\"%s\", size=%s, border=%s});",
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
				AppendHead(ssprintf("LoadTFontResource(\"%s\", {data=texturefont_data});", e->Attribute("name")));
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
		void Parse(const XMLNode *node) {
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
		void StartParse(const XMLNode *node) {
			Clear();
			indent++;
			Parse(node);
			indent--;
		}
		std::string GetLuaCode() {
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
		void Clear() {
			indent = 0;
			objid = 0;
			head = "";
			body = "";
			indent = 0;
			depth = 0;
		};
	};
}

bool Skin::SaveToLua(const char* filepath) {
	// convert first
	XmlToLuaConverter *converter = new XmlToLuaConverter();
	converter->StartParse(skinlayout.FirstChild());		// (doc)->Skin->(element)
	std::string out = converter->GetLuaCode();
	delete converter;

	// write start
	FILE *f = SkinUtil::OpenFile(filepath, "w");
	if (!f) return false;
	fwrite(out.c_str(), 1, out.size(), f);
	fclose(f);
	return true;
}

void Skin::Release() {
	skinlayout.Clear();
}

Skin::Skin(): skinlayout(false) {}
Skin::~Skin() { Release(); }

// --------------------- Skin End --------------------------

SkinMetric::SkinMetric(): tree(false) {}

SkinMetric::~SkinMetric() { }

void SkinMetric::GetDefaultOption(SkinOption *o) {
	o->Clear();

	XMLElement *skin = tree.FirstChildElement("skin");
	XMLElement *option = skin->FirstChildElement("option");
	XMLElement *ele_switch = option->FirstChildElement("customswitch");
	while (ele_switch) {
		if (ele_switch->FirstChild()) {
			XMLElement *defaultoption = ele_switch->FirstChildElement();
			o->GetSwitches().push_back({ ele_switch->Attribute("name"), defaultoption->Attribute("value") });
		}
		ele_switch = ele_switch->NextSiblingElement("customswitch");
	}

	/*
	* CustomValue isn't supported in LR2 skin (dummy code here)
	*/
	XMLElement *ele_value = option->FirstChildElement("customvalue");
	while (ele_value) {
		if (ele_value->FirstChild()) {
			XMLElement *defaultoption = ele_value->FirstChildElement();
			o->GetValues().push_back({ ele_value->Attribute("name"), defaultoption->IntAttribute("value") });
		}
		ele_value = ele_value->NextSiblingElement("customvalue");
	}

	XMLElement *ele_file = option->FirstChildElement("customfile");
	while (ele_file) {
		o->GetFiles().push_back({ ele_file->Attribute("name"), ele_file->Attribute("path") });
		ele_file = ele_file->NextSiblingElement("customfile");
	}
}

bool SkinMetric::Save(const char *filepath) {
	return (tree.SaveFile(filepath) == XML_SUCCESS);
}