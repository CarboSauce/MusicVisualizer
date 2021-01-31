#version 430 core
in vec2 fragTexCoord;
layout(location = 1) uniform float time;
uniform sampler2D texture0;
out vec4 finalColor;
void main(){
    float smoothCircle = smoothstep(0.5,0.49,length(fragTexCoord - vec2(.5)));
    //finalColor = vec4( .5-cos(time)*(.5-texture2D(texture0,fragTexCoord).rgb),smoothCircle);
    //float gx = 2.f*abs(mod(time*.125,1.f)-.5);
    finalColor = /*vec4(gx+(1.-2.*gx)*/vec4(texture2D(texture0,fragTexCoord).rgb,smoothCircle);
}