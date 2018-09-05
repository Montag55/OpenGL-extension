#ifndef GLAPPLICATION_HPP
#define GLAPPLICATION_HPP
#define _USE_MATH_DEFINES

#include <GL/glew.h>
#include <GL/gl.h>
#include <cmath>
#include <cerrno>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <streambuf>
#include "structs.hpp"
#include "window.hpp"

///GLM INCLUDES
#define GLM_FORCE_RADIANS
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>

class GLApplication{

  public:
    GLApplication();
    ~GLApplication();
    std::vector<float> render(std::vector<std::vector<float>*> pictures, std::vector<float> weights);
    void cleanupSSBOs();


  protected:
    std::string readFile(std::string const& file);
    GLuint loadShaders(std::string const& cs);
    GLuint loadShader(GLenum type, std::string const& s);
    GLuint createProgram(std::string const& comp);

    void read_Computation();
    void read_Texture();
    void do_Computation(GLuint program);
    void update_StorageBuffer(Pstruct pstruct);
    void update_StorageBuffer(std::vector<float> input_buffer);
    void update_Texture();
    void update_PixelBuffer(PBO pbo, std::vector<float> input_buffer);

    void initialCheck();
    void initializePrograms();
    void initializeStorageBuffers();
    void initializeTextures();
    void initPixelBuffer();
    void initializeFramBuffer();

    glm::vec3   g_background_color;
    glm::ivec2  g_window_res;
    std::string g_error_message;
    bool g_reload_shader_error;

    Window g_win;
    GLuint g_sum_program;
    GLuint g_weighted_sum_program;
    GLuint g_tex_program;
    GLsizei m_layerCount;

    std::vector<float> result_container;
    std::vector<Tex_obj> tex_container;
    std::vector<const char*> uniform_names;
    std::vector<PBO> pbo_container;

    PBO p_pbo;
    Tex_obj srcTex;
    Tex_obj destTex;
    SSBO ssbo_weights;


    unsigned int m_num_pix;
    unsigned int m_work_group_size;
    glm::ivec2 m_res;
};

#endif
