#include "./../include/glapplication.hpp"

GLApplication::GLApplication() {
  g_background_color = glm::vec3(0.08f, 0.08f, 0.08f);
  g_window_res = glm::ivec2(800, 600);
  g_reload_shader_error = false;
  g_win.resize(g_window_res);
  g_sum_program = 0;
  g_weighted_sum_program = 0;
  g_tex_program = 0;
  m_layerCount = 10;
  m_num_pix = 1920*1080*3;
  m_work_group_size = 10;
  m_res = glm::ivec2(1920*3, 1080);

  initialCheck();
  initPixelBuffer();
  initializePrograms();
  initializeTextures();
  initializeStorageBuffers();
}

std::vector<float> GLApplication::render(std::vector<std::vector<float>*> pictures, std::vector<float> weights) {

  std::vector<float> test;
  std::vector<float> t_weights;
  unsigned int counter = 0;
  for(int i = 0; i < m_num_pix; i++){
    test.push_back(1.0f);
  }
  for(int i = 0; i < m_layerCount; i++){
    t_weights.push_back(1.0f);
  }

  while (!g_win.shouldClose()) {
    for(unsigned int i = 0; i < m_layerCount; i++){
      update_PixelBuffer(pbo_container[i], test);
    }

    update_Texture();
    update_StorageBuffer(t_weights);
    do_Computation(g_tex_program);
    read_Texture();

    g_win.update();

    if(counter >= 2){
      return result_container;
      g_win.pause();
    }
    counter += 1;
  }
}

void GLApplication::initializePrograms() {
  const std::string g_file_tex_shader("./../shader/tex.comp");

  try {
    g_tex_program = loadShaders(g_file_tex_shader);
  }
  catch (std::logic_error& e) {
      std::stringstream ss;
      ss << e.what() << std::endl;
      g_error_message = ss.str();
      g_reload_shader_error = true;
  }
}

void GLApplication::initializeStorageBuffers() {
  glGenBuffers(1, &ssbo_weights.handle);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_weights.handle);
  glBufferData(GL_SHADER_STORAGE_BUFFER, m_layerCount * sizeof(float), NULL, GL_DYNAMIC_DRAW);

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error initSBO: " << err << std::endl;
  }
}

void GLApplication::initializeTextures() {
  uniform_names.push_back("destTex");
  uniform_names.push_back("srcTex");

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &destTex.handle);
  glBindTexture(GL_TEXTURE_2D, destTex.handle);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_res.x, m_res.y, 0, GL_RED, GL_FLOAT, NULL);
  glBindImageTexture(0, destTex.handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

  glActiveTexture(GL_TEXTURE1);
  glGenTextures(1, &srcTex.handle);
  glBindTexture(GL_TEXTURE_2D_ARRAY, srcTex.handle);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  // glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, m_res.x, m_res.y, m_layerCount, 0, GL_RED, GL_FLOAT, NULL);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, m_res.x, m_res.y, m_layerCount);
  glBindImageTexture(1, srcTex.handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error initTex: " << err << std::endl;
  }
}

void GLApplication::initPixelBuffer() {
  glGenBuffers(1, &p_pbo.handle);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  glBufferData(GL_PIXEL_PACK_BUFFER, m_num_pix * sizeof(float), 0, GL_DYNAMIC_DRAW);

  for(unsigned int i = 0; i < m_layerCount; i++){
    PBO pbo;
    glGenBuffers(1, &pbo.handle);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.handle);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_num_pix * sizeof(float), 0, GL_DYNAMIC_DRAW);
    pbo_container.push_back(pbo);
  }

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error initPBO: " << err << std::endl;
  }
}

void GLApplication::initialCheck() {
  std::cout << "OpenGL  version: " << glGetString(GL_VERSION) << std::endl;

  int work_grp_cnt[3];
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);

  std::cout << "max global (total) work group size x: "
            << work_grp_cnt[0] << " y: "
            << work_grp_cnt[1] << " z: "
            << work_grp_cnt[2] << std::endl;

  int work_grp_size[3];
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
  std::cout << "max local (in one shader) work group sizes x: "
            << work_grp_size[0] << " y: "
            << work_grp_size[1] << " z: "
            << work_grp_size[2] << std::endl;

  int work_grp_inv;
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
  std::cout << "max local work group invocations " << work_grp_inv << std::endl;


  int uniform_comp;
  glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &work_grp_inv);
  std::cout<< "max uniform components: "<< uniform_comp << std::endl;

  int image_max_comp;
  glGetIntegerv(GL_MAX_IMAGE_UNITS, &image_max_comp);
  std::cout<< "max image unites: "<< image_max_comp << std::endl;

  int c_attachments_max_comp;
  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &c_attachments_max_comp);
  std::cout<< "max color_attachments unites: "<< c_attachments_max_comp << std::endl;

  int drawbuffer_max_comp;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawbuffer_max_comp);
  std::cout<< "max drawbuffer unites: "<< drawbuffer_max_comp << std::endl;

  int ssbo_max_bindings;
  glGetIntegerv(GL_MAX_DRAW_BUFFERS, &ssbo_max_bindings);
  std::cout<< "max ssbo bindings: "<< ssbo_max_bindings << std::endl;

  int uniform_max_blocks;
  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uniform_max_blocks);
  std::cout << "max uniform block size: " << uniform_max_blocks << std::endl;

  int max_tex_size;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_tex_size);
  std::cout << "max texture size: " << max_tex_size << std::endl;

  int max_array_tex_layers;
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_array_tex_layers);
  std::cout << "max array texture layers: " << max_array_tex_layers << std::endl;
  std::cout << "------------------------------------------------------------\n" << std::endl;
}

void GLApplication::read_Texture() {
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, destTex.handle);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, 0);

  std::vector<float> tmp;
  tmp.reserve(m_num_pix);
  GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
  memcpy(tmp.data(), ptr, m_num_pix * sizeof(float));
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);


  for(int i = 0; i < 4; i++){
    std::cout << tmp[i] << ", ";
  }
  std::cout << " " << std::endl;

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error readTex: " << err << std::endl;
  }
}

void GLApplication::do_Computation(GLuint program) {
  glUseProgram(program);
  glDispatchCompute(1920*3/ m_work_group_size, 1080/m_work_group_size, 1);
  glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
  //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error doComp: " << err << std::endl;
  }
}

void GLApplication::update_StorageBuffer(std::vector<float> input_buffer) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_weights.handle);
    GLfloat* ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, input_buffer.data(), m_layerCount * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void GLApplication::update_Texture() {

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D_ARRAY, srcTex.handle);


  // REMINDER: pbo_container.size() = m_layerCount
  for(unsigned int j = 0; j < m_layerCount; j++){
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_container[0].handle);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, j, m_res.x, m_res.y, 1, GL_RED, GL_FLOAT, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
  }

  // glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  // glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, 0);
  // std::vector<float> tmp;
  // tmp.reserve(m_num_pix);
  // GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
  // memcpy(tmp.data(), ptr, m_num_pix * sizeof(float));
  // glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  // glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

  // for(int i = 0; i < 4; i++){
  //   std::cout << tmp[i] << ", ";
  // }
  // std::cout << " " << std::endl;

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error updateTex: " << err << std::endl;
  }
}

void GLApplication::update_PixelBuffer(PBO pbo, std::vector<float> input_buffer) {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.handle);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, m_num_pix * sizeof(float), 0, GL_STREAM_DRAW);
  GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, input_buffer.data(), m_num_pix * sizeof(float));
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error updatePBO: " << err << std::endl;
  }
}

std::string GLApplication::readFile(std::string const& file) {
  std::ifstream in(file.c_str());
  if (in) {
    std::string str((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return str;
  }
  throw (errno);
}

GLuint GLApplication::loadShaders(std::string const& cs) {
  std::string c = readFile(cs);
  return createProgram(c);
}

GLuint GLApplication::loadShader(GLenum type, std::string const& s) {
  GLuint id = glCreateShader(type);
  const char* source = s.c_str();
  glShaderSource(id, 1, &source, NULL);
  glCompileShader(id);

  GLint successful;
  glGetShaderiv(id, GL_COMPILE_STATUS, &successful);
  if (!successful) {
    int length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    std::string info(length, ' ');

    glGetShaderInfoLog(id, length, &length, &info[0]);
    try{
      throw std::logic_error(info);
    }
    catch ( std::exception &e ){
      std::cout << "Caught: " <<e.what( ) << std::endl;
    }
  }
  return id;
}

GLuint GLApplication::createProgram(std::string const& comp) {
  GLuint id = glCreateProgram();

  GLuint compHandle = loadShader(GL_COMPUTE_SHADER, comp);
  glAttachShader(id, compHandle);
  glLinkProgram(id);
  GLint successful;

  glGetProgramiv(id, GL_LINK_STATUS, &successful);
  if (!successful) {
    int length;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
    std::string info(length, ' ');

    glGetProgramInfoLog(id, length, &length, &info[0]);
    try{
      throw std::logic_error(info);
    }
    catch ( std::exception &e ){
      std::cout << e.what( ) << std::endl;
    }
  }
  // schedule for deletion
  glDeleteShader(compHandle);
  return id;
}

void GLApplication::cleanupSSBOs() {

  GLfloat* ptr;
  std::vector<float> dump;
  dump.reserve(m_layerCount * sizeof(float));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_weights.handle);
  ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, dump.data(), m_layerCount * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

GLApplication::~GLApplication() {
  glDeleteBuffers(1, &p_pbo.handle);
  glDeleteBuffers(1, &ssbo_weights.handle);
  for(unsigned int i = 0; i < tex_container.size(); i++){
    glDeleteBuffers(1, &tex_container[i].handle);
  }
  for(unsigned int i = 0; i < pbo_container.size(); i++){
    glDeleteBuffers(1, &pbo_container[i].handle);
  }
};
