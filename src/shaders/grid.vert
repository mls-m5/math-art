attribute vec2 aPosition;
attribute vec2 aCoordinate;
attribute float aVariant;
uniform vec2 uView;
varying vec2 vCoordinate;
varying float vVariant;
varying vec2 vScreen;

void main()
{
    vCoordinate = aCoordinate;
    vVariant = aVariant;
    vScreen = 0.5 * (aPosition / uView + vec2(1.0));
    gl_Position = vec4(aPosition / uView, 0.0, 1.0);
}
