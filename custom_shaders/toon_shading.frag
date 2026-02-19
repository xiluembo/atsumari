float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float toonBands(float x, float bands)
{
    x = clamp(x, 0.0, 1.0);

    // Small screen-space dither to reduce banding/aliasing between steps.
    float d = (hash(FRAGCOORD.xy) - 0.5) * (1.0 / bands) * 0.65;
    x = clamp(x + d, 0.0, 1.0);

    // Quantize into discrete bands.
    return floor(x * bands) / max(1.0, (bands - 1.0));
}

void MAIN()
{
    // ---------- Your original 240s cycle (4 segments) ----------
    float t = fract(uTime / 240.0);

    float seg  = t * 4.0;          // 4 segments
    float fraq = fract(seg);       // position inside segment [0,1)
    int   idx  = int(floor(seg));  // segment index (0..3)

    vec4 color_a = vec4(0.973, 0.541, 0.224, 1.0);
    vec4 color_b = vec4(0.824, 0.031, 0.012, 1.0);
    vec4 color_c = vec4(0.282, 0.608, 0.863, 1.0);
    vec4 color_d = vec4(0.114, 0.722, 0.502, 1.0);

    vec4 c0, c1;
    if      (idx == 0) { c0 = color_a; c1 = color_b; }
    else if (idx == 1) { c0 = color_b; c1 = color_c; }
    else if (idx == 2) { c0 = color_c; c1 = color_d; }
    else               { c0 = color_d; c1 = color_a; }

    // Smooth monotonic easing (zero derivative at endpoints)
    float w = 0.5 - 0.5 * cos(3.14159265 * fraq);
    vec4 base = mix(c0, c1, w);

    // ---------- MAIN-only toon lighting (no scene light uniforms) ----------
    // NORMAL: world-space normal (already adjusted for double-sided)
    // VIEW_VECTOR: vector from fragment toward camera
    vec3 n = normalize(NORMAL);
    vec3 v = normalize(VIEW_VECTOR);

    // "Headlight" style light direction (from camera)
    // This avoids needing a custom uLightDir uniform.
    vec3 l = v;

    float ndl = max(dot(n, l), 0.0);

    // Toon quantization (3.0 = more cartoony, 5.0 = smoother)
    float bands = 4.0;
    float b = toonBands(ndl, bands);

    // Keep colors alive (avoid full black in shadow)
    float ambient = 0.20;
    float lit = mix(ambient, 1.0, b);

    vec3 col = base.rgb * lit;

    // Hard anime-like specular highlight
    vec3 h = normalize(v + l);
    float ndh = max(dot(n, h), 0.0);
    float spec = step(0.965, ndh);     // lower threshold => more highlight
    col += spec * vec3(1.0) * 0.18;    // highlight intensity

    // Rim + fake ink outline via Fresnel
    float ndv = max(dot(n, v), 0.0);
    float edge = 1.0 - ndv;

    float ink = smoothstep(0.35, 0.85, edge);
    col = mix(col, col * 0.18, ink * 0.9); // darken edges

    float rim = smoothstep(0.65, 0.98, edge);
    col += base.rgb * rim * 0.06;          // subtle colored rim

    // ---------- Output ----------
    // Drive the look via emissive so it doesn't depend on the scene lighting pipeline.
    BASE_COLOR = vec4(0.0, 0.0, 0.0, base.a);
    EMISSIVE_COLOR = col;

    // Disable PBR contributions (we are "baking" our own shading)
    METALNESS = 0.0;
    SPECULAR_AMOUNT = 0.0;
    ROUGHNESS = 1.0;
}
