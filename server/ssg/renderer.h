#pragma once

#include <crow/mustache.h>
#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

#include "parser.h"

namespace ssg {

struct TagInfo {
    std::string tagName;
    std::vector<Attribute> attrs;
    std::ostringstream buff;
};

class Renderer : public Parser {
  public:
    Renderer();
    Renderer(const std::string_view &src);
    ~Renderer() = default;

    void Load(const std::string_view &src) override;

    void AddComponent(const std::string &name,
                      const std::shared_ptr<crow::mustache::template_t> &temp);

    std::string Result();

  private:
    void OnText(const std::string_view &text) override;

    void OnElementStart(const std::string_view &tagName,
                        const std::vector<Attribute> &attributes,
                        const bool &selfClosing) override;

    void OnElementEnd(const std::string_view &tagName) override;

    void OnError(const ParseError &error) override;

    std::unordered_map<std::string, std::shared_ptr<crow::mustache::template_t>>
        components_;

    std::vector<TagInfo> stack_;
};

} // namespace ssg
