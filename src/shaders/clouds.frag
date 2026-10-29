#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vCoordinate;
uniform sampler2D uNoise0;
uniform sampler2D uNoise1;
uniform sampler2D uNoise2;
uniform float uTime;
uniform float uAspect;
uniform float uWindTime;
uniform vec2 uMaskPeriod;
uniform sampler2D uCloudMask;

float density(vec2 p)
{
    vec2 wind = vec2(0.010, 0.002) * uWindTime;
    // Internal currents are independent of the circle mask's wind, so noise
    // evolves within a cloud rather than being painted onto a moving shape.
    vec2 evolution = vec2(0.003, -0.002) * uTime;
    float a = texture2D(uNoise0, p + wind + evolution).r;
    float b = texture2D(uNoise1, p * 2.0 + wind * 0.78 - evolution * 1.35 +
        vec2(0.17, 0.31 + sin(uTime * 0.017) * 0.025)).r;
    float c = texture2D(uNoise2, p * 4.0 + wind * 1.21 + evolution * 2.1 +
        vec2(cos(uTime * 0.023) * 0.035, 0.43)).r;
    return a * 0.55 + b * 0.30 + c * 0.15;
}

float fineDetail(vec2 p)
{
    vec2 advected = p + vec2(0.010, 0.002) * uWindTime;
    vec2 flowA = vec2(0.004, -0.003) * uTime;
    vec2 flowB = vec2(-0.0025, 0.0015) * uTime;
    float a = texture2D(uNoise2, (advected + flowA) * 8.0 + vec2(0.21, 0.73)).r;
    float b = texture2D(uNoise1, (advected + flowB) * 16.0 + vec2(0.61, 0.19) +
        vec2(sin(uTime * 0.03) * 0.008, 0.0)).r;
    return a * 0.65 + b * 0.35;
}

float cloudField(vec2 point)
{
    // Baked irregular groups: one lookup regardless of the number of lobes.
    return texture2D(uCloudMask, point / uMaskPeriod).r;
}

void main()
{
    vec2 screen = vec2(vCoordinate.x * uAspect, vCoordinate.y);
    vec3 sky = mix(vec3(0.63, 0.78, 0.86), vec3(0.20, 0.44, 0.69),
                   pow(vCoordinate.y, 0.75));
    vec2 sun = vec2(uAspect * 0.23, 0.81);
    float distance = length(screen - sun);
    sky += vec3(0.19, 0.15, 0.08) * exp(-distance * distance * 12.0);
    sky += vec3(0.38, 0.32, 0.18) * (1.0 - smoothstep(0.023, 0.028, distance));
    vec2 maskPoint = screen + vec2(0.010, 0.002) * uWindTime / 0.45 + vec2(0.08, 0.12);
    float field = cloudField(maskPoint);
    // Only skip exact endpoints. The blends below converge to these colors,
    // so there is no threshold ring where noise switches on or off.
    if (field <= 0.0) { gl_FragColor = vec4(sky, 1.0); return; }
    if (field >= 1.0) { gl_FragColor = vec4(1.0, 0.995, 0.985, 1.0); return; }
    vec2 p = screen * 0.45;
    float d = density(p);
    float detail = fineDetail(p) - 0.5;
    // One continuous bell envelope: no narrow noise cutoff against a circle.
    // Coverage and its slope approach both endpoints smoothly, preserving
    // clear sky and dense white cores without a hard inner contour.
    float edgeBand = 4.0 * field * (1.0 - field);
    float coverage = smoothstep(0.0, 1.0,
        field + ((d - 0.5) * 0.70 + detail * 0.55) * edgeBand);
    float core = smoothstep(0.25, 0.92, field);
    // A few density probes approximate sun-facing rims and soft self-shadow.
    // No ray marching: at most eleven filtered lookups, only around cloud edges.
    vec2 towardSun = vec2(-0.018, 0.026);
    float nearby = density(p + towardSun);
    float shadow = density(p + towardSun * 3.5);
    float light = clamp(0.90 + (d - nearby) * 2.0 - shadow * 0.42 +
                        detail * 0.22 * (1.0 - core), 0.42, 1.0);
    vec3 edgeCloud = mix(vec3(0.48, 0.57, 0.67), vec3(1.0, 0.99, 0.96), light);
    vec3 cloud = mix(edgeCloud, vec3(1.0, 0.995, 0.985), core);
    gl_FragColor = vec4(mix(sky, cloud, coverage), 1.0);
}
