#version 400 core

uniform vec3 eyepos;
uniform samplerCube cubemap;
uniform vec2 resolution;
uniform mat4 ViewMatrix;

uniform float dist_factor = 3.5;
uniform int max_iter;

uniform int accretion_disk;

out vec4 Fragcolor;

vec2 coord;

//https://gist.github.com/patriciogonzalezvivo/670c22f3966e662d2f83
vec4 permute(vec4 x) { return mod(((x*34.0)+1.0)*x, 289.0); }

vec2 fade(vec2 t) {return t*t*t*(t*(t*6.0-15.0)+10.0);}

// Perlin noise function to give accretion disk some texture
float cnoise(vec2 P){
    vec4 Pi = floor(P.xyxy) + vec4(0.0, 0.0, 1.0, 1.0);
    vec4 Pf = fract(P.xyxy) - vec4(0.0, 0.0, 1.0, 1.0);
    Pi = mod(Pi, 289.0); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;
    vec4 i = permute(permute(ix) + iy);
    vec4 gx = 2.0 * fract(i * 0.0243902439) - 1.0; // 1/41 = 0.024...
    vec4 gy = abs(gx) - 0.5;
    vec4 tx = floor(gx + 0.5);
    gx = gx - tx;
    vec2 g00 = vec2(gx.x,gy.x);
    vec2 g10 = vec2(gx.y,gy.y);
    vec2 g01 = vec2(gx.z,gy.z);
    vec2 g11 = vec2(gx.w,gy.w);
    vec4 norm = 1.79284291400159 - 0.85373472095314 * 
        vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;
    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));
    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}

// Calculates an the acceleration due to gravity at the given position
vec3 march(vec3 pos) {
    // Higher acceleration closer to the center
    float r = pow(length(pos), dist_factor);
    vec3 acceleration = -1 * pos / r;
    return acceleration;
}

vec4 trace(vec3 pos, vec3 velocity) {
    vec4 final_color = vec4(0,0,0,1);
    vec3 old_pos;
    bool passed_plane = false;
    float dist;

    for (int i = 0; i < max_iter; i++) {
        // Calculate new position this step
        vec3 acceleration = march(pos);
        velocity += acceleration;
        old_pos = pos;
        pos += velocity;

        dist = length(pos);

        // Inside Event Horizon
        if (dist < 1) {
            if (!passed_plane) {
                return vec4(0,0,0,0);
            }
            else {
                final_color*=10;
            }
        }

        if (accretion_disk != 0) {
            // Ray passed through accretion disk
            if (dist < 15 && old_pos.y*pos.y < 0) {
                final_color += vec4(0.1, 0.07, 0.05, 0)*50/pow(dist, 1)*(cnoise(100*pos.xz)+1);
                if (dist > 1 && !passed_plane) {
                    passed_plane = true;
                }
                // Draw over Event Horizon
                if (length(coord) < 0.2) {
                    return vec4(1);
                }
            }
        }
    }

    // Sample cubemap 
    // return texture(cubemap, velocity);
    if (accretion_disk != 0)
        return mix(final_color, texture(cubemap, velocity), dist/500);
    else 
        return texture(cubemap, velocity);
}

void main() {
    // Turn pixel coordinates into world coordinates
    // https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-generating-camera-rays/generating-camera-rays
    coord = (gl_FragCoord.xy / resolution.xy - vec2(0.5)) * 2;
    // Adjust for aspect ratio
    coord.x *= resolution.x / resolution.y;
    // Generate ray from camera
    vec3 velocity = vec3(coord.x, coord.y, 1)*0.3;
    velocity *= mat3(ViewMatrix);

    // Trace ray
    Fragcolor = trace(eyepos, velocity);
}