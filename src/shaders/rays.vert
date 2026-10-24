// GLSL 1.10 / GLSL ES 1.00 compatible: no fixed-function shader inputs.
attribute vec2 aPosition;
uniform vec2 uView;
varying vec2 vPosition;

void main()
{
    vPosition = aPosition;
    gl_Position = vec4(aPosition / uView, 0.0, 1.0);
}
