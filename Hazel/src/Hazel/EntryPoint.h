#ifndef HAZEL_ENTRYPOINT_H
#define HAZEL_ENTRYPOINT_H

#include <iostream>

int main(int argc, char **argv) {
  Hazel::Log::Init();
  auto app = Hazel::CreateApplication();
  app->Run();
  delete app;
}

#endif