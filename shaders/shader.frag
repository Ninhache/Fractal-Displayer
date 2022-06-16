#version 330
precision highp float;


#define PI 3.1415926538

//uniform vec2 v_center;
uniform vec2 resolution;
uniform int iterations;
uniform vec4 pallet[10];
uniform int colors_nb;
uniform float scale;
uniform bool smoth;
uniform vec4 background_color;
uniform float x_offset;
uniform float y_offset;

float modulo(float a, float b) {
	return a - b * floor(a / b);
}

float modulus_2(vec2 z) {
	return z.x * z.x + z.y * z.y;
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

	vec2 center = ((2.0 * gl_FragCoord.xy - resolution.xy) / resolution.y) * scale ;
    center.x -= x_offset;
	center.y -= y_offset;

	int i = 0;
	vec2 number = vec2(0.0f, 0.0f);
	vec2 temp = vec2(0.0f, 0.0f);
	float max_modulus;
	if (smoth) {
		max_modulus = 100.f;
	} else {
		max_modulus = 4.f;
	}
	
	while (modulus_2(number) < max_modulus && i < iterations) {
		temp = number;
		number.x = temp.x * temp.x - temp.y * temp.y + center.x;
		number.y = 2.f * temp.x * temp.y + center.y;
		i++;
	}
	
	float smooth_value = float(i + 1.f) - log(log(length(number))) / log(2.f);

	float V = log(modulus_2(number))/1.f;
	vec4 color;
	if (i == iterations) {
		color = background_color;
	} else {
		if (smoth) {
			//color = get_color(smooth_value, (float)max_iterations, pallet, 7);
			//color = get_color(modulo(smooth_value, float(iterations / 10.f)), float(iterations / 10.f), pallet);
			color = get_color(smooth_value, float(iterations), pallet);
			//color = newColorisation(color, V);
			//color = quasiperiodicColorisation(color, V);
		} else {
			//color = get_color(i, max_iterations, pallet, 7);
			color = get_color(float(i % int(float(iterations / 10.f))), float(iterations / 10.f), pallet);
		}
		
	}

	gl_FragColor = color;
}