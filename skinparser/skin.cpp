#include "skin.h"
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
	const char *objecttags[TAGMAXCOUNT] = {
		// generals
		"sprite", "text", "number", "graph", "button",
		// ingenerals
		"bga", "notefield", "groovegauge",
		"listview", 
	};
	const char* srctags[TAGMAXCOUNT] = {
		"SRC",
		"SRC_LN_NOTES",
	};
	const char* dsttags[TAGMAXCOUNT] = {
		"DST",
	};
	const char* attrtags[TAGMAXCOUNT] = {
		"DST",
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
		// current indentation
		int indent;
		// current parse depth
		int depth;
		// current object id
		int objid;
	public:
		void AppendIndentHead(int indent) {
			for (int i = 0; i < indent; i++)
				head.push_back('\t');
		}
		void AppendHead(const std::string& str) {
			AppendIndentHead(indent);
			head.append(str);
			head.append("\n");
		}
		// process single SRC
		void AppendSRC(const XMLElement *e) {
			AppendIndentHead(indent);
			head.append("Src={");
			for (const XMLAttribute *attr = e->FirstAttribute();
				attr;
				attr = attr->Next())
			{
				const char *name = attr->Name();
				if (strcmp(name, "resid") == 0 ||
					strcmp(name, "condition") == 0 ||
					strcmp(name, "length") == 0 ||
					strcmp(name, "value") == 0 ||
					strcmp(name, "align") == 0 ||
					strcmp(name, "edit") == 0)
					continue;
				if (strcmp(name, "timer") == 0)
					head.append(ssprintf("%s:\"%s\",", name, attr->Value()));
				else
					head.append(ssprintf("%s:%s,", name, attr->Value()));
			}
			if (head.back() == ',') head.pop_back();
			head.append("};\n");
		}
		// process single DST
		void AppendDST(const XMLElement *dst) {
			if (!dst) return;
			const char *timer = dst->Attribute("timer");
			if (!dst->Attribute("timer"))
				timer = "Init";
			AppendIndentHead(indent);
			head.append(ssprintf("On%s=DST(\n", timer));
			indent++;
			// add basic dst attribute (loop, acc)
			AppendIndentHead(indent);
			head.append("{");
			if (dst->Attribute("loop"))
				head.append(ssprintf("loop:%s", dst->Attribute("loop")));
			if (head.back() != '{')
				head.append(",");
			if (dst->Attribute("acc"))
				head.append(ssprintf("acc:%s", dst->Attribute("acc")));
			head.append("};\n");
			for (const XMLElement *f = dst->FirstChildElement();
				f;
				f = f->NextSiblingElement())
			{
				AppendIndentHead(indent);
				head.append("{");
				for (const XMLAttribute *attr = f->FirstAttribute();
					attr;
					attr = attr->Next())
				{
					const char *aname = attr->Name();
					if (strcmp(aname, "timer") == 0)
						head.append(ssprintf("%s:\"%s\",", aname, attr->Value()));
					else
						head.append(ssprintf("%s:%s,", aname, attr->Value()));
				}
				head.pop_back();
				head.append("};\n");
			}
			indent--;
			head.pop_back();
			head.append(");\n");
		}
		void AppendComment(const XMLComment *cmt) {
			AppendIndentHead(indent);
			const char *pn, *p = cmt->Value();
			do {
				pn = strchr(p, '\n');
				std::string l;
				if (!pn)
					l = p;
				else
					l = std::string(p, pn-p);
				head.append(ssprintf("-- %s\n", l.c_str()));
				p = pn+1;
			} while (pn);
		};
		// process general object/groups
		void AppendObject(const XMLElement *e, const char* name) {
			// some object - like playfield / combo - is quite nasty.
			// take care of them carefully.
			AppendIndentHead(indent);
			if (depth <= 1)		// 1 -> (skin) -> 2(base)
				head.append("t[#t+1] = ");
			head.append(ssprintf("Def.%s(\"res%d\", {\n", name, e->IntAttribute("resid")));
			indent++;
			// process attribute first
			const char *attrval;
			if (attrval = e->Attribute("condition"))
				AppendHead(ssprintf("class=\"%s\";", attrval));
			if (attrval = e->Attribute("value"))
				AppendHead(ssprintf("valuekey=\"%s\";", attrval));
			if (attrval = e->Attribute("align"))
				AppendHead(ssprintf("align=%s;", attrval));
			if (attrval = e->Attribute("edit"))
				AppendHead(ssprintf("edit=%s;", attrval));
			if (attrval = e->Attribute("length"))
				AppendHead(ssprintf("length=%s;", attrval));
			AppendSRC(e);
			// parse inner element
			Parse(e->FirstChildElement());
			indent--;
			AppendHead("});");
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
				AppendHead(ssprintf("t[#t+1] = LoadActor(\"%s\")", e->Attribute("path")));
			}
			// in case of resource,
			// parse it and register it as cached resource
			else if (strcmp(name, "image") == 0) {
				AppendHead(ssprintf("LoadImageResource(\"%s\", {path=\"%s\"})",
					e->Attribute("name"), e->Attribute("path")));
			}
			else if (strcmp(name, "texturefont") == 0) {
				AppendHead(ssprintf("-- converter message: use path rather than data attribute"));
				AppendHead(ssprintf("texturefont_data=[[%s]]", e->GetText()));
				AppendHead(ssprintf("LoadTFontResource(\"%s\", {data=texturefont_data})",
					e->Attribute("name")));
			}

			// in case of conditional clause
			else if (strcmp(name, "condition") == 0) {
				for (const XMLElement *cond = e->FirstChildElement();
					cond;
					cond = cond->NextSiblingElement())
				{
					if (strcmp(cond->Name(), "else") == 0) {
						AppendHead("else");
					}
					else {
						AppendHead(ssprintf("%s CND(\"%s\") then", cond->Name(), cond->Attribute("condition")));
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
				AppendHead("end");
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
					AppendHead(l.c_str());
					p = pn + 1;
				} while (pn);
			}
			// in case of group / object / src / dst
			else if (CheckTagGroup(name, objecttags)) {
				depth++;
				AppendObject(e, name);
				depth--;
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
			Parse(node);
		}
		std::string GetLuaCode() {
			return
				"-- skin table structure\n"
				"t = {}\n\n" +
				head +
				"\n" +
				"return t";
		};
		void Clear() {
			indent = 0;
			objid = 0;
			head = "";
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