#include "./../include/glapplication.hpp"
#include <chrono>

int main(int argc, char* argv[]) {

  std::vector<float> weights;
  std::vector<std::vector<float>*> pictures;
  for(unsigned int i = 0; i < 6; i++){
    std::vector<float>* tmp = new std::vector<float>;
    pictures.push_back(tmp);
  }

  GLApplication app;
  auto start = std::chrono::system_clock::now();
  std::vector<float> first_render = app.render(pictures, weights);
  auto mid = std::chrono::system_clock::now();
  app.cleanupSSBOs();
  auto end = std::chrono::system_clock::now();

  auto elapsed1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
  auto elapsed2 = std::chrono::duration_cast<std::chrono::milliseconds>(mid - start);
  auto elapsed3 = std::chrono::duration_cast<std::chrono::milliseconds>(end - mid);
  std::cout << "Total time: \t" << elapsed1.count() << " ms" << std::endl;
  std::cout << "Render time: \t" << elapsed2.count() << " ms" << std::endl;
  std::cout << "Cleanup time: \t" << elapsed3.count() << " ms" << std::endl;

  return 0;
}
