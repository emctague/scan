R"HERE(
#version 330 core

in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

const vec3 lightPos = vec3(0.5, 6.0, 0.5);
const vec3 objectColor = vec3(0.8, 0.5, 0.5);
const float falloff = 1.0;

void
main ()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0) * 1.0 / (falloff * pow(length(lightPos - FragPos), 2.0));
    vec3 diffuse = diff * vec3(1.0, 0.9, 0.9);
    vec3 ambient = 0.1 * vec3(0.9, 0.9, 1.0);
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);
}
)HERE";
