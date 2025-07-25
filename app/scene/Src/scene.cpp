#include "scene.h"

namespace Scene
{
    Scene::Scene(DisplayController &_display) : display{_display} {}

    void Scene::Init() {};
    void Scene::Focus(Direction dir) {};
}