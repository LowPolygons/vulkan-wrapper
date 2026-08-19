struct VertexInput
{
    float2 position;
    float3 colour;
    float number;
};

struct VertexOutput
{
    float3 color;
    float4 sv_position : SV_Position;
};

struct Circle
{
    float2 center;
    float radius;
}

struct ImageProperties
{
    float win_x;
    float win_y;
    float power;
    float3 ray_origin;
};

bool isInCircle(Circle circle, float2 position)
{
    float2 distance_from_center = position - circle.center;
    float modulus =
        distance_from_center[0] * distance_from_center[0] + distance_from_center[1] * distance_from_center[1];

    return modulus <= circle.radius * circle.radius;
}

float sphere_msd(float3 position, float radius)
{
    return length(position) - radius;
}

float mandelbulb_msd(float3 position, float power) {
    float3 z = position;
    float dr = 1;
    float r;

    for (int i = 0; i < 15; i++) {
        r = length(z);
        if (r > 2)
            break;

        float theta = acos(z.z / r) * power;
        float phi = atan2(z.y, z.x) * power;
        float zr = pow(r, power);
        dr = pow(r, power - 1) * power * dr + 1;

        z = zr * float3(sin(theta) * cos(phi), sin(phi) * sin(theta), cos(theta));

        z += position;
    }
    return 0.5 * log(r) * r / dr;
}

// float mandelbulb_msd(
//     float3 position,
//     int iterations,
//     float power,
//     float bailout)
// {
//     float3 z = position;
//     float derivative = 1.0;
//     float radius = 0.0;
//
//     for (int i = 0; i < iterations; i++)
//     {
//         radius = length(z);
//
//         if (radius > bailout)
//             break;
//
//         if (radius < 1e-6)
//             return 0.0;
//
//         float theta = acos(
//             clamp(z.z / radius, -1.0, 1.0)
//         );
//
//         float phi;
//
//         if (abs(z.x) < 1e-6 && abs(z.y) < 1e-6)
//         {
//             phi = 0.0;
//         }
//         else
//         {
//             phi = atan2(z.y, z.x);
//         }
//
//         derivative =
//             power *
//             pow(radius, power - 1.0) *
//             derivative +
//             1.0;
//
//         float radius_power = pow(radius, power);
//
//         theta *= power;
//         phi *= power;
//
//         z = radius_power * float3(
//             sin(theta) * cos(phi),
//             sin(theta) * sin(phi),
//             cos(theta)
//         );
//
//         z += position;
//     }
//
//     return 0.5 * log(radius) * radius / derivative;
// }

float3 get_norm_ray_direction(float width, float height, float2 input_pos)
{
    float field_of_view = 60.0 * 3.14159265 / 180.0;
    float aspect_ratio = width / height;

    float pp_width = tan(field_of_view / 2.0) * 2.0;
    float pp_height = pp_width / aspect_ratio;

    float pixel_increment_x = pp_width / width;
    float pixel_increment_y = pp_height / height;

    float x = (input_pos.x - width / 2.0) * pixel_increment_x;
    float y = (height / 2.0 - input_pos.y) * pixel_increment_y;

    return normalize(float3(x, y, 1.0));
}

[shader("vertex")]
VertexOutput vertMain(VertexInput input)
{

    VertexOutput output;
    // Remember that this is per vertex, and due to interpolation this will create a smooth gradient in the frag shader? i think; the interpolation happens somewhere
    output.sv_position = float4(input.position, 0.0, 1.0);
    if (input.number > 10)
    {
        float r = input.colour[2];
        float g = input.colour[0];
        float b = input.colour[1];
        float3 new_colour = { r, g, b };

        output.color = new_colour;
    }
    else
    {
        output.color = input.colour;
    }
    return output;
}

float3 estimate_normal(float3 p, float epsilon, float power)
{
    float dx = mandelbulb_msd(p + float3(epsilon, 0, 0), power)
             - mandelbulb_msd(p - float3(epsilon, 0, 0), power);

    float dy = mandelbulb_msd(p + float3(0, epsilon, 0), power)
             - mandelbulb_msd(p - float3(0, epsilon, 0), power);

    float dz = mandelbulb_msd(p + float3(0, 0, epsilon), power)
             - mandelbulb_msd(p - float3(0, 0, epsilon), power);

    return normalize(float3(dx, dy, dz));
}

[[vk::push_constant]]
ImageProperties properties;

//
// float sdSphere(float3 p, float r)
// {
//     return length(p) - r;
// }
//
// float sdBox(float3 p, float3 b)
// {
//     float3 q = abs(p) - b;
//     return length(max(q, 0.0)) +
//            min(max(q.x, max(q.y, q.z)), 0.0);
// }
//
// float sdTorus(float3 p, float2 t)
// {
//     float2 q = float2(length(p.xz) - t.x, p.y);
//     return length(q) - t.y;
// }
//
// float sdCylinder(float3 p, float h, float r)
// {
//     float2 d = abs(float2(length(p.xz), p.y)) -
//                float2(r, h);
//
//     return min(max(d.x, d.y), 0.0) +
//            length(max(d, 0.0));
// }
//
// float smoothUnion(float a, float b, float k)
// {
//     float h = saturate(0.5 + 0.5 * (b - a) / k);
//     return lerp(b, a, h) - k * h * (1.0 - h);
// }
//
// float scene(float3 p, float time)
// {
//     float ca = cos(time * 0.22);
//     float sa = sin(time * 0.22);
//
//     p.xz = float2(
//         p.x * ca - p.z * sa,
//         p.x * sa + p.z * ca
//     );
//
//     float d = 1000.0;
//
//     // Main warped sphere
//     float3 core = p;
//
//     float warp =
//         sin(core.x * 5.0 + time) *
//         sin(core.y * 4.0 - time * 0.7) *
//         sin(core.z * 6.0 + time * 0.4);
//
//     core += normalize(core + 0.0001) * warp * 0.075;
//
//     float coreD = sdSphere(core, 1.15);
//
//     float groove =
//         abs(
//             sin(
//                 atan2(core.z, core.x) * 12.0 +
//                 core.y * 8.0 -
//                 time * 1.5
//             )
//         ) - 0.82;
//
//     coreD += max(groove * 0.13, -0.06);
//
//     d = smoothUnion(d, coreD, 0.12);
//
//     // Orbiting torus rings
//     for (int i = 0; i < 4; i++)
//     {
//         float fi = float(i);
//
//         float phase =
//             time * (0.45 + fi * 0.11) +
//             fi * 1.57;
//
//         float c = cos(phase);
//         float s = sin(phase);
//
//         float3 ringP = p;
//
//         ringP.xy = float2(
//             ringP.x * c - ringP.y * s,
//             ringP.x * s + ringP.y * c
//         );
//
//         ringP.x -= 1.65 + fi * 0.33;
//
//         float ringD = sdTorus(
//             ringP,
//             float2(
//                 0.42 + sin(time * 0.7 + fi) * 0.05,
//                 0.045
//             )
//         );
//
//         d = smoothUnion(d, ringD, 0.045);
//     }
//
//     // Central spine
//     float3 spine = p;
//
//     spine.x += sin(spine.y * 3.0 + time) * 0.12;
//     spine.z += cos(spine.y * 2.4 - time * 0.8) * 0.12;
//
//     d = smoothUnion(
//         d,
//         sdCylinder(spine, 2.7, 0.115),
//         0.08
//     );
//
//     // Floating shards
//     for (int j = 0; j < 7; j++)
//     {
//         float fj = float(j);
//
//         float angle =
//             fj * 0.897 +
//             time * (0.17 + fj * 0.021);
//
//         float radius =
//             2.35 +
//             sin(time * 0.31 + fj * 1.9) * 0.35;
//
//         float height =
//             sin(time * 0.41 + fj * 2.7) * 1.8;
//
//         float3 shard =
//             p -
//             float3(
//                 cos(angle) * radius,
//                 height,
//                 sin(angle) * radius
//             );
//
//         float cs = cos(angle + time * 0.8);
//         float sn = sin(angle + time * 0.8);
//
//         shard.xz = float2(
//             shard.x * cs - shard.z * sn,
//             shard.x * sn + shard.z * cs
//         );
//
//         float scale =
//             0.18 +
//             0.035 * sin(time + fj);
//
//         float shardD =
//             sdBox(
//                 shard,
//                 float3(scale, 0.48, scale)
//             );
//
//         d = smoothUnion(
//             d,
//             shardD,
//             0.025
//         );
//     }
//
//     // Floor
//     float floorD = p.y + 2.45;
//
//     d = min(d, floorD);
//
//     return d;
// }
//
// float3 calcNormal(float3 p, float time)
// {
//     float e = 0.0015;
//
//     float3 n = float3(
//         scene(p + float3(e, 0.0, 0.0), time) -
//         scene(p - float3(e, 0.0, 0.0), time),
//
//         scene(p + float3(0.0, e, 0.0), time) -
//         scene(p - float3(0.0, e, 0.0), time),
//
//         scene(p + float3(0.0, 0.0, e), time) -
//         scene(p - float3(0.0, 0.0, e), time)
//     );
//
//     return normalize(n);
// }
//
// float softShadow(
//     float3 rayOrigin,
//     float3 rayDirection,
//     float time)
// {
//     float result = 1.0;
//     float t = 0.04;
//
//     for (int i = 0; i < 32; i++)
//     {
//         float h =
//             scene(
//                 rayOrigin + rayDirection * t,
//                 time
//             );
//
//         result = min(
//             result,
//             12.0 * h / t
//         );
//
//         t += clamp(h, 0.015, 0.25);
//
//         if (result < 0.01 || t > 8.0)
//             break;
//     }
//
//     return saturate(result);
// }
//
// float hash21(float2 p)
// {
//     p = frac(p * float2(123.34, 456.21));
//     p += dot(p, p + 45.32);
//     return frac(p.x * p.y);
// }
//
// [shader("fragment")]
// float4 fragMain(VertexOutput input) : SV_Target
// {
//     float2 fragCoord =
//         input.sv_position.xy;
//
//     float2 resolution =
//         float2(
//             properties.win_x,
//             properties.win_y
//         );
//
//     float time =
//         properties.power;
//
//     float2 uv =
//         (fragCoord * 2.0 - resolution) /
//         min(resolution.x, resolution.y);
//
//     // Camera
//     float3 ro =
//         float3(0.0, 0.1, -6.0);
//
//     float3 rd =
//         normalize(
//             float3(
//                 uv.x,
//                 uv.y,
//                 1.7
//             )
//         );
//
//     float yaw =
//         sin(time * 0.17) * 0.3;
//
//     float pitch =
//         sin(time * 0.23) * 0.15;
//
//     float cy = cos(yaw);
//     float sy = sin(yaw);
//
//     rd = float3(
//         rd.x * cy + rd.z * sy,
//         rd.y,
//         -rd.x * sy + rd.z * cy
//     );
//
//     float cp = cos(pitch);
//     float sp = sin(pitch);
//
//     rd = float3(
//         rd.x,
//         rd.y * cp - rd.z * sp,
//         rd.y * sp + rd.z * cp
//     );
//
//     // Raymarch
//     float depth = 0.0;
//     float3 hit = ro;
//     bool found = false;
//
//     for (int i = 0; i < 128; i++)
//     {
//         float3 p =
//             ro + rd * depth;
//
//         float d =
//             scene(p, time);
//
//         if (d < 0.001)
//         {
//             hit = p;
//             found = true;
//             break;
//         }
//
//         depth += d * 0.72;
//
//         if (depth > 18.0)
//             break;
//     }
//
//     // Background
//     float3 color =
//         float3(
//             0.002,
//             0.004,
//             0.012
//         );
//
//     float backgroundGlow =
//         exp(-length(uv) * 1.7);
//
//     color +=
//         float3(
//             0.015,
//             0.025,
//             0.07
//         ) *
//         backgroundGlow;
//
//     // Background stars
//     float2 starCell =
//         floor(uv * 55.0);
//
//     float star =
//         hash21(starCell);
//
//     float starPulse =
//         pow(
//             0.5 +
//             0.5 *
//             sin(
//                 time * (2.0 + star * 5.0) +
//                 star * 20.0
//             ),
//             12.0
//         );
//
//     color +=
//         step(0.985, star) *
//         starPulse *
//         0.35;
//
//     if (!found)
//     {
//         return float4(
//             saturate(color),
//             1.0
//         );
//     }
//
//     // Surface normal
//     float3 normal =
//         calcNormal(hit, time);
//
//     // Animated light
//     float3 lightPos =
//         float3(
//             sin(time * 0.51) * 4.0,
//             3.0 + sin(time * 0.37),
//             cos(time * 0.43) * 4.0
//         );
//
//     float3 lightDir =
//         normalize(lightPos - hit);
//
//     float3 viewDir =
//         normalize(ro - hit);
//
//     float diffuse =
//         max(
//             dot(normal, lightDir),
//             0.0
//         );
//
//     float shadow =
//         softShadow(
//             hit + normal * 0.012,
//             lightDir,
//             time
//         );
//
//     float3 halfDir =
//         normalize(
//             lightDir + viewDir
//         );
//
//     float specular =
//         pow(
//             max(
//                 dot(normal, halfDir),
//                 0.0
//             ),
//             72.0
//         );
//
//     float fresnel =
//         pow(
//             1.0 -
//             max(
//                 dot(normal, viewDir),
//                 0.0
//             ),
//             3.5
//         );
//
//     // Procedural material
//     float materialWave =
//         sin(
//             length(hit) * 18.0 -
//             time * 3.0 +
//             sin(hit.x * 7.0 + time) +
//             cos(hit.z * 9.0 - time * 0.7)
//         );
//
//     float materialMix =
//         0.5 +
//         0.5 * materialWave;
//
//     float3 materialA =
//         float3(
//             0.025,
//             0.12,
//             0.32
//         );
//
//     float3 materialB =
//         float3(
//             0.34,
//             0.025,
//             0.42
//         );
//
//     float3 materialC =
//         float3(
//             0.01,
//             0.65,
//             0.78
//         );
//
//     float3 material =
//         lerp(
//             materialA,
//             materialB,
//             materialMix
//         );
//
//     material =
//         lerp(
//             material,
//             materialC,
//             pow(
//                 abs(materialWave),
//                 7.0
//             )
//         );
//
//     // Energy veins
//     float veins =
//         pow(
//             abs(
//                 sin(
//                     hit.x * 12.0 +
//                     hit.y * 9.0 +
//                     hit.z * 15.0 +
//                     time * 2.2
//                 )
//             ),
//             13.0
//         );
//
//     material +=
//         veins *
//         float3(
//             0.1,
//             0.75,
//             1.0
//         );
//
//     // Lighting
//     color =
//         material *
//         (
//             0.18 +
//             diffuse * shadow * 1.35 +
//             fresnel * 0.85
//         );
//
//     color +=
//         specular *
//         float3(
//             0.55,
//             0.8,
//             1.0
//         ) *
//         shadow;
//
//     // Rim light
//     color +=
//         fresnel *
//         (
//             0.35 +
//             0.25 *
//             sin(
//                 time * 1.4 +
//                 hit.y * 4.0
//             )
//         ) *
//         float3(
//             0.08,
//             0.45,
//             0.9
//         );
//
//     // Atmospheric glow
//     float glow =
//         exp(-depth * 0.085);
//
//     glow *=
//         0.5 +
//         0.5 *
//         sin(
//             time * 2.1 +
//             depth * 4.0
//         );
//
//     color +=
//         glow *
//         float3(
//             0.015,
//             0.06,
//             0.15
//         );
//
//     // Chromatic edge shimmer
//     float chroma =
//         fresnel * 0.035;
//
//     color.r +=
//         chroma *
//         sin(
//             time +
//             hit.y * 5.0
//         );
//
//     color.b +=
//         chroma *
//         cos(
//             time * 1.3 +
//             hit.x * 4.0
//         );
//
//     // HDR-ish tonemap
//     color *= 1.25;
//
//     color =
//         1.0 - exp(-color);
//
//     color =
//         pow(
//             max(color, 0.0),
//             float3(
//                 0.82,
//                 0.82,
//                 0.82
//             )
//         );
//
//     return float4(
//         saturate(color),
//         1.0
//     );
// }
//
[shader("fragment")]
float4 fragMain(VertexOutput input) : SV_Target
{
    float2 p = input.sv_position.xy;
    float2 res = float2(properties.win_x, properties.win_y);

    // Normalized coordinates, centered and aspect-corrected.
    float2 uv = p / res;
    float2 q = (p - 0.5 * res) / min(res.x, res.y);

    float t = properties.power;
    float r = length(q);
    float a = atan2(q.y, q.x);

    // Warp the plane through several animated rotational fields.
    float2 w = q;

    for (int i = 0; i < 7; ++i)
    {
        float fi = float(i);
        float s = sin(t * (0.21 + fi * 0.071) + fi * 1.731);
        float c = cos(t * (0.17 + fi * 0.053) - fi * 2.173);

        float ca = cos(a + fi * 0.83 + s * 0.7);
        float sa = sin(a + fi * 0.83 + s * 0.7);

        float2 rot = float2(
            w.x * ca - w.y * sa,
            w.x * sa + w.y * ca
        );

        float ripple =
            sin(length(rot) * (8.0 + fi * 2.7) - t * (1.2 + fi * 0.31))
            * 0.035;

        w += normalize(rot + 1e-5) * ripple;
        w += float2(
            sin(rot.y * (5.0 + fi) + t * 0.43),
            cos(rot.x * (6.0 + fi * 0.7) - t * 0.37)
        ) * (0.018 + fi * 0.004);
    }

    float rr = length(w);
    float aa = atan2(w.y, w.x);

    // Exploding radial geometry.
    float spiral =
        sin(
            aa * 13.0
            + rr * 31.0
            - t * 2.7
            + sin(rr * 17.0 - t) * 3.0
        );

    float spiral2 =
        cos(
            aa * 29.0
            - rr * 47.0
            + t * 1.9
            + sin(aa * 7.0 + t * 0.8) * 2.0
        );

    float bands =
        sin(rr * 74.0 - t * 5.0 + spiral * 2.5) *
        cos(aa * 9.0 + t * 0.7);

    // Pulsing singularity.
    float pulse = 0.5 + 0.5 * sin(t * 1.7 + rr * 11.0);
    float singular =
        exp(-rr * (3.5 + pulse * 4.0)) *
        (0.5 + 0.5 * sin(aa * 17.0 - t * 3.2));

    // Layered pseudo-fractal turbulence.
    float n = 0.0;
    float amp = 0.55;
    float freq = 3.0;

    for (int j = 0; j < 6; ++j)
    {
        float fj = float(j);

        n += amp *
            sin(w.x * freq + sin(w.y * freq * 1.37 + t * (0.15 + fj * 0.04)))
            *
            cos(w.y * freq * 0.83 - cos(w.x * freq * 1.21 - t * 0.11));

        freq *= 2.03;
        amp *= 0.47;
    }

    n = n * 0.5 + 0.5;

    // Moving energy filaments.
    float filament =
        pow(
            abs(
                sin(
                    aa * 41.0
                    + rr * 21.0
                    - t * 4.5
                    + n * 8.0
                )
            ),
            18.0
        );

    float filament2 =
        pow(
            abs(
                cos(
                    aa * 23.0
                    - rr * 38.0
                    + t * 3.1
                )
            ),
            24.0
        );

    // Chromatic separation driven by the geometry.
    float phaseR = n * 6.28318 + spiral * 1.7 + t * 0.31;
    float phaseG = n * 5.28318 - spiral2 * 1.3 - t * 0.23;
    float phaseB = n * 7.28318 + spiral * 2.1 + t * 0.47;

    float3 col;

    col.r =
        0.10
        + 0.55 * (0.5 + 0.5 * sin(phaseR))
        + 0.65 * filament
        + 0.30 * singular;

    col.g =
        0.04
        + 0.60 * (0.5 + 0.5 * sin(phaseG))
        + 0.75 * filament2
        + 0.25 * singular;

    col.b =
        0.16
        + 0.65 * (0.5 + 0.5 * sin(phaseB))
        + 0.80 * filament
        + 0.55 * singular;

    // Violent interference between the layers.
    col *=
        0.55
        + 0.45 * (0.5 + 0.5 * spiral)
        + 0.35 * bands;

    col += float3(
        filament * 0.9,
        filament2 * 0.8,
        (filament + filament2) * 0.7
    );

    // Central bloom.
    float bloom =
        exp(-rr * rr * 5.0) *
        (0.5 + 0.5 * sin(t * 2.3 + rr * 19.0));

    col += float3(
        bloom * 0.9,
        bloom * 0.55,
        bloom * 1.2
    );

    // Rotating edge shockwave.
    float wave =
        exp(
            -abs(
                sin(rr * 12.0 - t * 1.8 + n * 4.0)
            ) * 11.0
        );

    col += wave * float3(
        0.25 + 0.25 * sin(t),
        0.15 + 0.35 * cos(t * 0.73),
        0.45 + 0.30 * sin(t * 0.41)
    );

    // Vignette and nonlinear color response.
    float vignette = 1.0 - smoothstep(0.25, 1.15, rr);
    col *= vignette;

    col = max(col, 0.0);
    col = col / (1.0 + col);
    col = pow(col, float3(0.72, 0.72, 0.72));

    // Tiny temporal shimmer.
    float shimmer =
        sin(
            p.x * 0.031
            + p.y * 0.047
            + t * 13.0
        ) * 0.018;

    col += shimmer;

    return float4(saturate(col), 1.0);
}
//
// [shader("fragment")]
// float4 fragMain(VertexOutput inVert) : SV_Target
// {
//
//     float3 bulb_colour = {0.8, 0.23, 0.32};
//     float4 nothing_colour = { 0.05, 0.05, 0.05, 1.0 };
//
//     float power = 15.0;
//     float bailout = 20.0;
//     int precision = 150;
//
//     float TOO_FAR = 15;
//
//     float3 ray_dir = get_norm_ray_direction(properties.win_x, properties.win_y, { inVert.sv_position[0], inVert.sv_position[1] });
//
//     float3 ray_position = properties.ray_origin;
//
//     for (int i = 0; i < 10000; i++)
//     {
//         float distance = mandelbulb_msd(
//             ray_position,
//             properties.power 
//         );
//
//         if (isnan(distance))
//             break;
//
//         if (distance < 1e-04)
//         {
//             float3 normal = estimate_normal(ray_position + distance * ray_dir, 0.01, properties.power);
//
//             normal = (normal + 1.0) / 2.0;
//
//             return float4(normal, 1.0);
//         }
//
//         if (distance >= TOO_FAR)
//             return nothing_colour;
//
//         ray_position += distance * ray_dir;
//     }
//
//     return nothing_colour;
//
// }
