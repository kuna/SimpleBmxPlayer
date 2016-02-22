#include "Display.h"
#include "Surface.h"
#include <assert.h>
using namespace Display;





bool DisplaySDLGlew::Initialize(int width, int height) {
	m_glCtx = SDL_GL_CreateContext(m_SDL);
	if (!m_glCtx)
		return false;
	// set basic settings
	glClearColor(0, 0, 0, 0);
	// set basic ortho matrix & viewport
	SetResolution(width, height);
	// enable 2D texture & blending, basically.
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	// we don't use mipmap, so this setting is very important.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	return true;
}

// this function only changes viewport & projection matrix in fact.
// this DONT' change DISPLAY MODE!!
void DisplaySDLGlew::SetResolution(int width, int height) {
	m_width = width;
	m_height = height;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 10, -10);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

const char* DisplaySDLGlew::GetInfo() {
	return (const char*)glGetString(GL_VERSION);
}

int DisplaySDLGlew::GetErrorCode() {
	return glGetError();
};

const char* DisplaySDLGlew::GetErrorStr() {
	return (const char*)glewGetErrorString(glGetError());
};

Texture DisplaySDLGlew::CreateTexture(const Surface* surf) {
	Texture tex;
	glGenTextures(1, &tex.id);
	if (surf) {
		glBindTexture(GL_TEXTURE_2D, tex.id);
		tex.width = surf->GetWidth();
		tex.height = surf->GetHeight();
		glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGBA8,
			tex.width,
			tex.height,
			0,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
			surf->GetPtr());
	}
	return tex;
}

Texture DisplaySDLGlew::CreateEmptyTexture(int width, int height) {
	Texture tex;
	glGenTextures(1, &tex.id);
	glBindTexture(GL_TEXTURE_2D, tex.id);
	tex.width = width; tex.height = height;
	glTexImage2D(GL_TEXTURE_2D,
		0,
		GL_RGBA8,
		width,
		height,
		0,
		GL_RGBA,
		GL_UNSIGNED_BYTE,
		0);
	return tex;
}

void DisplaySDLGlew::UpdateTexture(Texture* tex, const Surface* surf, int x = 0, int y = 0) {
	assert(surf);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, surf->GetWidth(), surf->GetHeight(),
		GL_RGB, GL_UNSIGNED_BYTE, surf->GetPtr());
}

void DisplaySDLGlew::DeleteTexture(Texture* tex) {
	glDeleteTextures(1, &tex->id);
	tex->id = 0;
}





void DisplaySDLGlew::SetTexture(const Texture* tex) {
	m_Texture = *tex;
}

void DisplaySDLGlew::SetColorMod(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	m_ColorMod.r = r;
	m_ColorMod.g = g;
	m_ColorMod.b = b;
	m_ColorMod.a = a;
}

void DisplaySDLGlew::SetSRC(const Rect* r) {
	m_SRC = *r;
}

void DisplaySDLGlew::SetDST(const Rect* r) {
	m_DST = *r;
	// negative height isn't allowed in LR2
	if (m_DST.h < 0) {
		m_DST.y += m_DST.h;
		m_DST.h *= -1;
	}
	if (m_DST.w < 0) {
		m_DST.x += m_DST.w;
		m_DST.w *= -1;
	}
}

void DisplaySDLGlew::SetZPos(float z) {
	m_ZPos = z;
}

void DisplaySDLGlew::SetupVertices() {
	// set vertex/color/texture quad vertices.
	// TODO: move them to Display class
	// COMMENT: rendering position should be relative to center pos!
	v_Vertex[0 * 3 + 0] =
		v_Vertex[2 * 3 + 0] =
		m_DST.x - m_Center.x;
	v_Vertex[1 * 3 + 0] =
		v_Vertex[3 * 3 + 0] =
		m_DST.x + m_DST.w - m_Center.x;
	v_Vertex[0 * 3 + 1] =
		v_Vertex[1 * 3 + 1] =
		m_DST.y - m_Center.y;
	v_Vertex[2 * 3 + 1] =
		v_Vertex[3 * 3 + 1] =
		m_DST.y + m_DST.h - m_Center.y;
	v_Vertex[0 * 3 + 2] =
		v_Vertex[1 * 3 + 2] =
		v_Vertex[2 * 3 + 2] =
		v_Vertex[3 * 3 + 2] =
		m_ZPos;

	//
	// Vertex num
	//
	// 0   1
	// 2   3

	// if src w , src h is zero?
	// -> then set it to 1.0f
	if (m_SRC.w == 0) m_SRC.w = m_Texture.width;
	if (m_SRC.h == 0) m_SRC.h = m_Texture.height;
	v_Texture[0 * 2 + 0] =
		v_Texture[2 * 2 + 0] =
		(float)m_SRC.x / m_Texture.width;
	v_Texture[1 * 2 + 0] =
		v_Texture[3 * 2 + 0] =
		(float)(m_SRC.x + m_SRC.w) / m_Texture.width;
	v_Texture[0 * 2 + 1] =
		v_Texture[1 * 2 + 1] =
		(float)m_SRC.y / m_Texture.height;
	v_Texture[2 * 2 + 1] =
		v_Texture[3 * 2 + 1] =
		(float)(m_SRC.y + m_SRC.h) / m_Texture.height;

	// tilt is processed in here, not matrix.
	if (m_Tilt.x != 0) {
		v_Vertex[2 * 3 + 0] += m_Tilt.x * m_Texture.width;
		v_Vertex[3 * 3 + 0] += m_Tilt.x * m_Texture.width;
	}
	if (m_Tilt.y != 0) {
		v_Vertex[1 * 3 + 1] += m_Tilt.y * m_Texture.height;
		v_Vertex[3 * 3 + 1] += m_Tilt.y * m_Texture.height;
	}

	// colormod
	for (int i = 0; i < 4; i++) {
		v_Color[i * 4 + 0] = m_ColorMod.r;
		v_Color[i * 4 + 1] = m_ColorMod.g;
		v_Color[i * 4 + 2] = m_ColorMod.b;
		v_Color[i * 4 + 3] = m_ColorMod.a;
	}

	// set GL
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, v_Vertex);
	glEnableClientState(GL_COLOR_ARRAY);
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, v_Color);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 0, v_Texture);
}

void DisplaySDLGlew::SetBlendMode(int blend) {
	glEnable(GL_BLEND);
	switch (blend) {
	case 0:
		/*
		* LR2 is strange; NO blending does BLENDING, actually.
		*
		SDL_SetTextureBlendMode(img->GetPtr(), SDL_BlendMode::SDL_BLENDMODE_NONE);
		break;
		*/
	case 1:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_BLEND);
		break;
	case 2:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_ADD);
		break;
	case 3:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case 4:
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_MOD);
		break;
	default:
		//SDL_SetTextureBlendMode(t, SDL_BlendMode::SDL_BLENDMODE_BLEND);
		break;
	}
}

void DisplaySDLGlew::SetCenter(int x, int y) {
	m_Center.x = x;
	m_Center.y = y;
}

void DisplaySDLGlew::SetRotateX(float angle) {
	m_Rotation.x = angle;
}

void DisplaySDLGlew::SetRotateY(float angle) {
	m_Rotation.y = angle;
}

void DisplaySDLGlew::SetRotateZ(float angle) {
	m_Rotation.z = angle;
}

void DisplaySDLGlew::SetTilt(float sx, float sy) {
	m_Tilt.x = sx;
	m_Tilt.y = sy;
}

void DisplaySDLGlew::SetOffset(int x, int y) {

}

void DisplaySDLGlew::SetZoom(float zx, float zy) {
	m_Zoom.x = zx;
	m_Zoom.y = zy;
}

void DisplaySDLGlew::SetClip(const Display::Rect* r) {

}

void DisplaySDLGlew::PushState() {
	// matrix modification - rotation / zoom
	// COMMENT: modify tilt / color?
	glPushMatrix();
	glTranslatef(-m_Center.x, -m_Center.y, 0);
	glRotatef(m_Rotation.x / 360.0f, 1, 0, 0);
	glRotatef(m_Rotation.y / 360.0f, 0, 1, 0);
	glRotatef(m_Rotation.z / 360.0f, 0, 0, 1);
	glScalef(m_Zoom.x, m_Zoom.y, 1);
	glTranslatef(m_Center.x, m_Center.y, 0);
}

void DisplaySDLGlew::PopState() {
	glPopMatrix();
}

void DisplaySDLGlew::ResetDrawing() {
	// this won't reset SRC/DST.
	// + Blending
	m_Tilt.x = 0;
	m_Tilt.y = 0;
	m_Zoom.x = 1;
	m_Zoom.y = 1;
	m_Rotation.x = 0;
	m_Rotation.y = 0;
	m_Rotation.z = 0;
	m_Center.x = 0;
	m_Center.y = 0;
	m_ColorMod.a = 1;
	m_ColorMod.r = 1;
	m_ColorMod.g = 1;
	m_ColorMod.b = 1;
	m_Texture.id = 0;
	m_ZPos = 1.0f;
}

void DisplaySDLGlew::DrawPrimitives() {
	// is valid texture?
	if (m_Texture.id == 0)
		return;

	// apply vertices
	SetupVertices();

	// render
	glBindTexture(GL_TEXTURE_2D, m_Texture.id);
	DrawPrimitives();
}







namespace RenderHelper {
	void Render(IDisplay *d, Texture* tex, const Rect* src, const Rect* dst) {
		d->ResetDrawing();
		d->SetTexture(tex);
		d->SetSRC(src);
		d->SetDST(dst);
		d->DrawPrimitives();
	}
}