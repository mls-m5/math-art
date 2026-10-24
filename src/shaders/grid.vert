attribute vec2 aPosition;
attribute vec2 aCoordinate;
uniform vec2 uView;
varying vec2 vCoordinate;

void main()
{
    vCoordinate = aCoordinate;
    gl_Position = vec4(aPosition / uView, 0.0, 1.0);
}
