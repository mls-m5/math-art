#ifdef GL_ES
precision mediump float;
#endif

varying vec2 vLocal;
varying vec2 vCapsule;
varying float vBrightness;

void main()
{
    // Exact signed distance to a capsule; a zero-length segment is a disk.
    vec2 q = vec2(max(abs(vLocal.x) - vCapsule.x, 0.0), vLocal.y);
    float distance = length(q) - vCapsule.y;
    // Coordinates are pixels, so no derivatives/extensions are needed for AA.
    float coverage = 1.0 - smoothstep(-0.65, 0.65, distance);
    vec3 light = vec3(0.83, 0.91, 1.0) * vBrightness * coverage;
    gl_FragColor = vec4(light, coverage);
}
