### Compile:
g++ main.cpp window.cpp glapplication.cpp -o test -lglut -lGL -lGLU -lGLEW -lglfw -std=c++11


### Dependencies:
* libgl1-mesa-dev
* libglew-dev
* libglfw3
* libglfw3-dev
* libglm-dev
* freeglut3-dev

### HowToUse
* Initialize:                       GLApplication app;
* Call render with picture arary:   std::vector<float> render = app.render(pictures);
* Cleanup SSBO GPU memory:               app.cleanup();
Cleanup is  necessary for I do not delete the context. I plainly pause it untill the context
qued task.
