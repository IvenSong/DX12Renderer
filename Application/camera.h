#pragma once
#include "Common/base.h"
#include "inputHandler.h"


#define NO_MOVE 0
#define MOVE_LEFT 0x001
#define MOVE_RIGHT 0x002
#define MOVE_FRONT 0x004
#define MOVE_BACK 0x008

using namespace DirectX;


// 1. receive input (mouse/keyboard)
// 2. change state
// 3. call update() every frame to update viewMatrix

class Camera : public InputHandler {
public:


	Camera(float fovy, float aspect, float n, float f, const XMVECTOR& top);

	~Camera() override;

	// Event respond

	void onRMouseDown(const int& x, const int& y) override;

	void onRMouseUp(const int& x, const int& y) override;

	void onKeyDown(const uint32_t& key) override;

	void onKeyUp(const uint32_t& key) override;

	void onMouseMove(const int& x, const int& y) override;

	void update();
	void setPose(const XMVECTOR& position, const XMVECTOR& target);

	XMMATRIX getViewMatrix() const { return mViewMatrix; }

	XMMATRIX getProjectionMatrix() const { return mProjectionMatrix; }

private:
	void pitch(int yOffset);

	void yaw(int xOffset);

private:
	XMMATRIX mViewMatrix;
	XMMATRIX mProjectionMatrix;

	XMVECTOR mPosition{ 0.0f,0.0f,3.0f }; // current position
	XMVECTOR mFront{ 0.0f,0.0f,-1.0f };   // space direction
	XMVECTOR mTop{ 0.0f,1.0f,0.0f };      // up direction
	float mSpeed = { 0.01f };

	float mPitch{ 0.0f };            // elevation
	float mYaw{ -90.0f };            // angle in y axis
	float mSensitivity{ 0.1f };      // sensitivity of mouse changing angles

	uint32_t mMoveState{ NO_MOVE };
	bool	 mMouseMoving{ false };
	int		 mCurrentMouseX{ 0 };
	int		 mCurrentMouseY{ 0 };
};
