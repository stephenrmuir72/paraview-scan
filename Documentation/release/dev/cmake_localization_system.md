## CMake localization system

This is a system that let users generate, translate
and use their own translations into ParaView.
and titled to match the topic name to avoid merge conflicts.
It adds three cached variables:
* `PARAVIEW_ENABLE_LOCALIZATION` that make ParaView Client
load a translation file at start.

* `PARAVIEW_GENERATE_TRANSLATIONS` that generates translation
TS file to be filled with translation, and then compile them.

* `translations_directory` that indicate the desired path
for the ts files.
