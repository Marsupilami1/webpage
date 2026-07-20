#include "renderer.h"
#include <crow/logging.h>
#include <crow/mustache.h>

namespace ssg {

Renderer::Renderer() : Parser(), components_(), stack_() {}

Renderer::Renderer(const std::string_view &src)
    : Parser(src), components_(), stack_() {
  stack_.push_back({"", {}, {}});
}

void Renderer::Load(const std::string_view &src) {
  stack_.clear();
  stack_.push_back({"", {}, {}});
  Parser::Load(src);
}

void Renderer::ClearComponents() {
  components_.clear();   
}

void Renderer::AddComponent(
    const std::string &name,
    const std::shared_ptr<crow::mustache::template_t> &temp) {
  components_[name] = std::move(temp);
}

std::string Renderer::Result() {
  if (stack_.size() != 1) {
    for (const auto &elt : stack_) {
      std::cerr << "-" << elt.tagName << std::endl;
    }
    OnError({"Stack is not 1-sized", std::string_view::npos});
  }
  return stack_[0].buff.str();
}

void Renderer::OnText(const std::string_view &text) {
  stack_.back().buff << text;
}

void Renderer::OnElementStart(const std::string_view &tagName,
                              const std::vector<Attribute> &attributes,
                              const bool &selfClosing) {
  // OnElementEnd is called even for self-closing tags
  stack_.push_back({std::string(tagName), attributes, {}});
}

void Renderer::OnElementEnd(const std::string_view &tagName) {
  TagInfo info = std::move(stack_.back());
  stack_.pop_back();

  if (!components_.contains(info.tagName)) {
    OnError({"Component not found " + info.tagName, pos_});
    return;
  }
  auto temp = components_.at(info.tagName);

  crow::mustache::context ctx;
  ctx["slot"] = info.buff.str();
  for (const auto &[k, v] : info.attrs) {
    ctx[k] = v;
  }

  stack_.back().buff << temp->render_string(ctx);
}

void Renderer::OnError(const ParseError &err) {
  std::ostringstream os;
  os << "(" << err.position << ") " << err.message;
  throw std::runtime_error(os.str());
}

} // namespace ssg
