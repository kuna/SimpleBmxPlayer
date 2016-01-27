/*
 * @description
 * SkinRenderTree: creates proper render tree for each object.
 * Renderer-dependent.
 */
#pragma once

#include "image.h"
#include "timer.h"
#include "font.h"
#include "skin.h"
#include <vector>

#include "tinyxml2.h"
using namespace tinyxml2;

#include "ActorBasic.h"
#include "ActorPlay.h"


/** @brief rendering tree which is used in real rendering - based on Xml skin structure */
class SkinRenderTree: public SkinGroupObject {
public:
	/** @brief stores all skin render objects */
	std::vector<SkinRenderObject*> _objpool;

	/** @brief resources used in this game */
	std::map<RString, Image*> _imagekey;
	/** @brief resources used in this game */
	std::map<RString, Font*> _fontkey;

	/** @brief decide group object's texture size */
	int _scr_w, _scr_h;

	/** @brief stores element ID-pointer. use GetElementById() to use this array. */
	std::map<RString, SkinRenderObject*> _idpool;
public:
	SkinRenderTree(int skinwidth, int skinheight);
	~SkinRenderTree();
	void SetSkinSize(int skinwidth, int skinheight);
	/** @brief only releases all rendering object */
	void ReleaseAll();
	int GetWidth();
	int GetHeight();
	SkinRenderObject* GetElementById(RString &id);

	SkinUnknownObject* NewUnknownObject();
	SkinGroupObject* NewGroupObject(bool clipping = false);
	SkinImageObject* NewImageObject();
	SkinSliderObject* NewSliderObject();
	SkinGraphObject* NewGraphObject();
	SkinGrooveGaugeObject* NewGrooveGaugeObject();
	SkinTextObject* NewTextObject();
	SkinNumberObject* NewNumberObject();
	SkinBgaObject* NewBgaObject();
	SkinScriptObject* NewScriptObject();

	//SkinLifeGraphObject* NewLifeGraphObject();
	SkinPlayObject* NewPlayObject();
	SkinComboObject* NewComboObject();

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
	void ConstructBasicRenderObject(SkinRenderObject *obj, XMLElement *e);
	SDL_Rect ToRect(ImageSRC &r);
	SDL_Rect ToRect(ImageDSTFrame &r);
	bool CalculateFrame(ImageDST &dst, ImageDSTFrame &frame);
	ImageDSTFrame Tween(ImageDSTFrame& a, ImageDSTFrame &b, double t, int acctype);
	/** @brief in debug mode, border will drawn around object. */
	void Render(Image *img, ImageSRC *src, ImageDSTFrame *frame, int blend = 1, int rotationcenter = ACTORROTATIONCENTER::CENTER);

	void PushRenderOffset(int x, int y);
	void PopRenderOffset();

	/** @brief replaces path string to a correct one */
	void ConvertPath(RString& path);
	/** @brief constructs resid-object mapping and loads resources from XmlSkin. */
	bool LoadResourceFromSkin(SkinRenderTree &rtree, Skin &s);
	/** @brief constructs rendertree object from XmlSkin. must LoadResource first. */
	bool ConstructTreeFromSkin(SkinRenderTree &rtree, Skin &s);
}