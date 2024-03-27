## Add parameter xml in htg depth limiter

Added the JustMarkTheDepthLimiter parameter to the Properties of the vtkHyperTreeGridDepthLimiter filter in the ParaView GUI.

Activation or not results in a call to the SetJustCreateNewMask method.
If enabled, applying this option will cause a setting to be placed that will limit the depth travel of the sliders. This avoids consuming additional memory. Not activating this option allows you to produce a new smaller mesh, for example when saving.
SaveData does not take into account the value of the depth render controller. If you want to store a reduced mesh in depth, you must deactivate this new option.

Improved documentation.
