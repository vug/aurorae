#version 460
#extension GL_EXT_scalar_block_layout: require
#extension GL_EXT_shader_explicit_arithmetic_types_int64: require

struct VertexOutput {
    vec3 objectPosition;
    vec3 worldPosition;
    vec3 objectNormal;
    vec3 worldNormal;
    vec4 color;
    vec2 texCoord0;
};

layout (location = 0) in VertexOutput v;

layout (set = 0, binding = 0, scalar) uniform perFrameData {
    mat4 viewFromModel;
    mat4 projectionFromView;
    vec3 camPos;
    uint64_t frameNo;
} perFrame;

layout (set = 1, binding = 0, scalar) uniform MaterialParams {
    vec4 ambientLight;
    //
    vec4 diffuseColor;
    vec4 specularColor;
    float shininess;
    vec4 lightColor;
    float radiantFlux;
} matParams;

#define PI 3.1415926535897932384626433832795

layout (location = 0) out vec4 outColor;

vec3 lin2rgb(vec3 lin) { // linear to sRGB approximation
                         return pow(lin, vec3(1.0 / 2.2));
}

vec3 rgb2lin(vec3 rgb) { // sRGB to linear approximation
                         return pow(rgb, vec3(2.2));
}

vec3 phongBRDF(vec3 lightDir, vec3 viewDir, vec3 normal,
               vec3 diffuseCol, vec3 specularCol, float shininess) {
    vec3 color = diffuseCol;
    vec3 reflectDir = reflect(-lightDir, normal);
    float specDot = max(dot(reflectDir, viewDir), 0.0);
    color += pow(specDot, shininess) * specularCol;
    return color;
}

vec3 blinnPhongBRDF(vec3 lightDir, vec3 viewDir, vec3 normal,
                    vec3 diffuseCol, vec3 specularCol, float shininess) {
    vec3 color = diffuseCol;
    vec3 halfDir = normalize(viewDir + lightDir);
    float specDot = max(dot(halfDir, normal), 0.0);
    color += pow(specDot, shininess) * specularCol;
    return color;
}

void main() {
    const vec3 lightPos = vec3(5);
    vec3 lightDir = lightPos - v.worldPosition;
    const float r = length(lightDir);
    lightDir = normalize(lightDir);
    const vec3 viewDir = normalize(perFrame.camPos - v.worldPosition);
    const vec3 n = normalize(v.worldNormal);

    vec3 radiance = rgb2lin(matParams.diffuseColor.rgb) * rgb2lin(matParams.ambientLight.rgb);
    const float irradiance = max(dot(lightDir, n), 0.0) * matParams.radiantFlux / (4.0 * PI * r * r);
    const vec3 brdf = blinnPhongBRDF(lightDir, viewDir, n, rgb2lin(matParams.diffuseColor.rgb), rgb2lin(matParams.specularColor.rgb), matParams.shininess);

    radiance += brdf * irradiance * rgb2lin(matParams.lightColor.rgb);

    outColor.rgb = lin2rgb(radiance);

    outColor.rgb = matParams.lightColor.rgb * max(dot(lightDir, n), 0.0);
    outColor.a = 1.0;
}
