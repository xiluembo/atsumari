VARYING vec3 pos;
void MAIN()
{
    pos = VERTEX;
    float wavex = sin(pos.x * 4.0 + uTime / 10) * 2.1;
    float wavey = sin(pos.y * 4.0 + uTime / 10) * 2.1;
    float wavez = sin(pos.z * 4.0 + uTime / 10) * 2.1;
    vec3 displaced = pos + vec3(wavez, wavex, wavey);
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(displaced, 1.03);
}
