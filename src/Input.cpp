#include "Input.h"
#include "SDL/SDL.h"
#include "Logger.h"
#include "Setting.h"

namespace {
	// SDL
	SDL_Joystick*	JOYSTICK[10] = { 0, };
	int				nJoystickCnt = 0;

	// check joystick input status
	const int Joystick_Code_Max = 900;
	bool pressing[Joystick_Code_Max];

	// returns false if it's first pressing
	inline void JoyPress(int code) {
		pressing[code % Joystick_Code_Max] = true;
	}
	inline bool IsJoyPressing(int code) {
		return pressing[code % Joystick_Code_Max];
	}
	inline void JoyUp(int code) {
		pressing[code % Joystick_Code_Max] = false;
	}

	// mouse coordinate save
	int mx, my;
}






/* COMMENT: use class as implementation? */

InputManager::InputManager() { Initalize(); }

InputManager::~InputManager() { Release(); }

bool InputManager::Initalize() {
	m_BlockInputTime = 1000;
	m_InputTimer.Start();

	nJoystickCnt = SDL_NumJoysticks();
	if (nJoystickCnt > 10) nJoystickCnt = 10;
	for (int i = 0; i < nJoystickCnt; i++) {
		JOYSTICK[i] = SDL_JoystickOpen(i);
	}

	LOG->Info("Found %d joysticks.", nJoystickCnt);
	return true;
}

void InputManager::Release() {
	for (int i = 0; i < nJoystickCnt; i++)
		SDL_JoystickClose(JOYSTICK[i]);
}

void InputManager::Update() {
	/*
	 * Keybd, mouse event
	 */
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		// pop all pending events if input timer isn't suitable
		if (m_InputTimer.GetTick() < m_BlockInputTime)
			continue;

		if (e.type == SDL_QUIT) {
			OnSys(INPUTSYS::EXIT);
		}
		else if (e.type == SDL_KEYDOWN) {
			if (e.key.repeat == 0)
				OnDown(e.key.keysym.scancode);
			OnPress(e.key.keysym.scancode);
		}
		else if (e.type == SDL_KEYUP) {
			OnUp(e.key.keysym.scancode);
		}
		else if (e.type == SDL_JOYBUTTONDOWN) {
			//e.jbutton.which
			int id = 1 + e.jbutton.which * 1000 + e.jbutton.button;
			if (!IsJoyPressing(id))
				OnDown(id);
			OnPress(id);
			JoyPress(id);
		}
		else if (e.type == SDL_JOYBUTTONUP) {
			int id = 1 + e.jbutton.which * 1000 + e.jbutton.button;
			OnUp(id);
			JoyUp(id);
		}
		else if (e.type == SDL_JOYAXISMOTION) {
			// 0: left / right
			// - 1020: left
			// - 1021: right
			// 1/2: up / down
			// - 1022: up
			// - 1023: down
#define JOYSTICKPRESS(id)\
	if (!IsJoyPressing(id)) { OnDown(id); } OnPress(id); JoyPress(id);
#define JOYSTICKUP(id)\
	if (IsJoyPressing(id)) { OnUp(id); JoyUp(id); }
			if (e.jaxis.axis == 0) {
				if (e.jaxis.value >= 8000) {
					JOYSTICKPRESS(1020);
					JOYSTICKUP(1021);
				}
				else if (e.jaxis.value <= -8000) {
					JOYSTICKUP(1020);
					JOYSTICKPRESS(1021);
				}
				else {
					JOYSTICKUP(1020);
					JOYSTICKUP(1021);
				}
			}
			else {
				if (e.jaxis.value >= 8000) {
					JOYSTICKPRESS(1022);
					JOYSTICKUP(1023);
				}
				else if (e.jaxis.value <= -8000) {
					JOYSTICKUP(1022);
					JOYSTICKPRESS(1023);
				}
				else {
					JOYSTICKUP(1022);
					JOYSTICKUP(1023);
				}
			}
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN) {
			if (e.button.button == SDL_BUTTON_LEFT) {
				OnMouseDown(e.button.x, e.button.y);
				mx = e.button.x; my = e.button.y;
			}
		}
		else if (e.type == SDL_MOUSEBUTTONUP) {
			if (e.button.button == SDL_BUTTON_LEFT) {
				OnMouseUp(e.button.x, e.button.y);
			}
		}
		else if (e.type == SDL_MOUSEMOTION) {
			OnMouseMove(e.motion.x, e.motion.y);
			if (e.motion.state != 0) {
				OnMouseDrag(e.motion.x - mx, e.motion.y - my);
			}
			mx = e.motion.x; my = e.motion.y;
		}
		else if (e.type == SDL_MOUSEWHEEL) {
			OnMouseWheel(e.wheel.y);
		}
		else if (e.type == SDL_FINGERDOWN) {
			OnMouseDown(e.tfinger.x * SETTING.width, e.tfinger.y * SETTING.height);
		}
		else if (e.type == SDL_FINGERMOTION) {
			OnMouseMove(e.tfinger.x * SETTING.width, e.tfinger.y * SETTING.height);
			OnMouseDrag(e.tfinger.dx * SETTING.width, e.tfinger.dy * SETTING.height);
		}
		else if (e.type == SDL_FINGERUP) {
			OnMouseDown(e.tfinger.x * SETTING.width, e.tfinger.y * SETTING.height);
		}
	}
}

void InputManager::BlockInput(uint32_t msec) {
	m_BlockInputTime = msec;
	m_InputTimer.Start();
}

void InputManager::Register(InputReceiver* recv) {
	m_Receivers.push_back(recv);
}

#define ITER(it, obj) for (auto it = obj.begin(); it != obj.end(); ++it)
void InputManager::UnRegister(InputReceiver* recv) {
	ITER(it, m_Receivers) {
		if (*it == recv) {
			m_Receivers.erase(it);
			break;
		}
	}
}


void InputManager::OnSys(int code) {
	ITER(it, m_Receivers) {
		(*it)->OnSys(code);
	}
}
void InputManager::OnPress(int code) {
	ITER(it, m_Receivers) {
		(*it)->OnPress(code);
	}
}
void InputManager::OnDown(int code) {
	ITER(it, m_Receivers) {
		(*it)->OnDown(code);
	}
}
void InputManager::OnUp(int code) {
	ITER(it, m_Receivers) {
		(*it)->OnUp(code);
	}
}
void InputManager::OnMouseDown(int x, int y) {
	ITER(it, m_Receivers) {
		(*it)->OnMouseDown(x, y);
	}
}
void InputManager::OnMouseMove(int x, int y) {
	ITER(it, m_Receivers) {
		(*it)->OnMouseMove(x, y);
	}
}
void InputManager::OnMouseUp(int x, int y) {
	ITER(it, m_Receivers) {
		(*it)->OnMouseUp(x, y);
	}
}
void InputManager::OnMouseDrag(int x, int y) {
	ITER(it, m_Receivers) {
		(*it)->OnMouseMove(x, y);
	}
}
void InputManager::OnMouseWheel(uint32_t d) {
	ITER(it, m_Receivers) {
		(*it)->OnMouseWheel(d);
	}
}




InputReceiver::InputReceiver() { INPUT->Register(this); }

InputReceiver::~InputReceiver() { INPUT->UnRegister(this); }




InputManager* INPUT;