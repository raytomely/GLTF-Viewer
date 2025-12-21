#include <SDL/SDL.h>
#include "glad.h"

#include "gltf_loader.h"

#include "shader_s.h"
#include "camera.h"
#include "filesystem.h"

#include <iostream>


void processInput(void);
void sleep(void);
void dw1_model_transform(Shader *shader);
void dw2_model_transform(Shader *shader);
void dw3_model_transform(Shader *shader);

int animation_index = 0;
int animations_count;
int change_animation = true;

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
        printf("animations_count: %d \n",gltf_data->animations_count);
	}

    Model_Data* model = load_model(gltf_data, &options);
    animations_count = model->animations_count;
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

	cgltf_free(gltf_data);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("gltf_loader/shaders/model.vs", "gltf_loader/shaders/model.fs");

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    while (main_loop)
    {
        // per-frame time logic
		// --------------------
		currentFrame = SDL_GetTicks();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

        if(change_animation)
        {
             change_model_animation(model, animation_index);
             change_animation = false;
        }

        update_skeletal_animation(model, deltaTime);

        // input
        // -----
        processInput();

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // activate shader
        ourShader.use();

        // pass projection matrix to shader (note that in this case it could change every frame)
        glm::mat4 projection_mat = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        ourShader.setMat4("projection", projection_mat);

        // camera/view transformation
        glm::mat4 view_mat = camera.GetViewMatrix();
        ourShader.setMat4("view", view_mat);

        // calculate the model matrix for each object and pass it to shader before drawing
        dw1_model_transform(&ourShader);

        // render model
        draw_model(model, ourShader.ID);

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteProgram(ourShader.ID);

    //free_model_animation(animation);
    free_model(model);

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
                    case SDLK_p:
                        animation_index += 1;
                        if(animation_index > animations_count-1)
                            animation_index = 0;
                        change_animation = true;
                        break;
                    case SDLK_o:
                        animation_index -= 1;
                        if(animation_index < 0)
                            animation_index = animations_count-1;
                        change_animation = true;
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

void dw1_model_transform(Shader *shader)
{
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    model_mat = glm::translate(model_mat, glm::vec3(10.0f, 3.0f, 20.0f)); // translate it down so it's at the center of the scene
    model_mat = glm::scale(model_mat, glm::vec3(0.02f, 0.02f, 0.02f));
    model_mat = glm::rotate(model_mat, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model_mat = glm::rotate(model_mat, glm::radians(210.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, &model_mat[0][0]);
}

void dw2_model_transform(Shader *shader)
{
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    model_mat = glm::translate(model_mat, glm::vec3(10.0f, -20.0f, -40.0f)); // translate it down so it's at the center of the scene
    model_mat = glm::scale(model_mat, glm::vec3(0.02f, 0.02f, 0.02f));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, &model_mat[0][0]);
}

void dw3_model_transform(Shader *shader)
{
    glm::mat4 model_mat = glm::mat4(1.0f); // make sure to initialize matrix to identity matrix first
    model_mat = glm::translate(model_mat, glm::vec3(10.0f, 3.0f, 20.0f)); // translate it down so it's at the center of the scene
    model_mat = glm::rotate(model_mat, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    model_mat = glm::rotate(model_mat, glm::radians(210.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shader->ID, "model"), 1, GL_FALSE, &model_mat[0][0]);
}

