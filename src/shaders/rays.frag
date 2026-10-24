#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vPosition;
uniform float uTime;       // Seconds; positive time moves bands inward.
uniform float uAngleStart; // Start ray of this wedge, radians in [0, 2*pi).
uniform float uAngleSpan;  // Positive angle to the next ray (including wrap).

const float TAU = 6.28318530718;

vec2 fieldCoordinates(vec2 position)
{
    float angle = atan(position.y, position.x);
    // x is normalized angle: 0 at the first ray, 1 at the second.
    float x = clamp(mod(angle - uAngleStart + TAU, TAU) / uAngleSpan, 0.0, 1.0);
    // y is true radial distance in world units, not interpolated triangle depth.
    float y = length(position);
    return vec2(x, y);
}

void main()
{
    vec2 field = fieldCoordinates(vPosition);
    // A constant phase moves toward smaller radii as time increases.
    // Each black or white field is 0.10 world units wide, moving at 0.18 units/s.
    float band = mod(floor((field.y + uTime * 0.18) / 0.10), 2.0);
    // field.x is available here for future angular patterns.
    gl_FragColor = vec4(vec3(band), 1.0);
}
