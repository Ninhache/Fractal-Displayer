#version 400
// GLSL analog of OpenCL's `#pragma OPENCL EXTENSION cl_khr_fp64`: fp64 is core in
// GL 4.0, but declaring the extension documents the hard dependency. Adjacent pixels
// collapse to the same coordinate past ~10^4 zoom in 32-bit float; double reaches ~10^13.
#extension GL_ARB_gpu_shader_fp64 : enable

#define PI 3.1415926538

//uniform vec2 v_center;
uniform vec2 resolution;
uniform int iterations;
uniform vec4 pallet[10];
uniform int colors_nb;
uniform bool smoth;
uniform vec4 background_color;

// SFML can't upload double uniforms. Two floats only preserve 48 of a double's 52 bits
// (and splitting df64 hi/lo separately leaves a 4-bit gap → effective ~10^13), so the
// view center is carried as a 5-float expansion: a sum whose terms descend in magnitude
// and, summed in double on the GPU, reconstruct the full ~104-bit df64 (~10^30). scale
// stays a single double — its tiny magnitude needs only relative precision.
uniform float scale_hi;
uniform float scale_lo;
uniform float x_off[5];   // view center X as a 5-float expansion
uniform float y_off[5];   // view center Y as a 5-float expansion
uniform bool  dd_mode;    // true => use the heavy df64 path (engaged only when deep)

// 0 Mandelbrot, 1 Julia, 2 Burning Ship, 3 Tricorn, 4 Multibrot
uniform int  fractal_type;
uniform vec2 julia_c;     // Julia constant (float precision is fine for c)
uniform int  power_d;     // Multibrot exponent (>= 2)
#define FT_MANDELBROT  0
#define FT_JULIA       1
#define FT_BURNING     2
#define FT_TRICORN     3
#define FT_MULTIBROT   4

float modulo(float a, float b) {
	return a - b * floor(a / b);
}

float modulus_2(vec2 z) {
	return z.x * z.x + z.y * z.y;
}

// ---- double-double (df64) arithmetic -------------------------------------------------
// A df64 value is an unevaluated sum hi+lo (lo <= 0.5 ulp(hi)) carried in a dvec2
// (.x=hi, .y=lo), giving ~106 mantissa bits. Standard Bailey/Hida QD algorithms.
double two_sum(double a, double b, out double err) {
	double s = a + b;
	double bb = s - a;
	err = (a - (s - bb)) + (b - bb);
	return s;
}
double quick_two_sum(double a, double b, out double err) {
	double s = a + b;
	err = b - (s - a);
	return s;
}
double two_prod(double a, double b, out double err) {
	double p = a * b;
	err = fma(a, b, -p);   // fp64 fma is core in GLSL 4.0
	return p;
}
dvec2 df_add(dvec2 a, dvec2 b) {
	double s2; double s1 = two_sum(a.x, b.x, s2);
	double t2; double t1 = two_sum(a.y, b.y, t2);
	s2 += t1; s1 = quick_two_sum(s1, s2, s2);
	s2 += t2; s1 = quick_two_sum(s1, s2, s2);
	return dvec2(s1, s2);
}
dvec2 df_mul(dvec2 a, dvec2 b) {
	double p2; double p1 = two_prod(a.x, b.x, p2);
	p2 += a.x * b.y + a.y * b.x;
	p1 = quick_two_sum(p1, p2, p2);
	return dvec2(p1, p2);
}
dvec2 df_neg(dvec2 a) { return dvec2(-a.x, -a.y); }
dvec2 df_sub(dvec2 a, dvec2 b) { return df_add(a, df_neg(b)); }
dvec2 df_abs(dvec2 a) { return a.x < 0.0lf ? df_neg(a) : a; }  // |x|: hi sign decides

// ---- complex arithmetic, plain double (complex = dvec2: x=re, y=im) ------------------
dvec2 cmul(dvec2 a, dvec2 b) { return dvec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x); }
dvec2 csqr(dvec2 z)          { return dvec2(z.x*z.x - z.y*z.y, 2.0lf*z.x*z.y); }
dvec2 cconj(dvec2 z)         { return dvec2(z.x, -z.y); }
dvec2 cabsc(dvec2 z)         { return dvec2(abs(z.x), abs(z.y)); }  // component-wise abs
double cmod2(dvec2 z)        { return z.x*z.x + z.y*z.y; }

// ---- complex arithmetic, df64 (complex = cdf: re/im each a df64 dvec2) ----------------
struct cdf { dvec2 re; dvec2 im; };
cdf cdf_add(cdf a, cdf b)  { cdf r; r.re = df_add(a.re, b.re); r.im = df_add(a.im, b.im); return r; }
cdf cdf_mul(cdf a, cdf b) {
	cdf r;
	r.re = df_sub(df_mul(a.re, b.re), df_mul(a.im, b.im));
	r.im = df_add(df_mul(a.re, b.im), df_mul(a.im, b.re));
	return r;
}
cdf cdf_sqr(cdf z)   { return cdf_mul(z, z); }
cdf cdf_conj(cdf z)  { cdf r; r.re = z.re; r.im = df_neg(z.im); return r; }
cdf cdf_absc(cdf z)  { cdf r; r.re = df_abs(z.re); r.im = df_abs(z.im); return r; }
double cdf_mod2(cdf z) { return df_add(df_mul(z.re, z.re), df_mul(z.im, z.im)).x; } // hi part

// Reconstruct a df64 from the CPU's 5-float expansion (summed exactly via df_add).
dvec2 expand5(float e[5]) {
	dvec2 a = dvec2(double(e[0]), 0.0lf);
	a = df_add(a, dvec2(double(e[1]), 0.0lf));
	a = df_add(a, dvec2(double(e[2]), 0.0lf));
	a = df_add(a, dvec2(double(e[3]), 0.0lf));
	a = df_add(a, dvec2(double(e[4]), 0.0lf));
	return a;
}

vec4 get_color(float iterations, float max_iterations, vec4 pallet[10]) {
	float value = iterations / max_iterations;
	vec4 color = vec4(0.f,0.f,0.f, 1.f);

	float min_value;
	float max_value;

	for (int i = 0; i < colors_nb; i++) {
		min_value = float(i) / float(colors_nb);
		max_value = float((i + 1.0f)) / float(colors_nb);

		if (value >= min_value && value <= max_value) {
			color = mix(pallet[i], pallet[i + 1], (value - min_value) * colors_nb);
			break;
		}
	}

	return color;
}

//https://www.math.univ-toulouse.fr/~cheritat/wiki-draw/index.php/Mandelbrot_set
vec4 newColorisation(vec4 old_color, float V) {
	float x = log(V) / 1.f;

	vec4 new_color = old_color * ((1 + cos(2 * PI * x)) / 2.f);

	return new_color;
}

/*
vec4 quasiperiodicColorisation(vec4 old_color, float V) {
	float x = log(V) / 1.f;
	
	vec3 abc = vec3(1, (1.f / (3.f * sqrt(2.f))), 1.f / (7.f * pow(3, (1.f/8.f)))) * (1.f / log(2));

	vec4 new_color = vec4((255.f * ((1.f - (cos(abc[0] * x)))/2.f)),
		 				  (255.f * ((1.f - (cos(abc[1] * x)))/2.f)),
		 				  (255.f * ((1.f - (cos(abc[2] * x)))/2.f)),
		 				  1.f);



	
	return new_color;
}
*/


/*
vec4 get_color(float current_iteration, float max_iterations, vec4 current_pallet[7], int colors_nb) {
	float value = iterations / max_iterations;
	vec4 color = vec4(1.f, 1.f, 1.f, 1.f);

	float min_value;
	float max_value;

	for (int i = 0; i < int(colors_nb); i++) {
		min_value = float(i) / colors_nb;
		max_value = float(i + 1) / colors_nb;

		if (value >= min_value && value <= max_value) {
			color = mix(pallet[i], pallet[i + 1], (value - min_value) * colors_nb);
			break;
		}
	}
	return color;
}

vec4 get_color(float current_iteration, float max_iterations, vec4 current_pallet[7], int colors_nb) {
	
    float value = current_iteration / max_iterations;
	vec4 color = vec4(1.f, 1.f, 1.f, 1.f);

	for (int i = 0; i < colors_nb; i++) {
		float min_value = float(i) / float(colors_nb);
		float max_value = float(i) / float(colors_nb);

		if (value >= min_value && value <= max_value){
			color = mix(current_pallet[i], current_pallet[i + 1], float(value - min_value) * float(colors_nb));
			break;
		}
	}

	return color;
} 

vec4 get_color(float current_iteration, float max_iterations, vec4 current_pallet[7], int colors_nb) {
	float value = iterations / max_iterations;
	vec4 color = vec4(1.f, 1.f, 1.f, 1.f);

	float min_value;
	float max_value;

	for (int i = 0; i < colors_nb; i++) {
		min_value = float(i) / colors_nb;
		max_value = float(i + 1) / colors_nb;

		if (value >= min_value && value <= max_value) {
			color = mix(pallet[i], pallet[i + 1], (value - min_value) * colors_nb);
			break;
		}
	}

	return color;
}
*/	
void main(void) {
	double scale = double(scale_hi) + double(scale_lo);
	// Pixel base term is O(1); times the tiny scale it is a precise plain double. Only
	// the subtraction of the large offset needs extra precision (done per-path below).
	double base_x = double(2.0 * gl_FragCoord.x - resolution.x) / double(resolution.y);
	double base_y = double(2.0 * gl_FragCoord.y - resolution.y) / double(resolution.y);
	double dx = base_x * scale;
	double dy = base_y * scale;

	double max_modulus = smoth ? 100.0lf : 4.0lf;
	int i = 0;
	float mod2;   // |z|^2 at escape (hi part), in float — plenty for coloring

	if (dd_mode) {
		// ---- double-double path (~10^30) -------------------------------------------
		cdf p;
		p.re = df_sub(dvec2(dx, 0.0lf), expand5(x_off));   // pixel coordinate (df64)
		p.im = df_sub(dvec2(dy, 0.0lf), expand5(y_off));

		cdf z, c;
		if (fractal_type == FT_JULIA) {
			z = p;
			c.re = dvec2(double(julia_c.x), 0.0lf);
			c.im = dvec2(double(julia_c.y), 0.0lf);
		} else {
			z.re = dvec2(0.0lf, 0.0lf); z.im = dvec2(0.0lf, 0.0lf);
			c = p;
		}

		while (i < iterations) {
			if (cdf_mod2(z) >= max_modulus) break;
			cdf w = z;
			if (fractal_type == FT_BURNING) w = cdf_absc(z);
			cdf zz;
			if (fractal_type == FT_TRICORN)        zz = cdf_sqr(cdf_conj(w));
			else if (fractal_type == FT_MULTIBROT) { zz = w; for (int k = 1; k < power_d; k++) zz = cdf_mul(zz, w); }
			else                                   zz = cdf_sqr(w);
			z = cdf_add(zz, c);
			i++;
		}
		mod2 = float(cdf_mod2(z));
	} else {
		// ---- fast plain-double path (~10^13) ---------------------------------------
		// First three expansion terms reconstruct the offset to ~72 bits (full double).
		dvec2 p;   // complex: x=re, y=im
		p.x = dx - (double(x_off[0]) + double(x_off[1]) + double(x_off[2]));
		p.y = dy - (double(y_off[0]) + double(y_off[1]) + double(y_off[2]));

		dvec2 z, c;
		if (fractal_type == FT_JULIA) { z = p; c = dvec2(double(julia_c.x), double(julia_c.y)); }
		else                          { z = dvec2(0.0lf, 0.0lf); c = p; }

		while (i < iterations) {
			if (cmod2(z) >= max_modulus) break;
			dvec2 w = (fractal_type == FT_BURNING) ? cabsc(z) : z;
			dvec2 zz;
			if (fractal_type == FT_TRICORN)        zz = csqr(cconj(w));
			else if (fractal_type == FT_MULTIBROT) { zz = w; for (int k = 1; k < power_d; k++) zz = cmul(zz, w); }
			else                                   zz = csqr(w);
			z = zz + c;
			i++;
		}
		mod2 = float(cmod2(z));
	}

	vec4 color;
	if (i == iterations) {
		color = background_color;
	} else {
		if (smoth) {
			// log(|z|) = 0.5*log(|z|^2); transcendentals only on escaped pixels.
			float smooth_value = float(i + 1) - log(0.5 * log(mod2)) / log(2.0);
			color = get_color(smooth_value, float(iterations), pallet);
		} else {
			color = get_color(float(i % int(float(iterations / 10.f))), float(iterations / 10.f), pallet);
		}
	}

	gl_FragColor = color;
}