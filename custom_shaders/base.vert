VARYING vec3 pos;
void MAIN()
{
    pos = VERTEX;
    POSITION = MODELVIEWPROJECTION_MATRIX * vec4(pos, 1.0);
}
