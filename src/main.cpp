#include "./../include/glapplication.hpp"

int main(int argc, char* argv[]) {

    std::vector<std::vector<float>*> pictures;
    for(unsigned int i = 0; i < 6; i++){
      std::vector<float>* tmp = new std::vector<float>;
      pictures.push_back(tmp);
    }

    GLApplication app;
    std::vector<float> first_render = app.render(pictures);
    //std::vector<float> a1_render = app.render(pictures);
    //app.cleanup();


    return 0;
}
