#ifdef GL_ES
precision mediump float;
#endif

varying float vHeight;
varying float vLight;

void main()
{
    float height = smoothstep(2.0, 10.0, vHeight);
    vec3 green = mix(vec3(0.09, 0.32, 0.13), vec3(0.36, 0.67, 0.27), height);
    gl_FragColor = vec4(green * vLight, 1.0);
}
