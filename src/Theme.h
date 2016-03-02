#pragma once

#define _USEPOOL				// to use SetEnvironmentFromOption()
#include "Pool.h"
#include "skin.h"

class Actor;
typedef Actor* (*CreateActorFn)();


/*
 * @description
 * manages to load & render skin data
 *
 * This class SHOULD NOT run in multiple time,
 * because that may cause collsion with lua variable.
 * (Although lua will be automatically locked ...)
 */
class Theme {
protected:
	// skin related information
	Skin m_Skin;
	SkinMetric m_Skinmetric;
	SkinOption m_Skinoption;

	// rendering object
	Actor* base_;
public:
	Actor* GetPtr();
	void Update();
	void Render();
	bool Load(const char* skinname);
	void ClearElements();

	// create actor from element
	static Actor* MakeActor(const tinyxml2::XMLElement* e, Actor* parent = 0);
	// register actor generate func
	static void RegisterActor(const std::string& name, CreateActorFn fn);
	// used when pool's resource are being cleared
	static void ClearAllResource();

	static Display::Texture* GetTexture(const std::string id);
	static Font* GetFont(const std::string id);
};