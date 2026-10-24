#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vCoordinate; // Each cell has its own x,y in [0, 1].
uniform float uTime;     // Available for future animated shader effects.

void main()
{
    gl_FragColor = vec4(vCoordinate.x, 0.0, vCoordinate.y, 1.0);
}
