/* 
    all-in-one basic OpenGL example 
    created Feb 2021 Eric Ameres
    using STB for graphics loading
    GLEW for OpenGL api wrangling
    SDL for general applpication "stuff"
*/

#include <stdio.h>
#include <stdlib.h>

#define SDL_MAIN_HANDLED

#define STB_IMAGE_IMPLEMENTATION

#include <stb_image.h>
#include <string>

#include "SDL.h"
#define GLEW_STATIC
#include <GL/glew.h>

static const GLuint WIDTH = 512;
static const GLuint HEIGHT = 512;

static const GLchar* vertex_shader_source =
"#version 400\n"
"layout(location = 0) in vec2 coord2d;\n"
"layout(location = 1) in vec2 vertexUV;\n"
"out vec2 fragmentUV;\n"
"out vec4 interpColor;\n"
"void main() {\n"
"    fragmentUV = vertexUV;\n"
"    interpColor = vec4(coord2d, 0.0, 1.0);\n"
"    gl_Position = vec4(coord2d, 0.0, 1.0);\n"
"}\n";

static const GLchar* fragment_shader_source =
"#version 400\n"
"layout(location = 0) out vec4 myColor;\n"
"uniform vec4 eColor;\n"
"uniform int mode;\n"
"uniform sampler2D u_texture;\n"
"in vec2 fragmentUV;\n"
"in vec4 interpColor;\n"
"void main() {\n"
"    if (mode == 0) myColor = vec4(1.0, 0.0, 0.0, 1.0);\n"     // this version uses a fixed color
"    if (mode == 1) myColor = interpColor;\n"     // this version uses an interpolated color
"    if (mode == 2) myColor = eColor;\n"                         // this version pulls changing color from uniform
"    if (mode == 3) myColor = texture(u_texture, fragmentUV);\n"   // this version pulls color from texture
"}\n";

static GLfloat positionCoordinates[] = {
    -0.4,  0.4,
    -0.8, -0.4,
     0.0, -0.4,
     0.8, -0.4,
     0.4,  0.4
}; 

static GLfloat uv[] = {
    -0.0,  0.8,
    -0.4, -0.0,
     0.4, -0.0,
     1.2, -0.0,
     0.8,  0.8
};

static GLshort indices[] = {
     0,1,2,2,3,4,2,0,4
};


GLuint texture_from_file(const char* path)
{
    /* first up, resolve the path according to the executable */
    extern char g_root_path[256];
    std::string fullpath = g_root_path;
    fullpath += path;
    
    /* next, load the image from the file using stb_image */
    int width, height, channels_in_file;
    uint8_t* data = stbi_load(fullpath.c_str(), &width, &height, &channels_in_file, 4);
    
    if (!data)
    {
        printf("texture load error\n");
        exit(EXIT_FAILURE);
    }

    /* finally generate a texture, bind it, describe it, and load the image into it */
    GLuint handle;

    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);

    /* no longer need the stb_image */
    stbi_image_free(data);

    return handle;
}


void check_shader_errors(GLuint shader, char *name) 
{  
    GLchar* log = NULL;
    GLint log_length, success;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    log = (GLchar*)malloc(log_length);
    if (log_length > 0) {
        glGetShaderInfoLog(shader, log_length, NULL, log);
        printf("%s log:\n\n%s\n",name, log);
    }
    free(log);
    if (!success) {
        printf("%s error\n",name);
        exit(EXIT_FAILURE);
    }
}

GLuint common_get_shader_program(
    const char* vertex_shader_source,
    const char* fragment_shader_source
) {
    GLuint fragment_shader, program, vertex_shader;

    /* Vertex shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    check_shader_errors(vertex_shader, "vertex shader compile");

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    check_shader_errors(fragment_shader, "fragment shader compile");

    /* Link shaders */
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    check_shader_errors(program, "shader program link");

    /* Cleanup. */
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

int foo = 0;
SDL_TimerID my_timer_id;

Uint32 my_callbackfunc(Uint32 interval, void* param)
{
    // using this as a hack to flash between filled and unfilled triangles
    foo++;
    my_timer_id = SDL_AddTimer(1000, my_callbackfunc, 0);
    return Uint32();
}

static void set_root_path(const char* exepath);

int main(int argc, const char** argv)
{

    set_root_path(argv[0]);

    GLuint program, vbo[4], vao, eLoc, tHandle[2], tLoc, mLoc;
    SDL_Event event;
    SDL_GLContext gl_context;
    SDL_Window* window;

    GLfloat colorVecBlue[] = { 0.0,0.0,1.0,1.0 };
    GLfloat colorVecRed[] = { 1.0,0.0,0.0,1.0 };

    // first we need to set up SLD and glew 
    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

    my_timer_id = SDL_AddTimer(1000, my_callbackfunc, 0);

    window = SDL_CreateWindow(__FILE__, 0, 0,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    gl_context = SDL_GL_CreateContext(window);
    glewInit();

    // done with "global system stuff"

    /* Shader setup. create a program from a vertex and fragment shader combo */
    program = common_get_shader_program(vertex_shader_source, fragment_shader_source);

    /* Vertex Array Object setup. */
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* Vertex Buffer setup. */
    glGenBuffers(3, vbo);

    // first buffer... positions of the vertices
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positionCoordinates), positionCoordinates, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // second buffer... UV coordinates for texturing (or other games)
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uv), uv, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // third buffer, indices for the vertices used in the geometry
    // notice that we are creating 3 triangles with only 5 vertices ! (some triangles share vertices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // tell OpenGL to use our shader program
    glUseProgram(program); 

    /* Global draw state */
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // the "uniform variables" are used to send data shared by ALL vertices to the shaders

    /* send the color as a uniform vec4 */
    eLoc = glGetUniformLocation(program, "eColor");
    glUniform4fv(eLoc, 1, colorVecBlue); 
    
    // which texture unit to use (different from which texture to use)
    tLoc = glGetUniformLocation(program, "u_texture");
    glUniform1i(tLoc, 0); // 0: GL_TEXTURE0

    // select between our custom fragment shader modes
    mLoc = glGetUniformLocation(program, "mode");
    glUniform1i(mLoc, 0); // "mode 0"

    // set up 2 textures

    tHandle[0] = texture_from_file("data/textures/magic.png");
    tHandle[1] = texture_from_file("data/textures/brillo.png");

    // set the  fill modes for polygons

    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    /* Main loop. */
    while (1) {

        glClear(GL_COLOR_BUFFER_BIT); // clear the background on each iteration

        //  I set up a few "modes" to switch between showing off different renders based on the same vao and vbos
        switch (foo & 3) {
            case 0:
                glUniform1i(mLoc, 3); // mode 3 : use texture
                 glBindTexture(GL_TEXTURE_2D, tHandle[0]);

                 glPolygonMode(GL_FRONT, GL_FILL);
                 glPolygonMode(GL_BACK, GL_FILL);

                break;
            case 1:
                glUniform1i(mLoc, 3); // mode 3 : use texture
                glBindTexture(GL_TEXTURE_2D, tHandle[1]);
                
                glPolygonMode(GL_FRONT, GL_FILL);
                glPolygonMode(GL_BACK, GL_FILL);

                break;
            case 2:
                glUniform1i(mLoc, 0); // mode 0 : use hardcoded color

                glPolygonMode(GL_FRONT, GL_LINE);
                glPolygonMode(GL_BACK, GL_LINE);
                
                break;
            case 3:
                glUniform1i(mLoc, 2); // mode 2 : use specified color
                glUniform4fv(eLoc, 1, colorVecBlue);

                // to get fancy we decrement the blue channel to "pulse"
                colorVecBlue[2] -= .01;
                if (colorVecBlue[2] < 0)
                    colorVecBlue[2] = 1.0;

                glPolygonMode(GL_FRONT, GL_FILL);
                glPolygonMode(GL_BACK, GL_FILL);

                break;
            default:
                break;
        }

        /* here's where we actually trigger the rendering, yup, just these few lines! */
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_SHORT, (void*)0);

        /* SDL needs to do a window buffer swap */
        SDL_GL_SwapWindow(window);

        /* a bit of user interface housekeeping */
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
            break; // QUIT!!
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    /* Cleanup. */
    glDeleteTextures(2, tHandle);
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);

    glDeleteProgram(program);
    SDL_RemoveTimer(my_timer_id);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}

char g_root_path[256];
static void set_root_path(const char* exepath)
{
    strcpy_s(g_root_path, sizeof(g_root_path), exepath);

    // Strip the executable file name off the end of the path:
    char* slash = strrchr(g_root_path, '\\');
    if (!slash)
    {
        slash = strrchr(g_root_path, '/');
    }
    if (slash)
    {
        slash[1] = '\0';
    }
}
