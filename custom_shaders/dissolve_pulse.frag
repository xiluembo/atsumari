float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void MAIN()
{
    float pulse = 0.5 + 0.5 * sin(3.14159265358979 * uTime / 60.0);
    vec4 otherColor = vec4(0.75, 0.65, 0.22, 1.0);

    float threshold = fract(uTime/3600);
    float tri = 1.0 - abs(2.0 * threshold - 1.0);
    float n = hash(gl_FragCoord.xy * 0.1);
    if (n < tri) {
        BASE_COLOR = vec4(vec3(0.25, 0.25, 0.25) * (0.3 + pulse/2), 1.0);
    } else {
        BASE_COLOR = vec4(vec3(baseColor * (0.3 +1-pulse)), 1.0);
    }
    METALNESS = 0.7;
    SPECULAR_AMOUNT = 0.4;
    ROUGHNESS = 0.4;
}
