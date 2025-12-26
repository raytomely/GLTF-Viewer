#version 330 core
out vec4 FragColor;

in vec3 ourColor;
in vec2 TexCoord;

// texture sampler
uniform sampler2D texture1;

uniform vec4 font_color;

void main()
{
        //FragColor = vec4(1, 1, 0, 1.0);
	FragColor = texture(texture1, TexCoord) ; // * font_color;
	// mix the resulting texture color with the vertex colors
        //FragColor = texture(texture1, TexCoord) * vec4(ourColor, 1.0);
}