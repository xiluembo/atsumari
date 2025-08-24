float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }
void MAIN()
{
    vec2 vUV = UV0;
    float stripe = step(0.5, fract(vUV.x * 10.0 + uTime / 120));
    BASE_COLOR = vec4(vec3(baseColor * stripe), 1.0);
}
