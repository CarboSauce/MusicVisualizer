#version 430 core
in vec2 fragTexCoord;
out vec4 finalColor;
layout(location = 1) uniform float time;
void main(){
    vec3 smoothed = smoothstep(vec3(.4,.8, .6), vec3(1.,1.,.2-.2*cos(time)),vec3(fragTexCoord.y));
    vec3 color = smoothed;
    finalColor = vec4(color,1.);
}