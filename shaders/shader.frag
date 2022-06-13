#version 330
precision highp float;

uniform vec2 resolution;
uniform int iterations;
uniform vec4 pallet[7];
uniform float scale;
uniform bool smoth;

float modulo(float a, float b) {
	return a - b * floor(a / b);
}

float modulus_2(vec2 z) {
	return z.x * z.x + z.y * z.y;
}

vec4 get_color(float iterations, float max_iterations, vec4 pallet[7], int colors_nb) {
	float value = iterations / max_iterations;
	vec4 color = vec4(1.f, 1.f, 1.f, 1.f);

	float min_value;
	float max_value;

	for (int i = 0; i < int(colors_nb); i++)
	{
		min_value = float(i) / colors_nb;
		max_value = float((i + 1)) / colors_nb;

		if (value >= min_value && value <= max_value) {
			color = mix(pallet[i], pallet[i + 1], (value - min_value) * colors_nb);
			break;
		}
	}

	return color;
}

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
}*/


/*
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
}*/

/*
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

	vec2 center = ((2.0 * gl_FragCoord.xy - resolution.xy) / resolution.y) * scale;
    center.x -= 0.5;

	int i = 0;
	vec2 number = vec2(0.0f, 0.0f);
	vec2 temp = vec2(0.0f, 0.0f);
	float max_modulus = 4.f;
	
	while (modulus_2(number) < max_modulus && i < iterations) {
		temp = number;
		number.x = temp.x * temp.x - temp.y * temp.y + center.x;
		number.y = 2.f * temp.x * temp.y + center.y;
		i++;
	}

	//>float smooth_value = float(i) + 1.0f - log(log(length(number))) / log(2.0f);
	
	
	
	float smooth_value = float(i + 1.f) - log(log(length(number))) / log(2.f);
	/*
	color = get_color(modulo(smooth_value, (float)max_iterations / 10.f), (float)max_iterations / 10.f, pallet, colors_nb);
	color = get_color(i % (max_iterations / 10), max_iterations / 10, pallet, colors_nb);
	*/


	vec4 color;
	if (i == iterations) {
		color = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	} else {
		if (smoth) {
			//color = get_color(smooth_value, (float)max_iterations, pallet, 7);
			color = get_color(modulo(smooth_value, float(iterations / 10.f)), float(iterations / 10.f), pallet, 7);
		} else {
			//color = get_color(i, max_iterations, pallet, 7);
			color = get_color(float(i % (iterations / 10)), float(iterations / 10), pallet, 7);
		}
		
	}

	gl_FragColor = color;
}