#version 330

in vec2 TexCoord0;
in vec3 Normal0;                                                                    
                                                                                    
out vec4 FragColor;                                                                 
                                                                                    
struct DirectionalLight                                                             
{                                                                                   
    vec3 Color;                                                                     
    float AmbientIntensity;                                                         
    float DiffuseIntensity;                                                         
    vec3 Direction;                                                                 
};                                                                                  

struct Material
{
    vec3 AmbientColor;
};

uniform DirectionalLight gDirectionalLight;                                         
uniform Material gMaterial;
uniform sampler2D gSampler;

void main()
{
    vec4 AmbientColor = vec4(gDirectionalLight.Color, 1.0f) *                       
                        gDirectionalLight.AmbientIntensity;                         

    float DiffuseFactor = dot(normalize(Normal0), -gDirectionalLight.Direction);    
                                                                                    
    vec4 DiffuseColor = vec4(0, 0, 0, 0);;                                                              
                                                                                    
    if (DiffuseFactor > 0) {                                                        
        DiffuseColor = vec4(gDirectionalLight.Color, 1.0f) *                        
                       gDirectionalLight.DiffuseIntensity *                         
                       DiffuseFactor;                                               
    }                                                                               

    FragColor = texture2D(gSampler, TexCoord0.xy) *
                vec4(gMaterial.AmbientColor, 1.0f) *
                (AmbientColor + DiffuseColor);                                      
}
