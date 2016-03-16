#pragma once

/*
 * @description
 * Manages device input.
 */

#include "global.h"
#include "Timer.h"
#include <vector>

namespace INPUTSYS {
	const int EXIT = 0;
}

class InputReceiver;

class InputManager {
protected:
	// time to block input
	uint32_t m_BlockInputTime;
	Timer m_InputTimer;

	// receivers
	std::vector<InputReceiver*> m_Receivers;

	void OnSys(int code);
	void OnPress(int code);
	void OnDown(int code);
	void OnUp(int code);
	void OnMouseDown(int x, int y);
	void OnMouseMove(int x, int y);		// consider this method as dragging
	void OnMouseUp(int x, int y);
	void OnMouseDrag(int dx, int dy);	// only for real mouse object
	void OnMouseWheel(uint32_t delta);
	void OnMouseRDown(int x, int y);

	bool Initalize();
	void Release();
public:
	InputManager();
	~InputManager();

	void Update();
	void BlockInput(uint32_t msec);		// blocks input for a while

	void Register(InputReceiver* recv);
	void UnRegister(InputReceiver* recv);
};

class InputReceiver {
public:
	InputReceiver();
	~InputReceiver();

	virtual void OnSys(int code) {};
	virtual void OnPress(int code) {};
	virtual void OnDown(int code) {};
	virtual void OnUp(int code) {};
	virtual void OnMouseDown(int x, int y) {};
	virtual void OnMouseMove(int x, int y) {};
	virtual void OnMouseUp(int x, int y) {};
	virtual void OnMouseDrag(int dx, int dy) {};
	virtual void OnMouseWheel(uint32_t delta) {};
	virtual void OnMouseRDown(int x, int y) {};
};

extern InputManager* INPUT;