![Open Walter](https://www.rodeofx.com/uploads/images/tech/Walter_logo_mini-01.png)

[![Join the chat at https://gitter.im/OpenWalter/Lobby](https://badges.gitter.im/OpenWalter/Lobby.svg)](https://gitter.im/OpenWalter/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Walter is suite of plugins using USD for various DCCs and renderer.

It's main goal is to stay as much as possible in the [*USD Stage*](http://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-Stage) 
loaded through a DCC in order to increase the interactivity and limit the import/export time.

The DCC communicate with the *USD Stage* to access or modify specific [*USD prim*](http://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-Prim)
or properties. Modifications are stored in specific [*session layers*](http://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-SessionLayer) 
like for example transform, variant or visibility layers.

The [*Hydra Renderer*](http://graphics.pixar.com/usd/docs/USD-Glossary.html#USDGlossary-HydraRenderer) is used to display the stage in DCC like Houdini or Maya.

Walter provide a portable lookdev workflow. From Maya, artists can assign Arnold shaders directly on the *USD prims*
of the loaded stage. Those *lookdev edits* can then be exported as Alembic layers to be loaded
in Houdini or Katana.

Walter let you interact with prims transforms directly from Maya. It is *connecting* Maya locator(s) to the expose prim(s), in order
to push any static or animated transforms to the *USD stage*. This is the way to exchange layout and rigid animations between DCCs using Walter.

Walter include a modified version of the Embree raytracer for Hydra. It is able to blend any rendered prims with native Maya or Houdini objects displayed in the viewport.
This raytracer is dedicated to fast display. On scenes with millions of polygons, it will be much faster that the default Hydra renderer plugin (called *Stream*).
You can mix Walter nodes using *Embree* and *Stream* in the same scene. For example, the main character could be displayed using *Stream* and the environment using *Embree*.

Finally Walter extend USD itself by providing additional schemas (like WalterVolume or Expression), file format plugin (Arnold Scene Sources) and resolver (Alembic 1.7 CoreLayer allowing to load more than one *main file* in the stage).


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
| Arnold          | 5.0.2.4, 5.1.1.1 |
| Houdini         | 16.5.350, 16.5.496 |
| Katana          | 2.5v7, 3.0.2 |
| Maya            | 2016SP4, 2018.3 |

Walter is also a plugin for USD itself as it is including additional schemas, asset resolver and file format. 

Finally, the walterViewer application is a standalone viewer that can load Walter layers outside of any DCCs or renderer.
It is mostly used for development purpose, as it let you load layers without launching a DCC.


Trying Walter
------------

Compiled versions for Centos7 can be downloaded from (https://github.com/rodeofx/OpenWalter/releases).
You will need to add the plugins to your application environment.
Take a look at the Houdini, Katana, Kick and Maya wrappers in (https://github.com/rodeofx/OpenWalter/tree/dev/walter/utils)
to see how to get a proper configuration to load Walter plugins.


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

Render a list of USD or Alembic files. 
**Materials** and **assignations** can be loaded directly, including assignations using **expressions**.
Session layers like transforms, variants, purposes, etc... can be passed as parameters too.

You must set the Arnold root path in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| ARNOLD_ROOT       | The root path to Arnold install                                                                 | 5.0.2.4 5.1.1.1 |


##### Houdini Plugin

* *Walter SOP* node: 
It is using an **Hydra** display to visualize a stack of USD/Alembic layers in the viewport as **Packed Walter** primitives.
You can unpack them using standard Houdini Unpack node.

* *Walter Procedural OBJ* node:
Create a Walter procedural node in Arnold (via HtoA) to render USD/Alembic layers.
It is supporting layers coming from Walter for Maya (like materials, assignations, overrides or transforms).

You must set the Houdini and HtoA root paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| HOUDINI_ROOT      | The root path to Houdini install                                                                | 16.5.350 16.5.496 |
| HTOA_ROOT         | The root path to HtoA install                                                                   | 3.0.3 |


##### Katana Plugin

* *Walter_In* node:
Create a Katana scene graph from a stack of USD/Alembic/Arnold Scene Sources layers.
It is supporting materials, assignations, overrides and transforms layers coming from Walter for Maya.

You must set the Maya and MtoA root paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| KATANA_ROOT       | The root path to Katana install                                                                 | 2.5v7 3.0.2 |
| KTOA_ROOT         | The root path to KtoA install                                                                   | 2.1.2.0 |

##### Maya Plugin

* *Walter Standin* node:

From the WalterStandin Attribute Editor you can:
* open the Walter Tree Widget
* select any variants available in the stage.
* select the purpose to use in the viewport (proxy or render)
* switch between bounding box and full representatin (disabled in the open source version for the moment)
* choose the Hydra Renderer plugin

You can select *USD prims* directly from the viewport like a standard Maya object or from the Walter Tree Widget (aka the *Walter outliner*). 
You can drag and drop materials from the Hypershader directly on any prims in the Walter Tree Widget and override Arnold attributes.
Shader drag and drop can be done on Walter Expression items in the Tree Widget (using regular expression to assign shaders to prims).

By righ clicking on a prim in the Walter Tree Widget you can:
* show or hide prims
* expose prims transforms as Maya locators
* override prim purpose.


Finally edits can be saved on disk as specific layers.


*Walter Translator* for MtoA:

Create a Walter procedural node in Arnold to render every USD layers, including sessions layers (edits).


You must set the Maya and MtoA install paths in your environment.

| Variable Name     | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| MAYA_ROOT         | The root path to Maya install                                                                   | 2016SP4 2018.3 |
| MTOA_ROOT         | The root path to MtoA install                                                                   | 3.0.1.1 |

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


Contributing
------------

Since the open source version of Walter is in its early days, we did not yet created any forum or documentation pages.

[![Join the chat at https://gitter.im/OpenWalter/Lobby](https://badges.gitter.im/OpenWalter/Lobby.svg)](https://gitter.im/OpenWalter/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

