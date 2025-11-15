#version 330 core

// output: colors in RGBA
out vec4 out_fragColor;

// input: texture coordinates from the Vertex Shader
in vec2 vert_texCoord;

// input: texture unit from main program
uniform sampler2D in_texture;

// input: canvas dimensions
uniform vec2 in_resolution;

// input: center
uniform vec2 in_center;

// input: radius
uniform float in_radius;

// Sign function
float msign(in float x) { return (x<0.0)?-1.0:1.0; }

// Ellipse signed distance function
float sdEllipse( vec2 p, in vec2 ab)
{
    if( ab.x==ab.y ) return length(p)-ab.x;

    p = abs( p ); 
    if( p.x>p.y ){ p=p.yx; ab=ab.yx; }
    
    float l = ab.y*ab.y - ab.x*ab.x;
    
    float m = ab.x*p.x/l; 
    float n = ab.y*p.y/l; 
    float m2 = m*m;
    float n2 = n*n;
    
    float c = (m2+n2-1.0)/3.0; 
    float c3 = c*c*c;

    float d = c3 + m2*n2;
    float q = d  + m2*n2;
    float g = m  + m *n2;

    float co;

    if( d<0.0 )
    {
        float h = acos(q/c3)/3.0;
        float s = cos(h) + 2.0;
        float t = sin(h) * sqrt(3.0);
        float rx = sqrt( m2-c*(s+t) );
        float ry = sqrt( m2-c*(s-t) );
        co = ry + sign(l)*rx + abs(g)/(rx*ry);
    }
    else
    {
        float h = 2.0*m*n*sqrt(d);
        float s = msign(q+h)*pow( abs(q+h), 1.0/3.0 );
        float t = msign(q-h)*pow( abs(q-h), 1.0/3.0 );
        float rx = -(s+t) - c*4.0 + 2.0*m2;
        float ry =  (s-t)*sqrt(3.0);
        float rm = sqrt( rx*rx + ry*ry );
        co = ry/sqrt(rm-rx) + 2.0*g/rm;
    }
    co = (co-m)/2.0;

    float si = sqrt( max(1.0-co*co,0.0) );

    vec2 r = ab * vec2(co,si);
    
    return length(r-p) * msign(p.y-r.y);
}

const vec3 color_black = vec3(0.0,0.0,0.0);
const vec3 color_white = vec3(1.0,1.0,1.0);

void main() {
    vec2 center = in_center/in_resolution;
    vec2 radii = in_radius/in_resolution;
    vec2 p = vert_texCoord-center;
    float dist_disk = sdEllipse(p, radii);
    vec3 mask = (dist_disk>0.0) ? color_black : color_white;
    vec4 image_mono = texture(in_texture, vert_texCoord);
    out_fragColor = vec4(vec3(image_mono)*mask, 1.0);
}