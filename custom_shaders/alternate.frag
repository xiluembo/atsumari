float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void MAIN()
{
    float pulse = 0.5 + 0.5 * sin(3.14159265358979 * uTime / 60.0);
    vec4 otherColor = vec4(0.75, 0.65, 0.22, 1.0);
    BASE_COLOR = vec4(vec3(baseColor * (1-pulse)) + vec3(otherColor * pulse), 1.0);
    METALNESS = 0.6;
    SPECULAR_AMOUNT = 0.4;
    ROUGHNESS = 0.4;
}
