#include "camera.h"
#include <cmath>



Camera::Camera(float fovy, float aspect, float n, float f, const XMVECTOR& top)
{
	mTop = top;
	mProjectionMatrix = XMMatrixPerspectiveFovRH(fovy, aspect, n, f);
	update();
}

Camera::~Camera()
{
}


void Camera::onRMouseDown(const int& x, const int& y) {
	mMouseMoving = true;
	mCurrentMouseX = x;
	mCurrentMouseY = y;
}

void Camera::onRMouseUp(const int& x, const int& y) {
	mMouseMoving = false;
}

void Camera::onKeyDown(const uint32_t& key) {
	switch (key) {
	case KEY_W:
		mMoveState |= MOVE_FRONT;
		break;
	case KEY_D:
		mMoveState |= MOVE_RIGHT;
		break;
	case KEY_A:
		mMoveState |= MOVE_LEFT;
		break;
	case KEY_S:
		mMoveState |= MOVE_BACK;
		break;
	}
}

void Camera::onKeyUp(const uint32_t& key)
{
	switch (key) {
	case KEY_W:
		mMoveState &= ~MOVE_FRONT;
		break;
	case KEY_D:
		mMoveState &= ~MOVE_RIGHT;
		break;
	case KEY_A:
		mMoveState &= ~MOVE_LEFT;
		break;
	case KEY_S:
		mMoveState &= ~MOVE_BACK;
		break;
	}
}

void Camera::onMouseMove(const int& x, const int& y) {
	if (mMouseMoving) {
		int xOffset = x - mCurrentMouseX;
		int yOffset = y - mCurrentMouseY;

		mCurrentMouseX = x;
		mCurrentMouseY = y;

		pitch(-yOffset);
		yaw(xOffset);
	}
}

void Camera::update() {
	XMVECTOR moveDirection = { 0.0f, 0.0f, 0.0f };

	XMVECTOR front = mFront;
	XMVECTOR right = XMVector3Normalize(XMVector3Cross(mFront, mTop));

	if (mMoveState & MOVE_FRONT) {
		moveDirection += front;
	}

	if (mMoveState & MOVE_BACK) {
		moveDirection += -front;
	}

	if (mMoveState & MOVE_RIGHT) {
		moveDirection += right;
	}

	if (mMoveState & MOVE_LEFT) {
		moveDirection += -right;
	}

	if (XMVectorGetX(XMVector3LengthSq(moveDirection)) != 0) {
		moveDirection = XMVector3Normalize(moveDirection);
		mPosition += moveDirection * mSpeed;
	}

	mViewMatrix = XMMatrixLookAtRH(mPosition, mPosition + mFront, mTop);
}

void Camera::setPose(const XMVECTOR& position, const XMVECTOR& target) {
	const XMVECTOR direction = target - position;
	if (XMVectorGetX(XMVector3LengthSq(direction)) <= 0.0f) {
		return;
	}
	mPosition = position;
	mFront = XMVector3Normalize(direction);
	mPitch = std::asin(std::clamp(XMVectorGetX(mFront), -1.0f, 1.0f)) * 180.0f / static_cast<float>(XM_PI);
	mYaw = std::atan2(XMVectorGetZ(mFront), XMVectorGetX(mFront)) * 180.0f / static_cast<float>(XM_PI);
	mViewMatrix = XMMatrixLookAtRH(mPosition, target, mTop);
}

void Camera::pitch(int yOffset) {
	mPitch += yOffset * mSensitivity;

	if (mPitch >= 89.0f) {
		mPitch = 89.0f;
	}

	if (mPitch <= -89.0f) {
		mPitch = -89.0f;
	}

	const float pitchRadians = XMConvertToRadians(mPitch);
	const float yawRadians = XMConvertToRadians(mYaw);
	mFront = XMVectorSet(
		std::cos(yawRadians) * std::cos(pitchRadians),
		std::sin(pitchRadians),
		std::sin(yawRadians) * std::cos(pitchRadians),
		0.0f
	);
	mFront = XMVector3Normalize(mFront);
}

void Camera::yaw(int xOffset) {
	mYaw += xOffset * mSensitivity;

	if (mYaw >= 360.0f) {
		mYaw -= 360.0f;
	}
	if (mYaw <= -360.0f) {
		mYaw += 360.0f;
	}


	const float pitchRadians = XMConvertToRadians(mPitch);
	const float yawRadians = XMConvertToRadians(mYaw);
	mFront = XMVectorSet(
		std::cos(yawRadians) * std::cos(pitchRadians),
		std::sin(pitchRadians),
		std::sin(yawRadians) * std::cos(pitchRadians),
		0.0f
	);
	mFront = XMVector3Normalize(mFront);
}
