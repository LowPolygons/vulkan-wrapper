struct VertexInput {
    float2 position;
    float3 colour;
    float number;
};

struct VertexOutput {
    float3 color;
    float4 sv_position : SV_Position;
};

struct Circle {
    float2 center;
    float radius;
}


struct ImageProperties {
  float win_x;
  float win_y;
};

bool isInCircle(Circle circle, float2 position) {
    float2 distance_from_center = position - circle.center;
    float modulus = 
        distance_from_center[0] * distance_from_center[0] 
        + distance_from_center[1] * distance_from_center[1];

    return modulus <= circle.radius*circle.radius;
}

[shader("vertex")]
VertexOutput vertMain(VertexInput input) {

    VertexOutput output;
    // Remember that this is per vertex, and due to interpolation this will create a smooth gradient in the frag shader? i think; the interpolation happens somewhere
    output.sv_position = float4(input.position, 0.0, 1.0);
    if (input.number > 10) {
        float r = input.colour[2];
        float g = input.colour[0];
        float b = input.colour[1];
        float3 new_colour = {r, g, b};

        output.color = new_colour;
    } else {
        output.color = input.colour;
    }
    return output;
}


[[vk::push_constant]] 
ImageProperties properties;

[shader("fragment")]
float4 fragMain(VertexOutput inVert) : SV_Target
{

    float3 color = inVert.color;

    Circle circle;

    circle.center = {properties.win_x / 2, properties.win_y / 2};
    circle.radius = 300;

    float2 pos = {inVert.sv_position[0], inVert.sv_position[1]};

    if (isInCircle(circle, pos)) {
        return float4(color, 1.0);
    }

    return float4(color[2],color[1],color[0],1.0);
}
