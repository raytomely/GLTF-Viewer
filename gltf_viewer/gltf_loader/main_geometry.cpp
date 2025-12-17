#include <SDL/SDL.h>
#include "glad.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

//#include <iostream>

const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

void processInput(void);
void sleep(void);
size_t float_count(cgltf_accessor* accessor);
float* read_accessor(cgltf_accessor* accessor);
float* read_primitive_geometry(cgltf_primitive* primitive);
void print_vertex_data_type(cgltf_primitive* primitive);
void print_geometry(float* geometry, cgltf_primitive* primitive);
void translate_geometry(float x, float y, float z, float *buffer, cgltf_primitive* primitive);
void scale_down_geometry(float factor, float *buffer, cgltf_primitive* primitive);
cgltf_accessor* get_position_accessor(cgltf_primitive* primitive);

#define float_buffer_size(accessor) float_count(accessor) * sizeof(float)

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
    char* model_file = (char*)"models/Agumon/AGUM.gltf";
    cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* data = NULL;
	cgltf_result result = cgltf_parse_file(&options, model_file, &data);

    if (result == cgltf_result_success)
		result = cgltf_load_buffers(&options, data, model_file);

	if (result == cgltf_result_success)
		result = cgltf_validate(data);

	printf("Result: %d\n", result);

	cgltf_mesh* mesh = &data->meshes[1];
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
	}

    float* vertex_array = read_primitive_geometry(primitive);
    int vertex_array_size = float_buffer_size(get_position_accessor(primitive));
    int vertex_count = get_position_accessor(primitive)->count;
    scale_down_geometry(128, vertex_array, primitive);
    translate_geometry(-0.5,0,0,vertex_array, primitive);
    print_geometry(vertex_array, primitive);

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

    // render loop
    // -----------

    while (main_loop)
    {

		// input
		// -----
        processInput();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // draw our first triangle
        glUseProgram(shaderProgram);
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

size_t float_count(cgltf_accessor* accessor)
{
    cgltf_size floats_per_element = cgltf_num_components(accessor->type);
	cgltf_size available_floats = accessor->count * floats_per_element;
	return available_floats;
}

float* read_accessor(cgltf_accessor* accessor)
{
    size_t available_floats = float_count(accessor);
    printf("available_floats=%d\n",available_floats);
    int mem_size = cgltf_accessor_unpack_floats(accessor, NULL, available_floats) * sizeof(float);
    float* float_buffer = (float*)malloc(mem_size);
    cgltf_accessor_unpack_floats(accessor, float_buffer, available_floats);
    printf("mem_size=%d\n",mem_size);
    return float_buffer;
}

int vertex_data_type(cgltf_primitive* primitive)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            return (primitive->attributes[i].data->type);
        }
    }
    return 0;
}

cgltf_accessor* get_position_accessor(cgltf_primitive* primitive)
{
    int i;
    for(i = 0; i < primitive->attributes_count; i++)
    {
        if(primitive->attributes[i].type == cgltf_attribute_type_position)
        {
            return (primitive->attributes[i].data);
        }
    }
    return NULL;
}

void print_vertex_data_type(cgltf_primitive* primitive)
{
    switch(vertex_data_type(primitive))
    {
        case cgltf_type_vec2:
            printf("vertex_data_type: vec2 \n");
            break;
        case cgltf_type_vec3:
            printf("vertex_data_type: vec3 \n");
            break;
        case cgltf_type_vec4:
            printf("vertex_data_type: vec4 \n");
            break;
        default:
            printf("vertex_data_type: is not a vertex data \n");
    }
}

void print_geometry(float* geometry, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)geometry;
    int vertex_count = get_position_accessor(primitive)->count;
    printf("\n ... printing geometry ... \n");
    printf("mesh vertex_count: %d \n",vertex_count);
    int i, num_vertex = 0, triangle = 1;
    for(i = 0; i < vertex_count; i++)
    {
        if(num_vertex == 0)
        {
            printf("triangle %d \n",triangle);
            triangle += 1;
        }
        printf("vertex %d: x=%f y=%f z=%f \n", i, buf_ptr[i].x, buf_ptr[i].y, buf_ptr[i].z);
        (num_vertex += 1) %= 3;
    }
}

float* read_primitive_geometry(cgltf_primitive* primitive)
{
    if(vertex_data_type(primitive) == cgltf_type_vec3)
    {
        return read_accessor(get_position_accessor(primitive));
    }
    else
    {
        printf("vertex data type is not vec3 \n");
    }
    return NULL;
}

float** read_mesh_geometry(cgltf_mesh* mesh)
{
    int primitives_count = mesh->primitives_count;
    float** float_buffers = (float**)malloc(primitives_count);
    int i;
    for(i = 0; i < primitives_count; i++)
    {
        float_buffers[i] = read_primitive_geometry(&mesh->primitives[i]);
    }
    return float_buffers;
}

void free_mesh_geometry(cgltf_mesh* mesh, float** float_buffers)
{
    int i;
    for(i = 0; i < mesh->primitives_count; i++)
    {
        free(float_buffers[i]); float_buffers[i] = NULL;
    }
    free(float_buffers); float_buffers = NULL;
}

void translate_geometry(float x, float y, float z, float *buffer, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)buffer;
    int vertex_count = get_position_accessor(primitive)->count;
    int i;
    for(i = 0; i < vertex_count; i++)
    {
        buf_ptr[i].x += x; buf_ptr[i].y += y; buf_ptr[i].z += z;
    }
}

void scale_down_geometry(float factor, float *buffer, cgltf_primitive* primitive)
{
    typedef struct
    {
        float x,y,z;
    }vec3;
    vec3* buf_ptr = (vec3*)buffer;
    int vertex_count = get_position_accessor(primitive)->count;
    int i;
    for(i = 0; i < vertex_count; i++)
    {
        buf_ptr[i].x /= factor; buf_ptr[i].y /= factor; //buf_ptr[i].z /= factor;
        buf_ptr[i].z = 0;
    }
}

