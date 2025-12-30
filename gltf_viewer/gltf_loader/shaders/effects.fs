#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform sampler2D texture2;

uniform float texcoord_xoffset;
float texture_width = 256.0;


void main()
{
        TexCoord.x = (TexCoord.x + texcoord_xoffset) * texture_width;
        TexCoord.x = mod(TexCoord.x, 80.0);
        if(TexCoord.x < 10.0)
            TexCoord.x += 10.0;
        TexCoord.x = TexCoord.x / texture_width;
        //TexCoord.x = max(TexCoord.x, 10.0 / texture_width);

	// linearly interpolate between both textures (80% container, 20% awesomeface)
	//FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
        FragColor = texture(texture1, TexCoord);
        //FragColor = vec4(1, 1, 0, 1.0);
}