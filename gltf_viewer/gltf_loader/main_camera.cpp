#include <SDL/SDL.h>
#include "glad.h"

#include "gltf_loader.h"

#include "shader_s.h"
#include "camera.h"
#include "filesystem.h"

#include <iostream>

void processInput(void);
void sleep(void);

// settings
const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 480;

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

bool main_loop = true;
SDL_Event event;
Uint8* keys;

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("gltf_viewer",NULL);
    SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);//|SDL_RESIZABLE);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // load gltf file
    char* model_file = (char*)"models/Agumon/AGUM.gltf";
    cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* gltf_data = NULL;
	cgltf_result result = cgltf_parse_file(&options, model_file, &gltf_data);

    if (result == cgltf_result_success)
		result = cgltf_load_buffers(&options, gltf_data, model_file);

	if (result == cgltf_result_success)
		result = cgltf_validate(gltf_data);

	printf("Result: %d\n", result);

	cgltf_mesh* mesh = &gltf_data->meshes[1];
	cgltf_primitive* primitive = &mesh->primitives[0];
	cgltf_texture* gltf_texture = &gltf_data->textures[0];
	std::string  texture_image = "models/Agumon/";
	//texture_image += gltf_data->textures->image->uri;

	if (result == cgltf_result_success)
	{
		printf("Type: %u\n", gltf_data->file_type);
		printf("Meshes: %u\n", (unsigned)gltf_data->meshes_count);
		printf("primitives_count: %u\n",mesh->primitives_count);
		print_vertex_data_type(primitive);
		printf("buffer_size: %u\n",gltf_data->buffers->size);
		printf("buffers_count: %u\n",gltf_data->buffers_count);
		printf("accessors_count: %u\n",gltf_data->accessors_count);
        printf("textures_count: %u\n",gltf_data->textures_count);
        printf("texture_image: %s \n",texture_image.c_str());
	}

	unsigned char *image_buffer;
	const char* comma = strchr(gltf_data->textures->image->uri, ',');
	unsigned int base64_size = buffer_base64_size((char*)comma + 1);
	cgltf_load_buffer_base64(&options, base64_size, comma + 1, (void**)&image_buffer);
    float* vertex_array = read_primitive_geometry(primitive);
    float* texcoord = read_primitive_texcoord(primitive);
    //unsigned int* indices_buffer = read_indices(primitive->indices);
    int vertex_array_size = float_buffer_size(get_position_accessor(primitive));
    int vertex_count = get_position_accessor(primitive)->count;
    int texcoord_size = float_buffer_size(get_texcoord_accessor(primitive));
    //scale_down_geometry(128, vertex_array, primitive);
    //translate_geometry(-0.5,0,0,vertex_array, primitive);
    //print_geometry(vertex_array, primitive);
    //print_texcoord(texcoord, primitive);
    //print_indices(indices_buffer, primitive);
    int texture_wrap_s = gltf_texture->sampler->wrap_s;
    int texture_wrap_t = gltf_texture->sampler->wrap_t;
    int texture_min_filter = gltf_texture->sampler->min_filter;
    int texture_mag_filter = gltf_texture->sampler->mag_filter;

	cgltf_free(gltf_data);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("gltf_loader/shaders/camera.vs", "gltf_loader/shaders/camera.fs");

    unsigned int VBO[2], VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBO);

    glBindVertexArray(VAO);

    // position attribute
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertex_array_size, vertex_array, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, texcoord_size, texcoord, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    ourShader.setInt("texture1", 0);
    ourShader.setInt("texture2", 1);

    // load and create a texture
    // -------------------------
    unsigned int texture;
    glGenTextures(1, &texture);
    // if your texture code doesn't work or shows up as completely black
    // that's because On some drivers it is required to assign a texture unit to each sampler uniform
    // GL_TEXTURE0 is always by default activated
    //glActiveTexture(GL_TEXTURE0); // activate the texture unit first before binding texture
    glBindTexture(GL_TEXTURE_2D, texture); // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, texture_wrap_s);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, texture_wrap_t);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texture_min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texture_mag_filter);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    //stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    unsigned char *data = stbi_load_from_memory(image_buffer, base64_size, &width, &height, &nrChannels, 0);
    void (*memory_free)(void*, void*) = options.memory.free_func ? options.memory.free_func : &cgltf_default_free;
    memory_free(options.memory.user_data, image_buffer);
    if (data)
    {
        // note that png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (main_loop)
    {
        // per-frame time logic
        // --------------------
        float currentFrame = static_cast<float>(SDL_GetTicks());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // activate shader
        ourShader.use();

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection);

        // camera/view transformation
        glm::mat4 view = camera.GetViewMatrix();
        ourShader.setMat4("view", view);

        // render model
        glBindVertexArray(VAO);
        // calculate the model matrix for each object and pass it to shader before drawing
        glm::mat4 model = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
        model = glm::translate(model, glm::vec3(-9.0f, 0.0f, -50.0f)); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
        float angle = 20.0f;
        model = glm::rotate(model, glm::radians(angle), glm::vec3(1.0f, 0.3f, 0.5f));
        ourShader.setMat4("model", model);

        glDrawArrays(GL_TRIANGLES, 0, vertex_count);

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(2, VBO);

    free(vertex_array);
    free(texcoord);

    SDL_Quit();
    return 0;
}


// process all input: query whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(void)
{
    if(SDL_PollEvent(&event) == 1)
    {
        switch(event.type)
        {
            case SDL_QUIT:
                main_loop = false;
                break;
            /*case SDL_VIDEORESIZE:
                SDL_SetVideoMode(event.resize.w, event.resize.h, 32, SDL_OPENGL|SDL_RESIZABLE);
                glViewport(0, 0, event.resize.w, event.resize.h);
                break;*/
            case SDL_MOUSEMOTION:
            {
                float xpos = static_cast<float>(event.motion.x);
                float ypos = static_cast<float>(event.motion.y);

                if (firstMouse)
                {
                    lastX = xpos;
                    lastY = ypos;
                    firstMouse = false;
                }

                float xoffset = xpos - lastX;
                float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

                lastX = xpos;
                lastY = ypos;

                camera.ProcessMouseMovement(xoffset, yoffset);
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                if (event.button.button == SDL_BUTTON_WHEELUP)
                {
                    camera.ProcessMouseScroll(static_cast<float>(2.0f));
                }
                else if (event.button.button == SDL_BUTTON_WHEELDOWN)
                {
                    camera.ProcessMouseScroll(static_cast<float>(-2.0f));
                }
                break;
            }

        }
    }

    keys = SDL_GetKeyState(NULL);

    if(keys[SDLK_ESCAPE])
        main_loop = 0;

    if(keys[SDLK_w])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    else if(keys[SDLK_a])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[SDLK_s])
        camera.ProcessKeyboard(LEFT, deltaTime);
    else if(keys[SDLK_d])
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if(keys[SDLK_UP])
        camera.ProcessMouseMovement(0, 10);
    else if(keys[SDLK_DOWN])
        camera.ProcessMouseMovement(0, -10);
    if(keys[SDLK_LEFT])
        camera.ProcessMouseMovement(-10, 0);
    else if(keys[SDLK_RIGHT])
        camera.ProcessMouseMovement(10, 0);
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
