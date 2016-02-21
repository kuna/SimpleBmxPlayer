/*
 * @description
 * SkinRenderTree: creates proper render tree for each object.
 * Renderer-dependent.
 */
#pragma once

#include "global.h"
#include "Surface.h"
#include "timer.h"
#include "font.h"
#include "skin.h"
#include <vector>

#include "tinyxml2.h"
using namespace tinyxml2;

namespace ROTATIONCENTER {
	enum ROTATIONCENTER {
		TOPLEFT = 7,
		TOPCENTER = 8,
		TOPRIGHT = 9,
		CENTERLEFT = 4,
		CENTER = 5,
		CENTERRIGHT = 6,
		BOTTOMLEFT = 1,
		BOTTOMCENTER = 2,
		BOTTOMRIGHT = 3,
	};
}
namespace ACCTYPE {
	enum ACCTYPE {
		LINEAR = 0,
		ACCEL = 1,
		DECEL = 2,
		NONE = 3,
	};
}

#include "ActorBasic.h"
#include "ActorPlay.h"


/** @brief rendering tree which is used in real rendering - based on Xml skin structure */
class ThemeConstructor {
	ThemeConstructor(SkinObject);
};
class SkinRenderTree: public SkinGroupObject {
private:
	friend SkinRenderObject;
	friend SkinImageObject;
	friend SkinTextObject;
	friend SkinNoteObject;
	friend SkinNoteFieldObject;

	/** @brief stores all skin render objects */
	std::vector<SkinRenderObject*> _objpool;

	//
	//
	//
#if 0
	/** @brief resources used in this game */
	std::map<RString, Display::Texture> _imagekey;
	/** @brief resources used in this game */
	std::map<RString, Font*> _fontkey;
#endif

	/** @brief decide group object's texture size */
	int _scr_w, _scr_h;
	/** @brief skin key count (zero if not play skin) */
	int _keycount;

	/** @brief object's relative position */
	struct _Offset { int x, y; };
	std::vector<_Offset> _offset_stack;
	int _offset_x, _offset_y;

	/** @brief stores element ID-pointer. use GetElementById() to use this array. */
	std::map<RString, SkinRenderObject*> _idpool;
public:
	SkinRenderTree(int skinwidth, int skinheight);
	~SkinRenderTree();
	virtual void SetObject(XMLElement *info);
	virtual void Update();
	virtual void Render();

	/** @brief set skin's basic resolution. automatically set when you load skin. */
	void SetSkinSize(int skinwidth, int skinheight);
	/** @brief only releases all rendering object */
	void ReleaseAll();
	SkinRenderObject* GetElementById(RString &id);

	/** @brief returns skins width/height information */
	int GetWidth();
	int GetHeight();

	/** @brief offset */
	void PushRenderOffset(int x, int y);
	void PopRenderOffset();
	void ResetOffset() { _offset_stack.clear(); _offset_x = _offset_y = 0; }
	int GetOffsetX() { return _offset_x; }
	int GetOffsetY() { return _offset_y; }

	/** @brief object generation */
#define NEWOBJECT(n)\
	n* New##n() {\
		n* obj = new n(this);\
		_objpool.push_back(obj);\
		return obj; }
	NEWOBJECT(SkinUnknownObject);
	NEWOBJECT(SkinGroupObject);
	NEWOBJECT(SkinCanvasObject);
	NEWOBJECT(SkinImageObject);
	NEWOBJECT(SkinSliderObject);
	NEWOBJECT(SkinGraphObject);
	NEWOBJECT(SkinTextObject);
	NEWOBJECT(SkinNumberObject);
	NEWOBJECT(SkinBgaObject);
	NEWOBJECT(SkinScriptObject);

	NEWOBJECT(SkinNoteFieldObject);
	NEWOBJECT(SkinNoteObject);
	NEWOBJECT(SkinNoteLineObject);
	NEWOBJECT(SkinNoteJudgeLineObject);
	NEWOBJECT(SkinComboObject);
	NEWOBJECT(SkinGrooveGaugeObject);

	/** @brief load image at globalresources. */
	void RegisterImage(RString& id, RString& path);
	/** @brief load font at globalresources. */
	void RegisterTTFFont(RString& id, RString& path, int size, int border, const char* texturepath = 0);
	/** @brief load font at globalresources. (using texturefont raw data) */
	void RegisterTextureFont(RString& id, RString& path);
	/** @brief load font at globalresources. (using texturefont raw data) */
	void RegisterTextureFontByData(RString& id, RString& textdata);
	/** @brief release all resources */
	void ReleaseAllResources();
};

namespace SkinRenderHelper {
	void AddFrame(ImageDST &d, ImageDSTFrame &f);
	SDL_Rect ToRect(ImageSRC &r);
	SDL_Rect ToRect(ImageDSTFrame &r);
	bool CalculateFrame(ImageDST &dst, ImageDSTFrame &frame);
	ImageDSTFrame Tween(ImageDSTFrame& a, ImageDSTFrame &b, double t, int acctype);
	/** @brief in debug mode, border will drawn around object. */
	void Render(Display::Texture *tex, ImageSRC *src, ImageDSTFrame *frame, int blend = 1, int rotationcenter = ROTATIONCENTER::CENTER);

	/** @brief replaces path string to a correct one */
	//void ConvertPath(RString& path);
	/** @brief load Skin (path is relative one!) */
	bool LoadSkin(const char* path, Skin& skin);
	/** @brief constructs resid-object mapping and loads resources from XmlSkin. */
	void LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s);
	/** @brief constructs rendertree object from XmlSkin. must LoadResource first. */
	bool ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s);

	/** @brief register skin object for each tag to process */
	void RegisterActor(const char* name, SkinRenderObject *obj);

	/** @brief construct object including condition, SRC, DST. */
	void ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e);
	void ConstructSRCFromElement(ImageSRC &src, XMLElement *e);
	void ConstructDSTFromElement(ImageDST &dst, XMLElement *e);
}