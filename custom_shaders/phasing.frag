// use a "_phasing" vertex shader to obtain phase data
VARYING vec2 vPhaseData;

float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

void MAIN()
{
    // 240s completge cycle => ~60s per transition
    float t = fract((vPhaseData.y / 2 + uTime) / 240.0);

    float seg  = t * 4.0;          // 4 segments
    float fraq = fract(seg);       // position inside segment [0,1)
    int   idx  = int(floor(seg));  // which segment (0..3)

    vec4 color_a = vec4(0.867, 0.22, 0.106, 1.0);
    vec4 color_b = vec4(0.376, 0.051, 0.89, 1.0);
    vec4 color_c = vec4(0.965, 0.871, 0.0, 1.0);
    vec4 color_d = vec4(0.149, 0.796, 0.569, 1.0);

    vec4 c0, c1;
    if      (idx == 0) { c0 = color_a; c1 = color_b; }
    else if (idx == 1) { c0 = color_b; c1 = color_c; }
    else if (idx == 2) { c0 = color_c; c1 = color_d; }
    else               { c0 = color_d; c1 = color_a; }

    // monotonic easing with zero derivative at the borders
    // w = 0 -> c0 | w = 1 -> c1
    float w = 0.5 - 0.5 * cos(3.14159265 * fraq);
    // alternative: float w = smoothstep(0.0, 1.0, fraq);

    vec4 mixColor = mix(c0, c1, w);

    BASE_COLOR = mixColor;
    METALNESS = 0.6;
    SPECULAR_AMOUNT = 0.4;
    ROUGHNESS = 0.4;
}
