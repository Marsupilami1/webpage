#include "parser.h"

#include <cctype>

using namespace ssg;

static constexpr std::string_view kNameChars =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
static constexpr std::string_view kWhitespace = " \t\r\n\f\v";

static size_t skip(const std::string_view &src, const size_t &pos,
                   const std::string_view &chars) {
    return src.find_first_not_of(chars, pos);
}

static size_t skipWhitespace(const std::string_view &src, const size_t &pos) {
    return skip(src, pos, kWhitespace);
}

Parser::Parser() : src_(), pos_(0) {}

Parser::Parser(const std::string_view &src) : src_(src), pos_(0) {}

void Parser::Run() {
    pos_ = 0;
    ParseChildren("");
}

void Parser::Load(const std::string_view &src) {
    src_ = src;
    pos_ = 0;
}

size_t Parser::NameEnd(const size_t &pos) {
    return src_.find_first_not_of(kNameChars, pos);
}

size_t Parser::Find(const char &target, const size_t &pos) {
    return src_.find(target, pos);
}

void Parser::ParseChildren(std::string_view enclosingTag) {
    size_t textStart = pos_;

    auto flushText = [&](size_t end) {
        if (end > textStart) {
            OnText(src_.substr(textStart, end - textStart));
        }
    };

    while (pos_ < src_.size()) {
        // Find open tag
        pos_ = Find('<', pos_);
        if (pos_ == std::string_view::npos) {
            break;
        }

        // Closing tag
        if (!enclosingTag.empty() && pos_ + 1 < src_.size() &&
            src_[pos_ + 1] == '/') {
            size_t nameStart = pos_ + 2;
            size_t i = NameEnd(nameStart);
            std::string_view closeName = src_.substr(nameStart, i - nameStart);

            if (closeName == enclosingTag) {
                flushText(pos_);
                pos_ = Find('>', i) + 1;
                return;
            }

            ++pos_;
            continue;
        }

        // Open tag
        if (pos_ + 1 < src_.size() &&
            std::isupper(static_cast<unsigned char>(src_[pos_ + 1]))) {
            flushText(pos_);
            ParseElement();
            textStart = pos_;
            continue;
        }

        ++pos_;
    }

    // End of file
    flushText(src_.size());

    if (!enclosingTag.empty()) {
        OnError({"unclosed component tag <" + std::string(enclosingTag) + ">",
                 pos_});
    }
}

void Parser::ParseElement() {
    size_t nameStart = pos_ + 1;
    size_t i = NameEnd(nameStart);
    std::string_view tagName = src_.substr(nameStart, i - nameStart);

    pos_ = i;
    auto attrs = ParseAttributes();

    if (pos_ >= src_.size()) {
        OnError({"unexpected end of file", pos_});
    }

    bool selfClosing = false;
    if (src_[pos_] == '/') {
        selfClosing = true;
        ++pos_;
    }
    if (src_[pos_] == '>') {
        ++pos_;
    } else {
        OnError(
            {"expected '>' closing tag <" + std::string(tagName) + ">", pos_});
    }

    OnElementStart(tagName, attrs, selfClosing);

    if (!selfClosing) {
        ParseChildren(tagName);
    }

    OnElementEnd(tagName);
}

std::vector<Attribute> Parser::ParseAttributes() {
    std::vector<Attribute> attrs;

    while (true) {
        pos_ = skipWhitespace(src_, pos_);
        if (pos_ >= src_.size() || src_[pos_] == '/' || src_[pos_] == '>')
            break;

        size_t nameStart = pos_;
        pos_ = NameEnd(pos_);
        std::string name(src_.substr(nameStart, pos_ - nameStart));

        if (name.empty()) {
            OnError({"malformed attribute", pos_});
            ++pos_;
            continue;
        }

        std::string value;
        size_t afterName = skipWhitespace(src_, pos_);

        if (afterName < src_.size() && src_[afterName] == '=') {
            pos_ = skipWhitespace(src_, afterName + 1);

            if (pos_ < src_.size() &&
                (src_[pos_] == '"' || src_[pos_] == '\'')) {
                char quote = src_[pos_];
                size_t valStart = pos_ + 1;
                size_t valEnd = Find(quote, valStart);
                value = std::string(src_.substr(valStart, valEnd - valStart));
                pos_ = valEnd + 1;
            } else {
                size_t valStart = pos_ + 1;
                size_t valEnd = NameEnd(nameStart);
                value = src_.substr(valStart, valEnd - valStart);
                pos_ = valEnd + 1;
            }
        } else {
            // No '=' sign, boolean attribute
            pos_ = afterName;
        }

        attrs.push_back({std::move(name), std::move(value)});
    }

    return attrs;
}
