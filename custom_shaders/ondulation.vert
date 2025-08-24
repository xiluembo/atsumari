VARYING vec3 pos;
void MAIN()
{
    pos = VERTEX;
    float wave = sin(pos.x * 4.0 + uTime / 10) * 2.1;
    vec3 displaced = pos + vec3(0.0, wave, 0.0);
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(displaced, 1.03);
}
