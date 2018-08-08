Open Walter
===========================

Walter is an open source suite of plugins using USD for various DCCs and renderer.


Supported Platforms
-------------------

Open Walter is currently supported on Linux platforms and has been built
and tested on CentOS 7.

If you are interested to contribute to a Windows build, we will be more than
happy to support you!

Supported DCC and renderers
-------------------

The following versions were used in production. 

| Application     | Version   |
| ----------------| --------- |
| Arnold          | 4.2.16.2, 5.0.2.4, 5.1.1.1 |
| Houdini         | 16.5.350, 16.5.496 |
| Katana          | 2.5v7, 3.0.2 |
| Maya            | 2016SP4, 2018.3 |

Walter is also a plugin for USD itself as it is including additional schemas, asset resolver and file format. 

Finally, the walterViewer application is a standalone viewer that can load Walter layers outside of any DCCs or renderer.
It is mostly used for development purpose, as it let you load layers without launching a DCC.


Building Walter
------------

### System requirements ###

Check that the following libraries are installed:
```
autoconf
libtool
libXcursor-devel
libXi-devel
libXinerama-devel
libXmu-devel
libXrandr-devel
libXt-devel
libXxf86vm-devel
mesa-libGL-devel
mesa-libGLU-devel
nasm
openssl-devel
```

### Optional Plugins

Open Walter comes with an Arnold Procedural and some DCC plugins.
They are enable by default. You can disable a plugin by using the following 
variable passed to make: ```BUILD_<NAME>_PLUGINS=OFF```.

For example to build only the Houdini plugin:

```make BUILD_KATANA_PLUGINS=OFF BUILD_MAYA_PLUGINS=OFF```

##### Arnold Procedural

*walter* node:

Render a list of USD or Alembic files. 
Walter **materials** and **assignations** can be loaded directly, including assignations using Walter Expression.
Session layers like transforms, variants, purposes, etc... can be passed as parameters too.

You must set the Arnold install path in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| ARNOLD_ROOT       | The root path to Arnold install                                                                 | 4.2.16.2 5.0.2.4 5.1.1.1 |


##### Houdini Plugin

*Walter SOP* node: 

It is using an **Hydra** display to visualise a stack of USD/Alembic layers in the viewport as **Packed Walter** primitives.
You can unpack them using standard Houdini Unpack node.

*Walter Procedural OBJ* node:
Create a Walter procedural node in Arnold (via HtoA) to render USD/Alembic layers.
It is supporting layers comming from Walter for Maya (like materials, assignations, overrides or transforms).

You must set the Houdini and HtoA install paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| HOUDINI_ROOT      | The root path to Houdini install                                                                | 16.5.350 16.5.496 |
| HTOA_ROOT         | The root path to HtoA install                                                                   | 2.2.4  3.0.3 |


##### Katana Plugin

*Walter_In* node:

Create a Katana scene graph from a stack of USD/Alembic layers.
It is supporting materials, assignations, overrides and transforms layers comming from Walter for Maya.

You must set the Maya and MtoA install paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| KATANA_ROOT       | The root path to Katana install                                                                 | 2.5v7 3.0.2 |
| KTOA_ROOT         | The root path to KtoA install                                                                   | 2.0.4.0 2.0.8.0 2.1.2.0 |

##### Maya Plugin

*Walter Standin* node:

It is using an **Hydra** display to visualise a stack of USD/Alembic layers in the viewport as one Maya shape node.
You can select USD prims from the viewport or from the Walter tree widget. 
You can modify the loaded USD stage, like adding materials, assignations or attributes. You can animate USD prims
by exposing their transforms to Maya as locator objects. You can select any variants available in the composed stage.
Finally all those edits can be saved on disk in specific layers.

*Walter Translator* for MtoA:

Create a Walter procedural node in Arnold to render every USD layers, including sessions layers (edits).


You must set the Maya and MtoA install paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| MAYA_ROOT         | The root path to Maya install                                                                   | 2016SP4 2018.3 |
| MTOA_ROOT         | The root path to MtoA install                                                                   | 1.4.2  2.0.2.2 3.0.1.1 |

For example:
```
# Open Walter environment to source
export KTOA_ROOT=/software/ktoa/ktoa-2.1.0.0
export KATANA_ROOT=/software/katana/katana3.0.v1
export ARNOLD_ROOT=/software/arnold/5.1.1.1
export HOUDINI_ROOT=/software/houdini/16.5.350
export HTOA_ROOT=/software/htoa/htoa-3.0.3
export MTOA_ROOT=/software/mtoa-3.0.1.1
export MAYA_ROOT=/software/maya/Maya2018.3/maya2018-x64
```
##### Build and install command

The simpliest way is to configure your environment and then run 
 ```make```.

Note that you can also use variable assignement directly in the make command for any install paths:
```make HOUDINI_ROOT=houdini/install/dir``` 

The very first build will take some time. This is because it will download the
required dependencies and build and install them along with Walter.

By default all the libraries (Boost, Qt, USD, etc...) will be installed in build/lib. You can change
the install path by setting ```PREFIX_ROOT```.

Once the build is finished, you can use houdini/katana/maya or kick launchers located 
in the walter/utils directory. They are using the same environement variables than the build
to locate Houdini, Katana and Maya. Those launchers are adding Walter plugins in order to quickly give it a try!

If you run ```make clean && make``` it will re-build only Walter (and so it will be much faster).

##### Build Options

Run ```make help``` for more info on the various targets available.
