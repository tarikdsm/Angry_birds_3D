shader_type spatial;
render_mode blend_mix, depth_prepass_alpha, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2D albedo_texture : source_color, filter_nearest_mipmap, repeat_enable;
uniform vec4 rim_tint : source_color = vec4(0.57, 0.882, 0.918, 1);
uniform float rim_power : hint_range(0.5, 8.0) = 3.2;

uniform float authored_roughness = 0.18;
uniform float authored_metallic = 0.0;
uniform float transmission_weight = 0.78;
uniform float coat_weight = 0.24;

void fragment() {
	vec4 texel = texture(albedo_texture, UV);
	float rim = pow(1.0 - max(dot(NORMAL, VIEW), 0.0), rim_power);
	ALBEDO = texel.rgb;
	ROUGHNESS = authored_roughness;
	METALLIC = authored_metallic;
	SPECULAR = clamp(0.5 + coat_weight * 0.5, 0.0, 1.0);
	ALPHA = texel.a;
	EMISSION = rim_tint.rgb * rim * (0.12 + transmission_weight * 0.10);
}
