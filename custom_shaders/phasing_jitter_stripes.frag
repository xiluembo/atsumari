// use a "_phasing" vertex shader to obtain phase data
VARYING vec2 vPhaseData;

float hash(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233))) * 43758.5453); }

float stripe_jitter(float coord)
{
    float k = floor(coord);
    return hash(vec2(k, 91.32)) - 0.5;
}

void MAIN()
{
    // 240s complete cycle => ~60s per transition
    float t = fract((vPhaseData.y / 2.0 + uTime) / 240.0);

    float seg  = t * 4.0;
    float fraq = fract(seg);
    int   idx  = int(floor(seg));

    vec4 color_a   = vec4(0.890, 0.878, 0.949, 1.0);
    vec4 color_b   = vec4(0.482, 0.173, 0.659, 1.0);
    vec4 color_c   = vec4(1.0,   0.4,   1.0,   1.0);
    vec4 color_d   = vec4(0.235, 0.0,   0.427, 1.0);

    vec4 c0, c1;
    if      (idx == 0) { c0 = color_a; c1 = color_b; }
    else if (idx == 1) { c0 = color_b; c1 = color_c;  }
    else if (idx == 2) { c0 = color_c; c1 = color_d; }
    else               { c0 = color_d; c1 = color_a; }

    // monotonic easing
    float w = 0.5 - 0.5 * cos(3.14159265 * fraq);
    vec4 mixColor = mix(c0, c1, w);

    vec2 vUV = UV0;

    // ====== Stripes with jitter ======
    const float stripes   = 10.0;
    const float phase     = uTime / 240.0;
    const float gap       = 0.06;
    const float jitterAmp = 0.20;

    // diagonal stripes
    float diagCoord = (vUV.x + vUV.y) * 0.5;
    float baseCoord = diagCoord * stripes + phase;

    float jitter = jitterAmp * stripe_jitter(baseCoord);
    float stripe = step(gap, fract(baseCoord + jitter));

    // ===== color_d in the center =====
    // glowing center
    float d = distance(vUV, vec2(0.5, 0.5));
    // more glow as it gets closer to the center
    float color_dFactor = smoothstep(0.7, 0.0, d);

    // mix "color_c" by color_dFactor
    vec4 color_dColor = mix(mixColor, color_c, 0.35 * color_dFactor);

    // reduce stripe brightness
    float stripeBrightness = mix(0.45, 1.0, stripe);

    BASE_COLOR = color_dColor * stripeBrightness;

    METALNESS        = 0.6;
    SPECULAR_AMOUNT  = 0.8;
    ROUGHNESS        = 0.25;
}
