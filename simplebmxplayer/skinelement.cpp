#include "skinelement.h"

void SkinElement::AddSrc(ImageSRC &src) {
	this->src = src;
}

void SkinElement::AddDst(ImageDST &dst) {
	this->dst.push_back(dst);
}

// --------------------------------------------------------

void ImageSRC::ToRect(SDL_Rect &r) {
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
}

void ImageDST::ToRect(SDL_Rect &r) {
	r.x = x;
	r.y = y;
	r.w = w;
	r.h = h;
}

// ---------------------------------------------------------

void SkinElement::Draw(SDL_Renderer *renderer, ImageSRC &src, ImageDST &dst) {
	SDL_Rect _s, _d;
	src.ToRect(_s);	dst.ToRect(_d);
	if (img) {
		SDL_RenderCopy(renderer, img->GetPtr(), &_s, &_d);
	}
}

void SkinElement::Draw(SDL_Renderer *renderer) {
	// TODO: calculate time/pos
}

// ---------------------------------------------------------

SkinNumberElement::SkinNumberElement(SkinElement &e) : SkinElement(e) {}

void SkinNumberElement::Draw(SDL_Renderer *renderer) {
	// TODO
}

// ---------------------------------------------------------

SkinElementGroup::SkinElementGroup() {}
SkinElementGroup::SkinElementGroup(SkinElement *e) {
	elements.push_back(e);
}