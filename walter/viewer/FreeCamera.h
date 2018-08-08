// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERVIEWER_FREECAMERA_H__
#define __WALTERVIEWER_FREECAMERA_H__

#include "Types.h"
#include "CameraControler.h"

#include <pxr/base/gf/camera.h>


PXR_NAMESPACE_USING_DIRECTIVE

namespace walter_viewer
{

class FreeCamera : public GfCamera
{
public:
    FreeCamera();
    void updateZoom();
	void updatePan(int height);
	void updateTumble();

	void reset();

	const GfVec3d& angles() const;
	const GfVec3d& center() const { return mCenter; }
	CameraControler& controler() { return mControler; }

private:
    GfVec3d mCenter{0.0};
    CameraControler mControler;
};

} // end namespace walter_viewer
#endif
