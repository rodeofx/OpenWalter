VFX Platform Builder
===========================

### What is this repository for? ###

Many tools made for the VFX industry rely on the same C++ libraries (like Boost, Qt, OpenImageIO, etc...).
To be able to use the same tools across multiple DCCs (Digital Content Creation applications), they must
link with those libraries using compatible versions.

The VFX Reference Platform is a list of versions that should be used as one 'bundle of VFX libraries'.

Main DCC vendors are now (trying) to use the same bundle each year (see [vfxplatform.com](http://www.vfxplatform.com/)).
For example, if you need to build tools for Maya 2016 or Houdini 15, you should use the CY2015 Reference.
Unfortunately as the VFX Reference Platform is still new, not everyone is using it 'as is' yet (but it is improving every years!).
For example, Pixar USD is using a set of versions very close to the CY2015 Reference... but not exactly the same.

This "repo" is to store the build system to build a specific reference platform for Walter.

### System requirements ###

Check that the following libraries are installed:
```
nasm
libXxf86vm-devel
openssl-devel
mesa-libGL-devel
mesa-libGLU-devel
libXcursor-devel
libXmu-devel
libXi-devel
```

### How to build a reference platform? ###

From the root of this repo, checkout the branch or tag you need and run `make help` to get all the info you need.
