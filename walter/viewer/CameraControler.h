// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERVIEWER_CAMERACONTROLER_H__
#define __WALTERVIEWER_CAMERACONTROLER_H__

#include <cstdio>

namespace walter_viewer
{

class CameraControler
{
public:
    CameraControler();

    float rotTheta() const { return mRotTheta; }
    float rotPhi() const { return mRotPhi; }
    float rotPsi() const { return mRotPsi; }
    float zoom() const { return mZoom; }
    float panX() const { return mPanX; }
    float panY() const { return mPanY; }

    void setRotation(float theta, float phi, float psi);
    void updateRotation(float dx, float dy);

    void setZoom(float value) { mZoom = value; }
    void updateZoom(float dx, float dy);

    bool isPanning() const { return mPaning; }
    void setPanning(bool value) { mPaning = value; }
    void setPan(float x, float y)
    {
        mPanX = x;
        mPanY = y;
    }
    void updatePan(float x, float y);
    void setCenter(float x, float y, float z);

    float const* center() const { return mCenter; }

    void reset();

    float mousePosLastX{0.f};
    float mousePosLastY{0.f};

private:
    // state
    bool mPaning{false};

    // cam rotation angles
    float mRotTheta{0.f};
    float mRotPhi{0.f};
    float mRotPsi{0.f};

    // cam zoom
    float mZoom{0.f};

    // cam pan
    float mPanX{0.f};
    float mPanY{0.f};

    float mCenter[3] = {0.f};
};

} // end namespace walter_viewer
#endif
