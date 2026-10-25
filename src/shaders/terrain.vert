attribute vec2 aPosition;
attribute vec2 aCoordinate;
attribute float aVariant;
attribute vec2 aParameters;
uniform vec2 uView;
varying float vHeight;
varying float vLight;

void main()
{
    // GLM has applied the 3D orthographic transform. Preserve projected Z so
    // the depth buffer, not draw order, decides which column faces are visible.
    gl_Position = vec4(aPosition / uView, aParameters.x, 1.0);
    vHeight = aCoordinate.x;
    vLight = aVariant;
}
