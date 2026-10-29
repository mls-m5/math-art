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

float density(vec2 p)
{
    vec2 wind = vec2(0.010, 0.002) * uWindTime;
    // Independently moving octaves give both drift and gentle shape evolution.
    float a = texture2D(uNoise0, p + wind).r;
    float b = texture2D(uNoise1, p * 2.0 + wind * 0.78 +
        vec2(0.17, 0.31 + sin(uTime * 0.017) * 0.025)).r;
    float c = texture2D(uNoise2, p * 4.0 + wind * 1.21 +
        vec2(cos(uTime * 0.023) * 0.035, 0.43)).r;
    return a * 0.55 + b * 0.30 + c * 0.15;
}

void main()
{
    vec2 screen = vec2(vCoordinate.x * uAspect, vCoordinate.y);
    vec2 p = screen * 0.45;
    float d = density(p);
    float coverage = smoothstep(0.42, 0.63, d);
    // A few density probes approximate sun-facing rims and soft self-shadow.
    // No ray marching: only nine filtered texture lookups per pixel.
    vec2 towardSun = vec2(-0.018, 0.026);
    float nearby = density(p + towardSun);
    float shadow = density(p + towardSun * 3.5);
    float light = clamp(0.90 + (d - nearby) * 2.0 - shadow * 0.42, 0.42, 1.0);
    vec3 cloud = mix(vec3(0.48, 0.57, 0.67), vec3(1.0, 0.99, 0.96), light);
    vec3 sky = mix(vec3(0.63, 0.78, 0.86), vec3(0.20, 0.44, 0.69),
                   pow(vCoordinate.y, 0.75));
    vec2 sun = vec2(uAspect * 0.23, 0.81);
    float distance = length(screen - sun);
    sky += vec3(0.19, 0.15, 0.08) * exp(-distance * distance * 12.0);
    sky += vec3(0.38, 0.32, 0.18) * (1.0 - smoothstep(0.023, 0.028, distance));
    gl_FragColor = vec4(mix(sky, cloud, coverage), 1.0);
}
