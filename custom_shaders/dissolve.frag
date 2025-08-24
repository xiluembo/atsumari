float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void MAIN()
{
    float threshold = fract(uTime/3600);
    float n = hash(gl_FragCoord.xy * 0.1);
    if (n < threshold) {
        BASE_COLOR = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        BASE_COLOR = vec4(vec3(baseColor), 1.0);
    }
    METALNESS = 0.6;
    SPECULAR_AMOUNT = 0.4;
    ROUGHNESS = 0.4;
}
