attribute vec2 aPosition;
attribute vec2 aCoordinate;
attribute float aVariant;
attribute vec2 aParameters;
uniform vec2 uView;
varying vec2 vLocal;
varying vec2 vCapsule;
varying float vBrightness;

void main()
{
    // Positions come from GLM's actual 3D view/perspective projection on the CPU.
    // Only the final capsule expansion is screen-space, giving round pixel caps.
    gl_Position = vec4(aPosition / uView, 0.0, 1.0);
    vLocal = aCoordinate;
    vCapsule = aParameters; // Half segment length and radius, in pixels.
    vBrightness = aVariant;
}
