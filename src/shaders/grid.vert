attribute vec2 aPosition;
attribute vec2 aCoordinate;
attribute float aVariant;
uniform vec2 uView;
varying vec2 vCoordinate;
varying float vVariant;

void main()
{
    vCoordinate = aCoordinate;
    vVariant = aVariant;
    gl_Position = vec4(aPosition / uView, 0.0, 1.0);
}
