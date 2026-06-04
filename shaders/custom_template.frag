#version 400
#extension GL_ARB_gpu_shader_fp64 : enable
// Custom-function template. main.cpp substitutes %FORMULA% with the user's recurrence
// for the next z (c = pixel coordinate, z starts at 0), then recompiles via loadFromMemory.
// Double precision only (~10^13 zoom) — keeps it simple and robust.

uniform vec2  resolution;
uniform int   iterations;
uniform vec4  pallet[10];
uniform int   colors_nb;
uniform bool  smoth;
uniform vec4  background_color;
uniform float scale_hi;
uniform float scale_lo;
uniform float x_off[5];
uniform float y_off[5];

// ---- complex helpers (complex = dvec2: x=re, y=im) ----
const dvec2 i = dvec2(0.0lf, 1.0lf);                              // imaginary unit
dvec2 cadd (dvec2 a, dvec2 b) { return a + b; }
dvec2 csub (dvec2 a, dvec2 b) { return a - b; }
dvec2 cmul (dvec2 a, dvec2 b) { return dvec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); }
dvec2 csqr (dvec2 z)          { return dvec2(z.x*z.x - z.y*z.y, 2.0lf*z.x*z.y); }
dvec2 cdiv (dvec2 a, dvec2 b) { double d = b.x*b.x + b.y*b.y; return dvec2((a.x*b.x + a.y*b.y)/d, (a.y*b.x - a.x*b.y)/d); }
dvec2 cconj(dvec2 z)          { return dvec2(z.x, -z.y); }
dvec2 cabsc(dvec2 z)          { return dvec2(abs(z.x), abs(z.y)); }   // component-wise abs
dvec2 cexp (dvec2 z)          { float e = exp(float(z.x)); return dvec2(double(e*cos(float(z.y))), double(e*sin(float(z.y)))); }
double cmod2(dvec2 z)         { return z.x*z.x + z.y*z.y; }

vec4 get_color(float it, float max_iterations, vec4 pal[10]) {
	float value = it / max_iterations;
	vec4 color = vec4(0.f, 0.f, 0.f, 1.f);
	float min_value, max_value;
	for (int k = 0; k < colors_nb; k++) {
		min_value = float(k) / float(colors_nb);
		max_value = float(k + 1) / float(colors_nb);
		if (value >= min_value && value <= max_value) {
			color = mix(pal[k], pal[k + 1], (value - min_value) * colors_nb);
			break;
		}
	}
	return color;
}

void main(void) {
	double scale = double(scale_hi) + double(scale_lo);
	double base_x = double(2.0 * gl_FragCoord.x - resolution.x) / double(resolution.y);
	double base_y = double(2.0 * gl_FragCoord.y - resolution.y) / double(resolution.y);
	double max_modulus = smoth ? 100.0lf : 4.0lf;

	dvec2 c;   // pixel coordinate (~72-bit double offset)
	c.x = base_x * scale - (double(x_off[0]) + double(x_off[1]) + double(x_off[2]));
	c.y = base_y * scale - (double(y_off[0]) + double(y_off[1]) + double(y_off[2]));
	dvec2 z = dvec2(0.0lf, 0.0lf);

	int n = 0;
	while (n < iterations) {
		if (cmod2(z) >= max_modulus) break;
		z = %FORMULA%;
		n++;
	}
	float mod2 = float(cmod2(z));

	vec4 color;
	if (n == iterations) {
		color = background_color;
	} else if (smoth) {
		float smooth_value = float(n + 1) - log(0.5 * log(mod2)) / log(2.0);
		color = get_color(smooth_value, float(iterations), pallet);
	} else {
		color = get_color(float(n % int(float(iterations / 10.f))), float(iterations / 10.f), pallet);
	}

	gl_FragColor = color;
}
