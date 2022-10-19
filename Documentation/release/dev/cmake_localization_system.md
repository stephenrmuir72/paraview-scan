## CMake localization system

This is a system that can generate, translate
and use its own translations into ParaView.
It adds two cached variables:

* `PARAVIEW_BUILD_TRANSLATIONS` that generates translation
TS file to be filled with translation, and then compile them.

* `PARAVIEW_TRANSLATIONS_DIRECTORY` that indicate the desired path
for the ts files.
