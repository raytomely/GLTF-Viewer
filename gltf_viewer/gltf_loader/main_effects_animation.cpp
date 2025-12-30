#include <SDL/SDL.h>
#include <dirent.h>
#include "gltf_loader/glad.h"

#include "gltf_loader/gltf_loader.h"

#include "gltf_loader/shader_s.h"
#include "gltf_loader/camera.h"
#include "gltf_loader/filesystem.h"

#include <iostream>


void processInput(void);
void sleep(void);
Model_Data* load_gltf_model_2(char* model_file);
void free_model_2(Model_Data* model);
void model_transform(Shader *shader);

// settings
const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 480;

// camera
Camera camera(glm::vec3(10.0f, 5.0f, 40.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;
float currentFrame = 0.0f;

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
    //char* model_file = (char*)"models/Agumon/AGUM.gltf";
    char* model_file = (char*)"models/Effects_dw1/0_efe.gltf";
    Model_Data* model = load_gltf_model_2(model_file);
    //model_animation* animation = load_model_animation(&gltf_data->animations[0], gltf_data->nodes, model->anim_nodes);
    /*for(int i = 0; i < animation->anim_data_count; i++)
    {
        if(i % 3 == 0)
        {
            printf("\n\n *** node=%d  *** \n\n", i/3);
            printf("*** node_children_count=%d  *** \n\n", model->anim_nodes[i/3]->children_count);
        }
        print_animation_data_2(animation->anim_data[i]);
    }*/
    //load_animation_frame(model, animation, 0);



    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("gltf_loader/shaders/effects.vs", "gltf_loader/shaders/effects.fs");

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    float texcoord_xoffset = 0.01;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


    // render loop
    // -----------
    while (main_loop)
    {
        // per-frame time logic
		// --------------------
		currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;


        // input
        // -----
        processInput();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate shader
        ourShader.use();

        ourShader.setFloat("texcoord_xoffset", texcoord_xoffset);
        texcoord_xoffset += 0.001;

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection_mat = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection_mat);

        // camera/view transformation
        glm::mat4 view_mat = camera.GetViewMatrix();
        ourShader.setMat4("view", view_mat);

        // calculate the model matrix for each object and pass it to shader before drawing
        model_transform(&ourShader);

        // render model
        Mesh_Data* mesh;
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->texture);
        // render model
        for (unsigned int i = 0; i < model->meshes_count; i++)
        //for (unsigned int i = 0; i < 1; i++)
        {
            mesh = model->meshes[i];
            glBindVertexArray(mesh->VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh->vertices_count);
        };

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteProgram(ourShader.ID);

    //free_model_animation(animation);
    free_model_2(model);

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
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        main_loop = 0;
                        break;
                }
                break;
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

void export_texture(unsigned char* buffer, size_t buffer_size, char* out_file_name)
{
    FILE* file = fopen(out_file_name, "wb");
    fwrite(buffer, buffer_size, 1, file);
    fclose(file);
}

unsigned int load_texture_from_memory_2(cgltf_texture* gltf_texture, cgltf_options* options)
{
    unsigned char *image_buffer;
	const char* comma = strchr(gltf_texture->image->uri, ',');
	unsigned int base64_size = buffer_base64_size((char*)comma + 1);
	void (*memory_free)(void*, void*) = options->memory.free_func ? options->memory.free_func : &cgltf_default_free;
	cgltf_load_buffer_base64(options, base64_size, comma + 1, (void**)&image_buffer);

    int texture_wrap_s = gltf_texture->sampler->wrap_s;
    int texture_wrap_t = gltf_texture->sampler->wrap_t;
    int texture_min_filter = gltf_texture->sampler->min_filter;
    int texture_mag_filter = gltf_texture->sampler->mag_filter;

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
    memory_free(options->memory.user_data, image_buffer);
    //printf("width: %d  height: %d \n", width, height);
    if (data)
    {
        // note that png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        printf("Failed to load texture \n");
    }
    stbi_image_free(data);

    return texture;
}

Model_Data* load_gltf_model_2(char* model_file)
{
    cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));
	cgltf_data* gltf_data = NULL;
	cgltf_result result = cgltf_parse_file(&options, model_file, &gltf_data);

    if (result == cgltf_result_success)
		result = cgltf_load_buffers(&options, gltf_data, model_file);
    else
        printf("could not parse gltf file: %s \n", model_file);

	if (result == cgltf_result_success)
		result = cgltf_validate(gltf_data);
    else
         printf("could not load buffers ! \n");

    Model_Data* model = NULL;

    if(result == cgltf_result_success)
    {
        unsigned int meshes_count = gltf_data->meshes_count;
        model = (Model_Data*)malloc(sizeof(Model_Data));
        model->meshes = (Mesh_Data**)malloc(sizeof(Mesh_Data*) * meshes_count);
        model->meshes_count = meshes_count;
        for(unsigned int i = 0; i < meshes_count; i++)
        {
            model->meshes[i] = load_mesh(&gltf_data->meshes[i]);
        }
        model->texture = load_texture_from_memory_2(&gltf_data->textures[0], &options);
    }

    //print_texcoord((float*)model->meshes[0]->texcoord, &gltf_data->meshes[0].primitives[0]);

    cgltf_free(gltf_data);

    return model;
}

void free_model_2(Model_Data* model)
{
    for(unsigned int i = 0; i < model->meshes_count; i++)
    {
        free_mesh(model->meshes[i]); model->meshes[i] = NULL;
    }
    free(model->meshes);  model->meshes = NULL;
    glDeleteTextures(1, &model->texture);
    free(model);  model = NULL;
}

void model_transform(Shader *shader)
{
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    model_mat = glm::translate(model_mat, glm::vec3(10.0f, 3.0f, 20.0f)); // translate it down so it's at the center of the scene
    model_mat = glm::scale(model_mat, glm::vec3(0.02f, 0.02f, 0.02f));
    model_mat = glm::rotate(model_mat, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model_mat = glm::rotate(model_mat, glm::radians(210.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, &model_mat[0][0]);
}
