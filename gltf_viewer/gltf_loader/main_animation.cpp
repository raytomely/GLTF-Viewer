#include <SDL/SDL.h>
#include "glad.h"

#include "gltf_loader.h"

//#include <iostream>

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 transform;"
    "void main()\n"
    "{\n"
    "   gl_Position = transform * vec4(aPos, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";


void processInput(void);
void sleep(void);

Animation_Data*  anim_data = NULL;
Animation_Timer  anim_timer;

bool main_loop = true;
SDL_Event event;
Uint8* keys;

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("gltf_viewer",NULL);
    SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        //std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // load gltf file
    char* model_file = (char*)"models/simple_animation.gltf";
    cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, model_file, &data);

    if (result == cgltf_result_success)
		result = cgltf_load_buffers(&options, data, model_file);

	if (result == cgltf_result_success)
		result = cgltf_validate(data);

	printf("Result: %d\n", result);

	cgltf_mesh* mesh = &data->meshes[0];
	cgltf_primitive* primitive = &mesh->primitives[0];

	if (result == cgltf_result_success)
	{
		printf("Type: %u\n", data->file_type);
		printf("Meshes: %u\n", (unsigned)data->meshes_count);
		printf("primitives_count: %u\n",mesh->primitives_count);
		print_vertex_data_type(primitive);
		printf("buffer_size: %u\n",data->buffers->size);
		printf("buffers_count: %u\n",data->buffers_count);
		printf("accessors_count: %u\n",data->accessors_count);
		printf("animations_count: %u\n",data->animations_count);
		printf("animation_sampler_count: %u\n",data->animations[0].samplers_count);
		printf("animation_channel_count: %u\n",data->animations[0].channels_count);
	}

    float* vertex_array = read_primitive_geometry(primitive);
    int vertex_array_size = float_buffer_size(get_position_accessor(primitive));
    int vertex_count = get_position_accessor(primitive)->count;
    print_geometry(vertex_array, primitive);
    print_trs(&data->nodes[0]);
    anim_data = read_animation_data(&data->animations[0].channels[0]);
    print_animation_data(&data->animations[0].channels[0]);
    anim_data->count = data->animations[0].channels[0].sampler->output->count;
    anim_timer.duration = anim_data->time[anim_data->count-1];
    anim_timer.ticks_per_second = 0.001;

	cgltf_free(data);

    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        //std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        //std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        //std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertex_array_size, vertex_array, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // timing
    float deltaTime = 0.0f;	// time between current frame and last frame
    float lastFrame = 0.0f;
    float currentFrame = 0.0f;
    anim_timer.delta_time = deltaTime;

    // render loop
    // -----------

    while (main_loop)
    {
        // per-frame time logic
		// --------------------
		currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		anim_timer.delta_time = deltaTime;

		// input
		// -----
        processInput();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // create transformations
        glm::mat4 transform = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        transform = update_animation(anim_data, &anim_timer);

        // get matrix's uniform location and set matrix
        glUseProgram(shaderProgram);
        unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        // glBindVertexArray(0); // no need to unbind it every time

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    free(vertex_array);
    free_animation_data(anim_data);

    SDL_Quit();

    return 0;
}

void processInput(void)
{
    if(SDL_PollEvent(&event) == 1)
    {
        switch(event.type)
        {
            case SDL_QUIT:
                main_loop = false;
                break;
        }
    }
}

void sleep(void)
{
    static int old_time = 0,  actual_time = 0;
    actual_time = SDL_GetTicks();
    if (actual_time - old_time < 16) // if less than 16 ms has passed
    {
        SDL_Delay(16 - (actual_time - old_time));
        old_time = SDL_GetTicks();
    }
    else
    {
        old_time = actual_time;
    }
}
