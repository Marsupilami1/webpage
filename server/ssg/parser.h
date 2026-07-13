#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ssg {

using size_t = std::string_view::size_type;

struct ParseError {
    std::string message;
    size_t position;
};

struct Attribute {
    std::string name, value;
};

class Parser {
  public:
    Parser();
    Parser(const std::string_view &src);
    virtual ~Parser() = default;

    void Run();

    virtual void Load(const std::string_view &src);

  private:
    size_t NameEnd(const size_t &pos);
    size_t Find(const char &target, const size_t &pos);

    void ParseChildren(std::string_view enclosingTag);

    void ParseElement();

    std::vector<Attribute> ParseAttributes();

    virtual void OnText(const std::string_view &text) = 0;

    virtual void OnElementStart(const std::string_view &tagName,
                                const std::vector<Attribute> &attributes,
                                const bool &selfClosing) = 0;

    virtual void OnElementEnd(const std::string_view &tagName) = 0;

    virtual void OnError(const ParseError &error) = 0;

    std::string_view src_;

  protected:
    size_t pos_;
};

} // namespace ssg
