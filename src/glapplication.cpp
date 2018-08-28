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
  m_NUM_PIX = 1920*1080*3;
  m_WORK_GROUP_SIZE = 10;
  m_res = glm::ivec2(1920*3, 1080);

  initialCheck();
  initializePrograms();
  initializeTextures();
  initPixelBuffer();
}

std::vector<float> GLApplication::render(std::vector<std::vector<float>*> pictures) {

  std::vector<float> test;
  unsigned int counter = 0;
  for(int i = 0; i < m_NUM_PIX; i++){
    test.push_back(1.0f);
  }

  while (!g_win.shouldClose()) {

    for(unsigned int i = 0; i < m_layerCount; i++){
      update_PixelBuffer(pbo_container[i], test);
    }

    update_Texture(test);
    do_Computation(g_tex_program);
    read_Texture();

    g_win.update();

    if(counter >= pictures.size()){
      return result_container;
      g_win.pause();
    }
    counter += 1;
  }
}

void GLApplication::initializePrograms() {
  const std::string g_file_sum_shader("./../shader/sum.comp");
  const std::string g_file_weighted_sum_shader("./../shader/weighted_sum.comp");
  const std::string g_file_tex_shader("./../shader/tex.comp");

  try {
    // g_sum_program = loadShaders(g_file_sum_shader);
    // g_weighted_sum_program = loadShaders(g_file_weighted_sum_shader);
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

  for(int i = 0; i < m_NUM_PIX; i++){
    result_container.push_back(0.0f);
  }

  glGenBuffers(1, &ssbo_result.handle);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_result.handle);
  glBufferData(GL_SHADER_STORAGE_BUFFER, m_NUM_PIX * sizeof(float), result_container.data(), GL_DYNAMIC_DRAW);

  for(unsigned int i = 0; i < 1; i++){
    SSBO ssbo;
    glGenBuffers(1, &ssbo.handle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i+1, ssbo.handle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (m_NUM_PIX + 1) * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    ssbo_container.push_back(ssbo);

    GLenum err;
    if((err = glGetError()) != GL_NO_ERROR){
      std::cout << "OpenGL error initSBO: " << err << std::endl;
    }
  }

}

void GLApplication::initializeTextures() {
  uniform_names.push_back("destTex");
  uniform_names.push_back("srcTex");

  for(unsigned int i = 0; i < uniform_names.size(); i++){

    Tex_obj tex;
    glActiveTexture(GL_TEXTURE0+i);

    if(i == 0) {
      glGenTextures(1, &tex.handle);
      glBindTexture(GL_TEXTURE_2D, tex.handle);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, m_res.x, m_res.y, 0, GL_RED, GL_FLOAT, NULL);
    }
    else {
      glGenTextures(1, &tex.handle);
      glBindTexture(GL_TEXTURE_2D_ARRAY, tex.handle);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      // glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, m_res.x, m_res.y, m_layerCount, 0, GL_RED, GL_FLOAT, NULL);
      glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_R32F, m_res.x, m_res.y, m_layerCount);
    }

    glBindImageTexture(i, tex.handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);
    tex_container.push_back(tex);

    GLenum err;
    if((err = glGetError()) != GL_NO_ERROR){
      std::cout << "OpenGL error initTex: " << err << std::endl;
    }
  }
}

void GLApplication::initPixelBuffer() {
  glGenBuffers(1, &p_pbo.handle);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  glBufferData(GL_PIXEL_PACK_BUFFER, m_NUM_PIX * sizeof(float), 0, GL_DYNAMIC_DRAW);

  for(unsigned int i = 0; i < m_layerCount; i++){
    PBO pbo;
    glGenBuffers(1, &pbo.handle);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.handle);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, m_NUM_PIX * sizeof(float), 0, GL_DYNAMIC_DRAW);
    pbo_container.push_back(pbo);
  }

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error initPBO: " << err << std::endl;
  }
}

void GLApplication::initializeFramBuffer() {

  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &color_tb.handle);
  glBindTexture(GL_TEXTURE_2D, color_tb.handle);
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA32F, 1920, 1080, 0, GL_RGBA, GL_FLOAT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_tb.handle, 0);

  glGenRenderbuffers(1, &color_rb.handle);
  glBindRenderbuffer(GL_RENDERBUFFER, color_rb.handle);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1920, 1080);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, color_rb.handle);

  glGenFramebuffers(1, &fbo.handle);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo.handle);
  GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
  glDrawBuffers(1, draw_buffers);

  GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER);
  if (status != GL_FRAMEBUFFER_COMPLETE){
    std::cout << "ERROR Framebuffer not complete" << '\n';
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
  glBindTexture(GL_TEXTURE_2D, tex_container[0].handle);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, 0);

  std::vector<float> tmp;
  tmp.reserve(m_NUM_PIX);
  GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_WRITE);
  memcpy(tmp.data(), ptr, m_NUM_PIX * sizeof(float));
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

void GLApplication::read_Computation() {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_result.handle);
  GLfloat *ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  memcpy(result_container.data(), ptr, m_NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  std::cout << "container: " << result_container[0] << std::endl;
}

void GLApplication::do_Computation(GLuint program) {
  glUseProgram(program);
  glDispatchCompute(1920*3/ m_WORK_GROUP_SIZE, 1080/m_WORK_GROUP_SIZE, 1);
  glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
  //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR){
    std::cout << "OpenGL error doComp: " << err << std::endl;
  }
}

void GLApplication::update_StorageBuffer(Pstruct pstruct) {
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    Pstruct* ptr = (Pstruct *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    //memcpy(ptr, &pstruct, m_NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

void GLApplication::update_StorageBuffer(std::vector<float> input_buffer) {
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    GLfloat* ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, input_buffer.data(), m_NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

void GLApplication::update_Texture(std::vector<float> input_buffer) {
  for(unsigned int i = 1; i < tex_container.size(); i++){
    glActiveTexture(GL_TEXTURE0+i);
    glBindTexture(GL_TEXTURE_2D_ARRAY, tex_container[i].handle);

    // pbo_container.size() = m_layerCount
    for(unsigned int j = 0; j < m_layerCount; j++){
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_container[j].handle);
      glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, j, m_res.x, m_res.y, 1, GL_RED, GL_FLOAT, 0);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    GLenum err;
    if((err = glGetError()) != GL_NO_ERROR){
      std::cout << "OpenGL error updateTex: " << err << std::endl;
    }
  }
}

void GLApplication::update_PixelBuffer(PBO pbo, std::vector<float> input_buffer) {
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo.handle);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, m_NUM_PIX * sizeof(float), 0, GL_STREAM_DRAW);
  GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, input_buffer.data(), m_NUM_PIX * sizeof(float));
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
  dump.reserve(m_NUM_PIX * sizeof(float));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_result.handle);
  ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, dump.data(), m_NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, dump.data(), m_NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

GLApplication::~GLApplication() {
  glDeleteBuffers(1, &ssbo_result.handle);
  glDeleteBuffers(1, &p_pbo.handle);
  glDeleteBuffers(1, &fbo.handle);
  glDeleteBuffers(1, &color_rb.handle);
  glDeleteBuffers(1, &color_tb.handle);
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glDeleteBuffers(1, &ssbo_container[i].handle);
  }
  for(unsigned int i = 0; i < tex_container.size(); i++){
    glDeleteBuffers(1, &tex_container[i].handle);
  }
  for(unsigned int i = 0; i < pbo_container.size(); i++){
    glDeleteBuffers(1, &pbo_container[i].handle);
  }

};
