#include "crow_all.h"
#include <iostream>
#include <string>


auto main(int argc, char *argv[]) -> int {
  std::cout << "Hello, World!" << std::endl;

  crow::SimpleApp app;

  crow::mustache::set_global_base("templates");

  CROW_ROUTE(app, "/")([]() {
    auto page = crow::mustache::load_text("index.html");
    return page;
  });

  CROW_ROUTE(app, "/cours-perf")([]() {
    auto page = crow::mustache::load_text("cours-perf.html");
    return page;
  });

  app.port(18080).multithreaded().run();

  return 0;
}
