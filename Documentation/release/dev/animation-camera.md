Camera Animation: GUI improvements

This adds some options in the KeyFrame editors, leading to:
 - a reduction of the number of camera mode. while keeping previous features.
 - a reduction of clicks to setup camera in interpolation mode

## Camera modes
Camera modes are now as follow:
 - follow path. A keyframe is a spline for camera position. An orbit (i.e. a circle) can be defined from center, normal and starting position.
 - interpolate cameras. A keyframe is a camera. The animation interpolate between cameras, either with spline or line depending on the relevant option.
 - follow data. A "Reset Camera" is done on each frame, with selected data as active object. No keyframe.

Orbit can now be configured at keyframe level (and not at cue level).
Spline mode can be toggled after cue creation.

Removed modes are:
 - orbit. Instead, use path and the option to setup an orbit on current keyframe.
 - interpolate camera locations (spline). Use interpolate cameras. It uses spline by default.
 - interpolate camera locations (linear). Use interpolate cameras, and uncheck "Use spline" option in the key frame editor.
