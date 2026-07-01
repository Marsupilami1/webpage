#include "crow_all.h"
#include <atomic>
#include <string>
#include <thread>

crow::json::wvalue LoadArticles() {
  static constexpr std::array<const char *, 6> kHeaderFields = {
      "date", "title", "short_authors", "authors", "conference", "url"};

  std::vector<crow::json::wvalue> articles;

  for (const auto &entry : std::filesystem::directory_iterator("assets/")) {
    std::ifstream file(entry.path());
    if (!file)
      continue;

    crow::json::wvalue article;
    std::string line;
    for (const char *field : kHeaderFields) {
      std::getline(file, line);
      article[field] = line;
    }

    std::ostringstream abstract;
    while (std::getline(file, line)) {
      if (abstract.tellp() > 0)
        abstract << ' ';
      abstract << line;
    }
    article["abstract"] = abstract.str();

    articles.push_back(std::move(article));
  }

  // sort by date
  std::sort(articles.begin(), articles.end(), [](const auto &a, const auto &b) {
    return a["date"].dump() > b["date"].dump();
  });

  return {{"articles", articles}};
}

std::string LoadApiKey() {
  const char *env_key = std::getenv("API_KEY");
  if (env_key != nullptr && std::strlen(env_key) > 0) {
    return std::string(env_key);
  }

  const char *env_path = std::getenv("API_KEY_FILE");
  std::string key_file =
      env_path != nullptr ? std::string(env_path) : "api-key.txt";

  std::ifstream file(key_file);
  if (!file.is_open()) {
    CROW_LOG_ERROR << "Failed to open API key file: " << key_file;
    CROW_LOG_ERROR << "Set API_KEY environment variable or "
                      "API_KEY_FILE to specify a file path";
    return "";
  }

  std::string key;
  std::getline(file, key);
  return key;
}

bool ValidAuthorization(const crow::request &req, const std::string &api_key) {
  std::string auth_header = req.get_header_value("Authorization");
  std::string expected_token = "Bearer " + api_key;
  return auth_header == expected_token;
}

void BuildStaticPages() {
  CROW_LOG_INFO << "Building static pages";
  static const std::filesystem::path build_path = "dist/";
  static const std::filesystem::path build_tmp_path = "dist_tmp/";
  static const std::filesystem::path build_old_path = "dist_old/";

  std::error_code ec;

  std::filesystem::remove_all(build_tmp_path, ec);
  std::filesystem::create_directories(build_tmp_path, ec);
  if (ec) {
    throw std::runtime_error("Failed to create tmp dir: " + ec.message());
  }

  CROW_LOG_INFO << "Compiling index";
  {
    auto page = crow::mustache::load("index.mustache");
    crow::mustache::context articles = LoadArticles();
    std::ofstream f(build_tmp_path / "index.html");
    f << page.render_string(articles);
  }

  CROW_LOG_INFO << "Compiling cours-perf";
  {
    auto page = crow::mustache::load("cours-perf.mustache");
    std::ofstream f(build_tmp_path / "cours-perf.html");
    f << page.render_string();
  }

  CROW_LOG_INFO << "Swapping build directories";
  std::filesystem::remove_all(build_old_path, ec);
  if (std::filesystem::exists(build_path)) {
    std::filesystem::rename(build_path, build_old_path, ec);
    if (ec)
      throw std::runtime_error("Failed to backup old build: " + ec.message());
  }

  std::filesystem::rename(build_tmp_path, build_path, ec);
  if (ec) {
    // Rollback
    std::filesystem::rename(build_old_path, build_path);
    throw std::runtime_error("Failed to publish build: " + ec.message());
  }

  std::filesystem::remove_all(build_old_path, ec);

  CROW_LOG_INFO << "Build complete";
}

auto main(int argc, char *argv[]) -> int {
  std::string api_key = LoadApiKey();
  if (api_key.empty()) {
    return 1;
  }

  std::atomic<bool> building{false};

  crow::SimpleApp app;

  CROW_LOG_INFO << "Starting Server";

  crow::mustache::set_global_base("templates");

  CROW_ROUTE(app, "/")([](crow::response &res) {
    res.set_static_file_info("dist/index.html");
    res.end();
  });

  CROW_ROUTE(app, "/cours-perf")([](crow::response &res) {
    res.set_static_file_info("dist/cours-perf.html");
    res.end();
  });

  CROW_ROUTE(app, "/webhook/build-site")
      .methods(crow::HTTPMethod::POST)(
          [&api_key, &building](const crow::request &req) {
            // I am the only one able to build the site
            if (!ValidAuthorization(req, api_key))
              return crow::response(crow::status::UNAUTHORIZED, "text/plain",
                                    "Access denied\n");

            // If we are already building the site, abort
            bool expected = false;
            if (!building.compare_exchange_strong(expected, true))
              return crow::response(crow::status::CONFLICT, "text/plain",
                                    "Build already in progress\n");

            std::thread([&building]() {
              try {
                BuildStaticPages();
              } catch (const std::exception &e) {
                CROW_LOG_ERROR << "Build failed: " << e.what();
              }
              building = false;
            }).detach();

            return crow::response(crow::status::ACCEPTED);
          });

  app.port(18080).multithreaded().run();

  return 0;
}
