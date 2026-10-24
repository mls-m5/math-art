#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vPosition;
uniform float uTime;       // Seconds; positive time moves bands inward.
uniform float uBandOffset; // 0 or 1, alternating in angular wedge order.
uniform float uBendAmount; // Radial half-sine amplitude in world units.
uniform float uInnerRadius; // Cutoff measured in the same warped radial space.
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
    // One half-sine per wedge: zero at both rays, maximum at x = 0.5.
    // Alternating signs bow neighboring wedges in opposite radial directions.
    float direction = 1.0 - 2.0 * uBandOffset;
    float radialDistance = field.y + direction * uBendAmount * sin(0.5 * TAU * field.x);
    // Cut out the center using the same warped distance as the bands.
    // This boundary stays still while bands flow into it.
    if (radialDistance < uInnerRadius) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }
    // A constant phase moves toward smaller radii as time increases.
    // Each black or white field is 0.10 world units wide, moving at 0.18 units/s.
    float band = mod(floor((radialDistance + uTime * 0.18) / 0.10) + uBandOffset, 2.0);
    gl_FragColor = vec4(vec3(band), 1.0);
}
