// Copyright 2017 Rodeo FX.  All rights reserved.

#ifndef __WALTERVIEWER_TYPES_H__
#define __WALTERVIEWER_TYPES_H__

#include <memory>

namespace walter_viewer
{

class FreeCamera;
typedef std::shared_ptr<FreeCamera> FreeCameraPtr;

class Scene;
typedef std::shared_ptr<Scene> ScenePtr;

class View;
typedef View* ViewPtr;


} // end namespace walter_viewer
#endif
