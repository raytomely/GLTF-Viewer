#include <SDL/SDL.h>
#include <dirent.h>
#include "gltf_loader/glad.h"

#include "gltf_loader/gltf_loader.h"

#include "gltf_loader/shader_s.h"
#include "gltf_loader/camera.h"
#include "gltf_loader/filesystem.h"

#include <iostream>


typedef struct
{
    unsigned int files_count;
    char** files_names;
}Files_List;

void processInput(void);
void sleep(void);
int listdir(char* dir_name);
Files_List* get_files_list(char* dir_name, char* extension_name);
void free_files_list(Files_List* files_list);
void join_path(char* path, char* filename, char* output_path);
Model_Data* load_gltf_model_2(char* model_file);
void free_model_2(Model_Data* model);
void model_transform(Shader *shader);

int model_file_index = 0;
int model_files_count;
int change_model = false;

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

    if(argc > 1 && argv[1][0] == '-' && argv[1][1] == 'h')
    {
        printf("help: \n\n");
        printf("gltf_viewer.exe file_name [model_version:(1,2,3)] \n\n");
        return 0;
    }

    // load gltf file
    //char* model_file = (char*)"models/Agumon/AGUM.gltf";
    char current_file_path[200];
    char files_path[] = "models/Effects_dw1/"; //listdir(files_path);
    Files_List* gltf_files = get_files_list("models/Effects_dw1/", "gltf");
    join_path(files_path, gltf_files->files_names[model_file_index], current_file_path);
    if(argc > 1)
        strcpy(current_file_path, argv[1]);
    Model_Data* model = load_gltf_model_2(current_file_path);
    model_files_count = gltf_files->files_count;
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
    Shader ourShader("gltf_loader/shaders/camera.vs", "gltf_loader/shaders/camera.fs");

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

        if(change_model)
        {
             free_model_2(model);
             join_path(files_path, gltf_files->files_names[model_file_index], current_file_path);
             model = load_gltf_model_2(current_file_path);

             change_model = false;
        }

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
        model_transform(&ourShader);

        // render model
        Mesh_Data* mesh;
        // bind textures on corresponding texture units
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->texture);
        // render model
        for (unsigned int i = 0; i < model->meshes_count; i++)
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
    free_files_list(gltf_files);

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
                        model_file_index += 1;
                        if(model_file_index > model_files_count-1)
                            model_file_index = 0;
                        change_model = true;
                        break;
                    case SDLK_o:
                        model_file_index -= 1;
                        if(model_file_index < 0)
                            model_file_index = model_files_count-1;
                        change_model = true;
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


int is_file(char* file_name, char* extension_name)
{
    char* dot = strchr(file_name, '.');
    if (dot)
    {
        if(strcmp(dot+1, extension_name) == 0)
            return 1;
        else
            return 0;
    }
    else
        return 0; // it must be a directory
}

int listdir(char* dir_name)
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir (dir_name);
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL)
        {
            //puts (ep->d_name);
            printf("name: %s is_gltf_file: %d \n", ep->d_name, is_file(ep->d_name, "gltf"));
        }

        closedir (dp);
        return 0;
    }
    else
    {
        perror ("Couldn't open the directory");
        return -1;
    }
}

int get_files_count(char* dir_name, char* extension_name)
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir (dir_name);
    int files_count = 0;
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL)
        {
            if(is_file(ep->d_name, extension_name))
                files_count++;
        }

        closedir (dp);
        return files_count;
    }
    else
    {
        perror ("Couldn't open the directory");
        return -1;
    }
}

Files_List* get_files_list(char* dir_name, char* extension_name)
{
    DIR *dp;
    struct dirent *ep;
    dp = opendir (dir_name);
    int files_count = 0;
    Files_List* files_list = NULL;
    if (dp != NULL)
    {
        while ((ep = readdir (dp)) != NULL)
        {
            if(is_file(ep->d_name, extension_name))
                files_count++;
        }
        if(files_count == 0)
            return NULL;
        rewinddir(dp);
        files_list = (Files_List*)malloc(sizeof(Files_List));
        files_list->files_names = (char**)malloc(sizeof(char*) * files_count);
        files_list->files_count = files_count;
        int i = 0;
        while ((ep = readdir (dp)) != NULL)
        {
            if(is_file(ep->d_name, extension_name))
            {
                files_list->files_names[i] = (char*)malloc(sizeof(char) * strlen(ep->d_name));
                strcpy(files_list->files_names[i], ep->d_name);
                i++;
            }
        }

        closedir (dp);
        return files_list;
    }
    else
    {
        perror ("Couldn't open the directory");
        return NULL;
    }
}

void free_files_list(Files_List* files_list)
{
    for(int i = 0; i < files_list->files_count; i++)
    {
        free(files_list->files_names[i]);  files_list->files_names[i] = NULL;
    }
    free(files_list->files_names);  files_list->files_names = NULL;
    free(files_list);  files_list = NULL;

}

void join_path(char* path, char* filename, char* output_path)
{
    int len = strlen(path);
    strcpy(output_path, path);
    output_path[len] = '/'; len++;
    strcpy(&output_path[len], filename);
    len += strlen(filename);
    output_path[len] = '\0';
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
        model->texture = load_texture_from_memory(&gltf_data->textures[0], &options);
    }

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
