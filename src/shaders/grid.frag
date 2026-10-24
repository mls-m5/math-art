#ifdef GL_ES
#extension GL_OES_standard_derivatives : enable
precision mediump float;
#endif

varying vec2 vCoordinate; // Each cell has its own x,y in [0, 1].
varying float vVariant;   // Checkerboard parity, identical at every cell vertex.
varying vec2 vScreen;     // Whole-screen x/y in [0, 1], independent of the cells.
uniform float uTime;
uniform float uWavesEnabled;
// First three curves are vertical, last three horizontal.
uniform float uWaveAmplitude[6];
uniform float uWaveFrequency[6];
uniform float uWavePhase[6];

float pixelHalfWidth(float distance)
{
#if !defined(GL_ES) || defined(GL_OES_standard_derivatives)
    return max(0.5 * fwidth(distance), 0.00001);
#else
    return 0.002;
#endif
}

float crossing(float distance)
{
    float width = pixelHalfWidth(distance);
    return smoothstep(-width, width, distance);
}

float wave(float coordinate, float amplitude, float frequency, float phase)
{
    float angle = 6.28318530718 * frequency * coordinate;
    return amplitude * (
        sin(angle + phase + 0.17 * uTime) +
        0.45 * sin(2.0 * angle + 1.7 * phase - 0.13 * uTime) +
        0.20 * sin(3.0 * angle + 2.3 * phase + 0.11 * uTime));
}

float waveInversion()
{
    float paritySign = 1.0;
    for (int i = 0; i < 3; ++i) {
        float baseline = 0.25 * float(i + 1);
        float vertical = baseline + wave(vScreen.y,
            uWaveAmplitude[i], uWaveFrequency[i], uWavePhase[i]);
        float horizontal = baseline + wave(vScreen.x,
            uWaveAmplitude[i + 3], uWaveFrequency[i + 3], uWavePhase[i + 3]);
        // To reach the left edge, cross vertical curves to our left.
        paritySign *= 1.0 - 2.0 * crossing(vScreen.x - vertical);
        // To reach the top edge, cross horizontal curves above us.
        paritySign *= 1.0 - 2.0 * crossing(horizontal - vScreen.y);
    }
    // Multiplying signs is XOR parity; smooth coverage antialiases boundaries.
    return 0.5 * (1.0 - paritySign);
}

void main()
{
    // Distance to the nearest cell edge forms nested rectangular contours.
    vec2 edge = min(vCoordinate, vec2(1.0) - vCoordinate);
    float distanceToEdge = min(edge.x, edge.y);
    float halfWidth = pixelHalfWidth(distanceToEdge);
    // Black outer border, white inner frame, black center.
    float outer = smoothstep(0.12 - halfWidth, 0.12 + halfWidth, distanceToEdge);
    float inner = smoothstep(0.28 - halfWidth, 0.28 + halfWidth, distanceToEdge);
    float color = outer * (1.0 - inner);
    color = mix(color, 1.0 - color, step(0.5, vVariant));
    if (uWavesEnabled > 0.5)
        color = mix(color, 1.0 - color, waveInversion());
    gl_FragColor = vec4(vec3(color), 1.0);
}
