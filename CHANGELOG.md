# Change Log

## [1.1.1] - 2018-09-21

### Added
- Build system: Added option in vfx_platform_builder script to disable 
  requirement for Houdini or Katana plugins.
- Katana: Handle USD purposes in KatanaPlugin.
  "guide" and "proxy" are viewed in the viewport but not in renderer.
  "render" is viewed everywhere (viewport and render).
- Katana: Handle USD Lights.
  All lights are scanned and put into a lightList at /root/world in Katana
  to ensure render find them. Since UsdLuxLight is not a concrete schema, we made an ArnoldLight
  that inherits from UsdLuxLight and allow us to have generic ArnoldLight into USD scene.
- Maya: Variants support in the Walter outliner.
- Arnold: primvar support for Usd PointInstancer

### Fixed
- Arnold: Texture coords stretching issue resolved.
- Katana: crash when scene graph is evaluated and layers are changed.
- Build system: The 'debug' flag is now propagated to every dependencies

### Removed
- Arnold: Version 4 is not supported anymore.
- MtoA Extension: jsoncpp dependency not needed anymore.

## [1.1.0] - 2018-08-30

### Added
- Katana: USD PointInstancer support using an procedural location.

### Removed
- MtoA Extension: jsoncpp dependency not needed anymore.

## [1.0.1] - 2018-08-09

### Changed
- New Walter icons in Maya and Houdini.

## [1.0.0] - 2018-08-08

Initial release
