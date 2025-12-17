#include <SDL/SDL.h>
#include "glad.h"

#include "gltf_loader.h"

//#include <iostream>

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec4 a_joint;\n"
    "layout (location = 2) in vec4 a_weight;\n"
    "uniform mat4 u_jointMat[2]\n;"
    "uniform mat4 transform;\n"
    "vec4 translation = vec4(0.0, -0.8, 0, 0); \n"
    "void main()\n"
    "{\n"
    "   mat4 skinMat =\n"
    "       a_weight.x * u_jointMat[int(a_joint.x)] +\n"
    "       a_weight.y * u_jointMat[int(a_joint.y)] +\n"
    "       a_weight.z * u_jointMat[int(a_joint.z)] +\n"
    "       a_weight.w * u_jointMat[int(a_joint.w)];\n"
    "   /* vec4 worldPosition = skinMat * vec4(aPos,1.0); */\n"
    "   /* vec4 cameraPosition = u_viewMatrix * worldPosition; */\n;"
    "   /* gl_Position = u_projectionMatrix * cameraPosition; */\n;"

    "   /* gl_Position = transform * vec4(aPos, 1.0);*/\n"
    "   gl_Position = skinMat * vec4(aPos,1.0) + translation;\n"
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
    char* model_file = (char*)"models/simple_skin.gltf";
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
        printf("skins_count: %u\n", data->skins_count);
	}

    float* vertex_array = read_primitive_geometry(primitive);
    int vertex_array_size = float_buffer_size(get_position_accessor(primitive));
    int vertex_count = get_position_accessor(primitive)->count;
    unsigned int* indices_buffer = read_indices(primitive->indices);
    int indices_count = primitive->indices->count;
    print_geometry(vertex_array, primitive);
    print_indices(indices_buffer, primitive);
    print_trs(&data->nodes[0]);
    anim_data = read_animation_data(&data->animations[0].channels[0]);
    print_animation_data(&data->animations[0].channels[0]);
    anim_data->count = data->animations[0].channels[0].sampler->output->count;
    anim_timer.duration = anim_data->time[anim_data->count-1];
    anim_timer.ticks_per_second = 0.001;
	print_skin_data(&data->skins[0]);
	print_bone_data(&data->meshes[0].primitives[0]);
	glm::mat4* inverse_bind_matrices = get_inverse_bind_matrices(&data->skins[0]);
    TRS_Transform joint_trs[2];
    get_trs_transform(&data->nodes[1], &joint_trs[0]);
    get_trs_transform(&data->nodes[2], &joint_trs[1]);
    glm::mat4 joint_current_transform = glm::mat4(1.0f);
    glm::mat4 joint_inverse_bind_matrix = inverse_bind_matrices[1];
    glm::mat4 joint_mat[2];
    Bone_Data* bones = read_bone_data(&data->meshes[0].primitives[0]);

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

    unsigned int VBO[3], VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(3, VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,  indices_count * sizeof(int), indices_buffer, GL_STATIC_DRAW);

    // position attribute
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_array_size, vertex_array, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // joint attribute
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, bones->joints_size, bones->joints, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(1);

    // weight attribute
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, bones->weights_size, bones->weights, GL_STATIC_DRAW);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(0));
    glEnableVertexAttribArray(2);

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

    // uniform location
    unsigned int transformLoc = glGetUniformLocation(shaderProgram, "transform");
    unsigned int jointMatLoc = glGetUniformLocation(shaderProgram, "u_jointMat");

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
        joint_trs[1].rot = update_animation(anim_data, &anim_timer);
        joint_current_transform = joint_trs[1].trans * joint_trs[1].rot * joint_trs[1].scale;
        transform = joint_current_transform * joint_inverse_bind_matrix;
        joint_mat[1] = joint_current_transform * inverse_bind_matrices[1];
        joint_current_transform = joint_trs[0].trans * joint_trs[0].rot * joint_trs[0].scale;
        joint_mat[0] = joint_current_transform * inverse_bind_matrices[0];

        // get matrix's uniform location and set matrix
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));
        glUniformMatrix4fv(jointMatLoc, 2, GL_FALSE, glm::value_ptr(joint_mat[0]));


        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, vertex_count);
        glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0);
        // glBindVertexArray(0); // no need to unbind it every time

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(3, VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);

    free(vertex_array);
    free(indices_buffer);
    free_animation_data(anim_data);
    delete [] inverse_bind_matrices;
    free_bone_data(bones);

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
