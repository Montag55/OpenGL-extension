#include "./../include/glapplication.hpp"

GLApplication::GLApplication(){
  g_background_color = glm::vec3(0.08f, 0.08f, 0.08f);
  g_window_res = glm::ivec2(800, 600);
  g_reload_shader_error = false;
  g_win.resize(g_window_res);
  g_sum_program = 0;
  g_weighted_sum_program = 0;
  g_tex_program = 0;
  NUM_PIX = 1920*1080*4;
  WORK_GROUP_SIZE = 1000;
  m_res = glm::ivec2(1920*1080);

  //initialCheck();
  initializePrograms();
  initializeTextures();
}

std::vector<float> GLApplication::render(std::vector<std::vector<float>*> pictures){

  std::vector<float> test;
  std::vector<float> test2;
  unsigned int counter = 0;
  for(int i = 0; i < NUM_PIX; i++){
    test.push_back(7.7f);
    test2.push_back(1.5f);
  }

  while (!g_win.shouldClose()) {

    if(counter%2 == 0)
      update_Texture(test);
    else
      update_Texture(test2);

    do_Computation(g_tex_program);
    read_Texture();

    g_win.update();

    for(int i = 0; i < 4; i++){
      std::cout << result_container[i] << ", ";
    }
    std::cout << "" << std::endl;

    if(counter >= pictures.size()){
      return result_container;
      g_win.pause();
    }
    counter += 1;
  }
}

void GLApplication::initializePrograms(){
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

  for(int i = 0; i < NUM_PIX; i++){
    result_container.push_back(0.0f);
  }

  glGenBuffers(1, &ssbo_result.handle);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_result.handle);
  glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_PIX * sizeof(float), result_container.data(), GL_DYNAMIC_DRAW);

  for(unsigned int i = 0; i < 1; i++){
    SSBO ssbo;
    glGenBuffers(1, &ssbo.handle);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i+1, ssbo.handle);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (NUM_PIX + 1) * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    ssbo_container.push_back(ssbo);
  }

}

void GLApplication::initializeTextures(){
  std::vector<const char*> uniform_names;
  uniform_names.push_back("destTex");
  uniform_names.push_back("srcTex");

  for(unsigned int i = 0; i < uniform_names.size(); i++){
    Tex_obj tex;
    glActiveTexture(GL_TEXTURE0+i);
    glGenTextures(1, &tex.handle);
    glBindTexture(GL_TEXTURE_1D, tex.handle);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F, m_res.x * m_res.y, 0, GL_RGB, GL_FLOAT, NULL);
    glBindImageTexture(i, tex.handle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
    glUniform1i(glGetUniformLocation(g_tex_program, uniform_names[i]), i);
    tex_container.push_back(tex);
  }
}

void GLApplication::initPixelBuffer(){
  glGenBuffers(1, &u_pbo.handle);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u_pbo.handle);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, NUM_PIX * sizeof(float), 0, GL_DYNAMIC_DRAW);
  glReadPixels(0, 0, 1920, 1080, GL_RGB, GL_FLOAT, 0);


  glGenBuffers(1, &p_pbo.handle);
  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  glBufferData(GL_PIXEL_PACK_BUFFER, NUM_PIX * sizeof(float), 0, GL_DYNAMIC_DRAW);
  glReadPixels(0, 0, 1920, 1080, GL_RGB, GL_FLOAT, 0);
}

void GLApplication::initializeFramBuffer(){

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
}

void GLApplication::read_Texture(){
  glActiveTexture(GL_TEXTURE0);
  float *tmp = new float[1920*1080*4];
  glBindTexture(GL_TEXTURE_1D, tex_container[0].handle);
  glGetTexImage(GL_TEXTURE_1D, 0, GL_RGBA, GL_FLOAT, tmp);

  result_container.reserve(NUM_PIX);
  memcpy(result_container.data(), tmp, NUM_PIX * sizeof(float));
}

void GLApplication::read_Computation() {
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_result.handle);
  GLfloat *ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
  memcpy(result_container.data(), ptr, NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  std::cout << "container: " << result_container[0] << std::endl;
}

void GLApplication::do_Computation(GLuint program) {
  glUseProgram(program);
  glDispatchCompute(NUM_PIX / WORK_GROUP_SIZE, 1, 1);
  //glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT);
}

void GLApplication::update_StorageBuffer(Pstruct pstruct){
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    Pstruct* ptr = (Pstruct *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    //memcpy(ptr, &pstruct, NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

void GLApplication::update_StorageBuffer(std::vector<float> input_buffer){
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    GLfloat* ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, input_buffer.data(), NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

void GLApplication::update_Texture(std::vector<float> input_buffer){
  for(unsigned int i = 0; i < tex_container.size(); i++){
    glActiveTexture(GL_TEXTURE0+i);
    glBindTexture(GL_TEXTURE_1D, tex_container[i].handle);
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, m_res.x * m_res.y, GL_RGBA, GL_FLOAT, input_buffer.data());
  }
}

void GLApplication::update_PixelBuffer(){
  float *tmp = new float[1920*1080*4];

  glBindBuffer(GL_PIXEL_PACK_BUFFER, p_pbo.handle);
  GLfloat *ptr = (GLfloat *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
  memcpy(tmp, ptr, NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
  glReadPixels(0, 0, 1920, 1080, GL_RGB, GL_FLOAT, 0);

  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, u_pbo.handle);
  ptr = (GLfloat *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, tmp, NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
  glReadPixels(0, 0, 1920, 1080, GL_RGB, GL_FLOAT, 0);
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

void GLApplication::cleanupSSBOs(){

  GLfloat* ptr;
  std::vector<float> dump;
  dump.reserve(NUM_PIX * sizeof(float));

  glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_result.handle);
  ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
  memcpy(ptr, dump.data(), NUM_PIX * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_container[i].handle);
    ptr = (GLfloat *) glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
    memcpy(ptr, dump.data(), NUM_PIX * sizeof(float));
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  }
}

GLApplication::~GLApplication(){
  glDeleteBuffers(1, &ssbo_result.handle);
  glDeleteBuffers(1, &u_pbo.handle);
  glDeleteBuffers(1, &p_pbo.handle);
  for(unsigned int i = 0; i < ssbo_container.size(); i++){
    glDeleteBuffers(1, &ssbo_container[i].handle);
  }
  for(unsigned int i = 0; i < tex_container.size(); i++){
    glDeleteBuffers(1, &tex_container[i].handle);
  }
};
