#version 430 core
in vec2 fragTexCoord;
layout (location = 1) uniform float bass_shift;
uniform sampler2D texture0;
out vec4 finalColor;
void main(){
    
    vec2 mix_shift = .05*vec2(smoothstep(10,100 ,bass_shift ));
    vec2 cord = fragTexCoord-vec2(2.)*mix_shift*fragTexCoord+mix_shift;
    finalColor = vec4(texture2D(texture0,cord).rgb,1.);
}