#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vCoordinate; // Each cell has its own x,y in [0, 1].
varying float vVariant;   // Checkerboard parity, identical at every cell vertex.
uniform float uTime;     // Available for future animated shader effects.

void main()
{
    // Distance to the nearest cell edge forms nested rectangular contours.
    vec2 edge = min(vCoordinate, vec2(1.0) - vCoordinate);
    float distanceToEdge = min(edge.x, edge.y);
    // Black outer border, white inner frame, black center.
    float color = step(0.12, distanceToEdge) * (1.0 - step(0.28, distanceToEdge));
    color = mix(color, 1.0 - color, step(0.5, vVariant));
    gl_FragColor = vec4(vec3(color), 1.0);
}
