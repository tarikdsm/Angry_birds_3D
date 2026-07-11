shader_type spatial;
render_mode blend_mix, depth_prepass_alpha, cull_back, diffuse_burley, specular_schlick_ggx;

uniform vec4 glass_tint : source_color = vec4(0.2, 0.72, 0.78, 0.56);
uniform vec4 rim_tint : source_color = vec4(0.57, 0.882, 0.918, 1);
uniform float rim_power : hint_range(0.5, 8.0) = 3.2;

void fragment() {
	float rim = pow(1.0 - max(dot(NORMAL, VIEW), 0.0), rim_power);
	ALBEDO = mix(glass_tint.rgb, rim_tint.rgb, rim * 0.58);
	ROUGHNESS = 0.16;
	METALLIC = 0.02;
	SPECULAR = 0.78;
	ALPHA = clamp(glass_tint.a + rim * 0.28, 0.0, 1.0);
	EMISSION = rim_tint.rgb * rim * 0.22;
}
