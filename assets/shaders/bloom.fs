#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 resolution;
uniform float threshold;
uniform float knee;
uniform float intensity;

// Output fragment color
out vec4 finalColor;

const float samples = 5.0;
const float quality = 1.0;

float Luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    vec2 sizeFactor = vec2(1.0)/resolution*quality;

    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    const int range = 2;            // should be = (samples - 1)/2;
    float kneeValue = max(knee, 0.0001);
    vec3 bloomSum = vec3(0.0);

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            vec4 sampleColor =
                texture(texture0, fragTexCoord + vec2(x, y)*sizeFactor);
            float luma = Luminance(sampleColor.rgb);
            float weight = smoothstep(threshold - kneeValue,
                                      threshold + kneeValue, luma);
            bloomSum += sampleColor.rgb * weight * sampleColor.a;
        }
    }

    vec3 bloom = bloomSum / (samples * samples);
    vec3 color = bloom * intensity;
    finalColor = vec4(color, 1.0) * colDiffuse * fragColor;
}
