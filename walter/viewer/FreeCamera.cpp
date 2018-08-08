// Copyright 2017 Rodeo FX.  All rights reserved.

#include "FreeCamera.h"
#include "CameraControler.h"
#include <pxr/base/gf/frustum.h>

namespace walter_viewer
{

static float DEFAULT_FOCUS_DISTANCE = 5.0f;

FreeCamera::FreeCamera()
{
    this->SetFocusDistance(DEFAULT_FOCUS_DISTANCE);
}

void FreeCamera::updateZoom()
{
    float scaleFactor = 1.f + mControler.zoom();

    float dist = this->GetFocusDistance();

    if (scaleFactor > 1.f && dist < 2.f)
    {
        scaleFactor -= 1.0;
        dist += scaleFactor;
    }
    else
    {
        dist *= scaleFactor;
    }

    this->SetFocusDistance(dist);
}

void FreeCamera::updatePan(int height)
{
    // From usdView code
    auto frustrum = this->GetFrustum();
    auto camUp = frustrum.ComputeUpVector();
    auto camRight = GfCross(frustrum.ComputeViewDirection(), camUp);
    auto offRatio =
        frustrum.GetWindow().GetSize()[1] * this->GetFocusDistance() / height;

    mCenter += -offRatio * mControler.panX() * camRight;
    mCenter += offRatio * mControler.panY() * camUp;

    mControler.setPan(0.f, 0.f);
}

void FreeCamera::updateTumble()
{
    GfMatrix4d newTransfo;

    newTransfo.SetTranslate(GfVec3d::ZAxis() * this->GetFocusDistance());

    newTransfo *= GfMatrix4d().SetRotate(
        GfRotation(GfVec3d::ZAxis(), -mControler.rotPsi()));
    newTransfo *= GfMatrix4d().SetRotate(
        GfRotation(GfVec3d::XAxis(), -mControler.rotPhi()));
    newTransfo *= GfMatrix4d().SetRotate(
        GfRotation(GfVec3d::YAxis(), -mControler.rotTheta()));

    newTransfo *= GfMatrix4d().SetTranslate(mCenter);

    this->SetTransform(newTransfo);
}

void FreeCamera::reset()
{
    this->SetFocusDistance(DEFAULT_FOCUS_DISTANCE);

    GfMatrix4d newTransfo;
    newTransfo.SetIdentity();

    GfVec3d translate = GfVec3d::ZAxis() * this->GetFocusDistance();
    newTransfo.SetTranslateOnly(translate);

    this->SetTransform(newTransfo);
    mCenter = GfVec3d(0.0);
    mControler.reset();
}

const GfVec3d& FreeCamera::angles() const
{
    auto transform = this->GetTransform();
    transform.Orthonormalize();
    auto angles = transform.DecomposeRotation(
        GfVec3d::XAxis(), GfVec3d::YAxis(), GfVec3d::ZAxis());

    return angles;
}

} // end namespace walter_viewer
