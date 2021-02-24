#include <stdio.h>
#include <stdlib.h>

#define SDL_MAIN_HANDLED

#include "SDL.h"
#define GLEW_STATIC
#include <GL/glew.h>

static const GLuint WIDTH = 512;
static const GLuint HEIGHT = 512;

static const GLchar* vertex_shader_source =
"#version 400\n"
"layout(location = 0) in vec2 coord2d;\n"
"void main() {\n"
"    gl_Position = vec4(coord2d, 0.0, 1.0);\n"
"}\n";

static const GLchar* fragment_shader_source =
"#version 400\n"
"layout(location = 0) out vec4 myColor;\n"
"void main() {\n"
"    myColor = vec4(0.0, 1.0, 0.0, 1.0);\n"
"}\n";

static GLfloat vertices[] = {
    -0.4,  0.4,
    -0.8, -0.4,
     0.0, -0.4,
     0.8, -0.4,
     0.4,  0.4
};

static GLshort indices[] = {
     0,1,2,2,3,4,2,0,4
};

GLuint common_get_shader_program(
    const char* vertex_shader_source,
    const char* fragment_shader_source
) {
    GLchar* log = NULL;
    GLint log_length, success;
    GLuint fragment_shader, program, vertex_shader;

    /* Vertex shader */
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
    glCompileShader(vertex_shader);
    
    glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
    log = (GLchar *)malloc(log_length);
    if (log_length > 0) {
        glGetShaderInfoLog(vertex_shader, log_length, NULL, log);
        printf("vertex shader log:\n\n%s\n", log);
    }
    if (!success) {
        printf("vertex shader compile error\n");
        exit(EXIT_FAILURE);
    }

    /* Fragment shader */
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
    glCompileShader(fragment_shader);

    glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
    glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
        log = (GLchar *)realloc(log, log_length);
        glGetShaderInfoLog(fragment_shader, log_length, NULL, log);
        printf("fragment shader log:\n\n%s\n", log);
    }
    if (!success) {
        printf("fragment shader compile error\n");
        exit(EXIT_FAILURE);
    }

    /* Link shaders */
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);


    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
        log = (GLchar*)realloc(log, log_length);
        glGetProgramInfoLog(program, log_length, NULL, log);
        printf("shader link log:\n\n%s\n", log);
    }
    if (!success) {
        printf("shader link error");
        exit(EXIT_FAILURE);
    }

    /* Cleanup. */
    free(log);
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

int main(void) {
    GLint attribute_coord2d;
    GLuint program, vbo[4], vao;
    SDL_Event event;
    SDL_GLContext gl_context;
    SDL_Window* window;

    SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

    my_timer_id = SDL_AddTimer(1000, my_callbackfunc, 0);

    window = SDL_CreateWindow(__FILE__, 0, 0,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    gl_context = SDL_GL_CreateContext(window);
    glewInit();

    /* Shader setup. */
    program = common_get_shader_program(vertex_shader_source, fragment_shader_source);
    attribute_coord2d = glGetAttribLocation(program, "coord2d");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /* Buffer setup. */
    glGenBuffers(2, vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 1);

    /* Global draw state */
    glUseProgram(program);
    glViewport(0, 0, WIDTH, HEIGHT);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    glPolygonMode(GL_FRONT, GL_FILL);
    glPolygonMode(GL_BACK, GL_FILL);

    glPolygonMode(GL_FRONT, GL_LINE);
    glPolygonMode(GL_BACK, GL_LINE);
    /* Main loop. */
    while (1) {
        glClear(GL_COLOR_BUFFER_BIT);

        if (foo & 1) {
            glPolygonMode(GL_FRONT, GL_LINE);
            glPolygonMode(GL_BACK, GL_LINE);
        }
        else {
            glPolygonMode(GL_FRONT, GL_FILL);
            glPolygonMode(GL_BACK, GL_FILL);
        }

        glDrawElements(GL_TRIANGLES, 9, GL_UNSIGNED_SHORT, (void*)0);

        SDL_GL_SwapWindow(window);
        if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
            break;
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);

    /* Cleanup. */
    glDeleteBuffers(2, vbo);
    glDeleteVertexArrays(1, &vao);

    glDeleteProgram(program);
    SDL_RemoveTimer(my_timer_id);

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return EXIT_SUCCESS;
}