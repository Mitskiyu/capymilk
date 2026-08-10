#version 450

in vec3 v_color;
flat in float v_type;

out vec4 frag_color;

void main()
{
    vec2 circ = 2.0 * gl_PointCoord - 1.0;

    float alpha;
    if (v_type == 0.0)
        alpha = 1.0 - length(circ);
    else
        alpha = 0.05 * (1.0 - length(circ));

    frag_color = vec4(v_color, alpha);
}
