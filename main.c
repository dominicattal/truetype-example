#include <stdio.h>
#include "glad.h"
#include "glfw.h"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 500

#define BITMAP_WIDTH 1024
#define FONT_SIZE 16
#define CHAR_OFFSET 32
#define NUM_CHARS 96
#define STRIDE_IN_BYTES 0
#define PADDING 1
#define TTF_BUFFER_SIZE ((1<<20)*sizeof(char))
#define OFFSET 0
#define FONT_INDEX 0
#define NUM_RANGES 1

unsigned char ttf_buffer[TTF_BUFFER_SIZE];
unsigned char bitmap[BITMAP_WIDTH*BITMAP_WIDTH];
stbtt_fontinfo info;
stbtt_pack_range font_range;
stbtt_packedchar chars[NUM_CHARS];
stbtt_pack_context spc;
GLuint tex;
GLuint shader;

const char* vertex_shader_source = 
    "#version 430\n"
    "layout (location = 0) in vec2 position;\n"
    "layout (location = 1) in vec2 tex_coord_in;\n"
    "out vec2 tex_coord;\n"
    "void main() {\n"
    "   gl_Position = vec4(position.x, position.y, 0.0, 1.0);\n"
    "   tex_coord = tex_coord_in;\n"
    "}\n";

const char* fragment_shader_source = 
    "#version 430\n"
    "out vec4 FragColor;\n"
    "in vec2 tex_coord;\n"
    "uniform sampler2D bitmap;\n"
    "void main() {\n"
    "   FragColor = texture(bitmap, tex_coord);\n"
    "}\n";

void init_font(void)
{
    stbtt_PackBegin(&spc, bitmap, BITMAP_WIDTH, BITMAP_WIDTH, STRIDE_IN_BYTES, PADDING, NULL);

    char* path;
    //path = "/usr/share/fonts/truetype/dejavu/DejaVuSerif.ttf";
    path = "../soultaker/assets/fonts/Space_Mono/SpaceMono-Regular.ttf";

    FILE* fptr = fopen(path, "rb");
    fread(ttf_buffer, sizeof(char), TTF_BUFFER_SIZE, fptr);
    fclose(fptr);

    stbtt_InitFont(&info, ttf_buffer, OFFSET);

    font_range.font_size = FONT_SIZE;
    font_range.first_unicode_codepoint_in_range = CHAR_OFFSET; 
    font_range.array_of_unicode_codepoints = NULL;
    font_range.num_chars = NUM_CHARS;       
    font_range.chardata_for_range = chars;

    stbtt_PackFontRanges(&spc, ttf_buffer, FONT_INDEX, &font_range, NUM_RANGES);

    stbtt_PackEnd(&spc);

    //for  (int i = 0; i < BITMAP_WIDTH*BITMAP_WIDTH; i++)
    //    bitmap[i] = 255;

    stbi_write_png("out.png", BITMAP_WIDTH, BITMAP_WIDTH, 1, bitmap, 0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, BITMAP_WIDTH, BITMAP_WIDTH, 0, GL_RED, GL_UNSIGNED_BYTE, bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLint swizzle_mask[] = {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED};
    glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzle_mask);

    GLuint vert, frag;
    int success;
    char info[512];
    shader = glCreateProgram();
    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &vertex_shader_source, NULL);
    glCompileShader(vert);
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vert, 512, NULL, info);
        puts(info);
        exit(1);
    }
    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &fragment_shader_source, NULL);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag, 512, NULL, info);
        puts(info);
        exit(1);
    }
    glAttachShader(shader, vert);
    glAttachShader(shader, frag);
    glLinkProgram(shader);
    glGetProgramiv(frag, GL_LINK_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, info);
        puts(info);
        exit(1);
    }

    glUseProgram(shader);
    int loc = glGetUniformLocation(shader, "bitmap");
    glUniform1i(loc, 0);
}

GLfloat normalized_x(GLfloat x)
{
    return 2 * (x / WINDOW_WIDTH - 0.5);
}

GLfloat normalized_y(GLfloat y)
{
    return 2 * ((WINDOW_HEIGHT-y) / WINDOW_HEIGHT - 0.5);
}

void push(GLfloat* d, int i, stbtt_aligned_quad q)
{
    d[24*i+0]=normalized_x(q.x0); d[24*i+1]=normalized_y(q.y0); d[24*i+2]=q.s0; d[24*i+3]=q.t0;
    d[24*i+4]=normalized_x(q.x1); d[24*i+5]=normalized_y(q.y0); d[24*i+6]=q.s1; d[24*i+7]=q.t0;
    d[24*i+8]=normalized_x(q.x1); d[24*i+9]=normalized_y(q.y1); d[24*i+10]=q.s1; d[24*i+11]=q.t1;
    d[24*i+12]=normalized_x(q.x0); d[24*i+13]=normalized_y(q.y0); d[24*i+14]=q.s0; d[24*i+15]=q.t0;
    d[24*i+16]=normalized_x(q.x1); d[24*i+17]=normalized_y(q.y1); d[24*i+18]=q.s1; d[24*i+19]=q.t1;
    d[24*i+20]=normalized_x(q.x0); d[24*i+21]=normalized_y(q.y1); d[24*i+22]=q.s0; d[24*i+23]=q.t1;
}

void render_text(float x, float y, char* text)
{
    int n = strlen(text);
    size_t size = n*4*6*sizeof(GLfloat);
    GLfloat* data = malloc(size);
    GLuint vao, vbo;
    int cnt = 0;

    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] < CHAR_OFFSET && text[i] >= CHAR_OFFSET+NUM_CHARS)
            continue;
        stbtt_aligned_quad q;
        stbtt_GetPackedQuad(chars, BITMAP_WIDTH, BITMAP_WIDTH, text[i]-CHAR_OFFSET, &x, &y, &q, 0);
        push(data, cnt, q);
        for (int k = 0; k < 6; k++) {
            printf("%f %f %f %f\n", data[24*cnt+4*k], data[24*cnt+4*k+1], data[24*cnt+4*k+2], data[24*cnt+4*k+3]);
       }
        puts("");
        cnt++;
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glUseProgram(shader);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 6 * cnt);

    free(data);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Hello World", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    init_font();
    while (!glfwWindowShouldClose(window)) {
        glClearColor(220, 220, 220, 255);
        glClear(GL_COLOR_BUFFER_BIT);
        render_text(20, 50, "The quick brown fox jumps over the lazy dog");
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
