// Copyright 2017 Rodeo FX.  All rights reserved.

#include "CameraControler.h"

#include <cmath>
#include <iostream>

namespace walter_viewer
{

CameraControler::CameraControler()
{}

void CameraControler::reset()
{
    mPaning = false;

    mRotTheta = 0.f;
    mRotPhi = 0.f;
    mRotPsi = 0.f;

    mZoom = 0.f;

    mPanX = 0.f;
    mPanY = 0.f;

    mCenter[0] = 0.f;
    mCenter[1] = 0.f;
    mCenter[2] = 0.f;
}

void CameraControler::setRotation(float theta, float phi, float psi)
{
    mRotTheta = theta;
    mRotPhi = phi;
    mRotPsi = psi;
}

void CameraControler::updateRotation(float dx, float dy)
{
    mRotTheta += 0.333 * dx;
    mRotPhi += 0.333 * dy;
}

void CameraControler::updateZoom(float dx, float dy)
{
    mZoom = -0.003f * (dx + dy);
}

void CameraControler::updatePan(float x, float y)
{
    if (fabs(mPanX) - fabs(x) < 0.001)
    {
        mPanX = 0.f;
    }
    else
    {
        mPanX = x;
    }

    if (fabs(mPanY) - fabs(y) < 0.001)
    {
        mPanY = 0.f;
    }
    else
    {
        mPanY = y;
    }
}

void CameraControler::setCenter(float x, float y, float z)
{
    mCenter[0] = x;
    mCenter[1] = y;
    mCenter[2] = z;
}
}