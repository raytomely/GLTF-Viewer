#include <SDL/SDL.h>
#include "glad.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "shader_s.h"
#include "filesystem.h"

#include <iostream>

void processInput(void);
void sleep(void);
void blit_char(char c, int x, int y, unsigned int* font, unsigned int* dest);
void print_string(char *s, int x, int y, unsigned int* font, unsigned int* dest);
void set_color_key(unsigned int* img_pixels, unsigned int alpha_color, int img_width, int img_height);

// settings
const unsigned int SCR_WIDTH = 640;
const unsigned int SCR_HEIGHT = 480;

bool main_loop = true;
SDL_Event event;

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

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("gltf_loader/shaders/font.vs", "gltf_loader/shaders/font.fs");

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    /*float vertices[] = {
        // positions          // colors           // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left
    };*/
    /*float vertices[] = {
        // positions          // colors           // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // top right
         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f  // top left
    };*/
    float vertices[] = {
        // positions          // colors           // texture coords
         0.75f*640,  0.25f*480, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f, // top right
         0.75f*640, 0.75f*480, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f, // bottom right
        0.25f*640, 0.75f*480, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f, // bottom left
        0.25f*640,  0.25f*480, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 0.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;
    //stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
    // The FileSystem::getPath(...) is part of the GitHub repository so we can find files on any IDE/platform; replace it with your own image path.
    unsigned char *data = stbi_load("gltf_loader/textures/CGA16x16thick.png", &width, &height, &nrChannels, STBI_rgb_alpha);
    //unsigned char *data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);
    unsigned char blit_surf[width * height * 4];
    unsigned char font[width * height * 4];
    memset(blit_surf, 0, sizeof(blit_surf));
    if (data)
    {
        set_color_key((unsigned int*) data, ((unsigned int*) data)[0], width, height);
        memcpy(font, data, width * height * 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(data);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    ourShader.use();
    ourShader.setVec4("font_color", glm::vec4(0.0f, 1.0f, 1.0f, 1.0f));

    blit_char('c', 32, 0, (unsigned int*)font, (unsigned int*)blit_surf);
    blit_char('t', 32, 32, (unsigned int*)font, (unsigned int*)blit_surf);
    print_string("cocola", 32, 16, (unsigned int*)font, (unsigned int*)blit_surf);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, blit_surf);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, blit_surf);

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

        // bind Texture
        glBindTexture(GL_TEXTURE_2D, texture);

        // render
        ourShader.use();
        //glm::mat4 projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);
        glm::mat4 projection =  glm::ortho(0.0f, 640.0f, 480.0f, 0.0f, -1.0f, 1.0f);
        ourShader.setMat4("projection", projection);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        SDL_GL_SwapBuffers();
        sleep();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    SDL_Quit();
    return 0;
}

// process all input: query whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(void)
{
    if(SDL_PollEvent(&event) == 1)
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

#define CHAR_COLUMNS 16
#define CHAR_WIDTH 16
#define CHAR_HEIGHT 16
#define CHAR_OFFSET 0

#define FONT_WIDTH 256

#define BLIT_BUFFER_WIDTH 256

void blit_char(char c, int x, int y, unsigned int* font, unsigned int* dest)
{
    int char_x = ((c - CHAR_OFFSET) % CHAR_COLUMNS) * CHAR_WIDTH;
    int char_y = ((c - CHAR_OFFSET) / CHAR_COLUMNS) * CHAR_HEIGHT;

    unsigned int* dst_buffer = dest + y * BLIT_BUFFER_WIDTH + x;
    unsigned int* src_bitmap = font + char_y * FONT_WIDTH + char_x;

    for(y = 0; y < CHAR_HEIGHT; y++)
    {
        for(x = 0; x < CHAR_WIDTH; x++)
        {
            //if(src_bitmap[x] != 0xff00ff)
            if((src_bitmap[x] & 0xff000000) != 0)
                dst_buffer[x] = 0xffffffff;
            //else
                //dst_buffer[x] = bg_color;
        }
        dst_buffer += BLIT_BUFFER_WIDTH;
        src_bitmap += FONT_WIDTH;;
    }
}

void print_string(char *s, int x, int y, unsigned int* font, unsigned int* dest)
{
    int i; char c;
    for (i = 0; i < strlen(s); i++)
    {
        c = s[i];
        blit_char(c, x, y, font, dest);
        x += CHAR_WIDTH;
    }
}

void set_color_key(unsigned int* img_pixels, unsigned int alpha_color, int img_width, int img_height)
{
    unsigned int* pixels  = (unsigned int*) img_pixels;
    for(int i = 0; i < img_width * img_height; i++)
    {
        if(pixels[i] == alpha_color)
            pixels[i] &= 0x00ffffff;
    }
}


