#include <nxdev/manifest/yaml.hpp>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <stdexcept>

namespace nxdev::manifest {

static const std::string EMPTY_STRING = "";
static const YamlNode::SequenceType EMPTY_SEQUENCE = {};
static const YamlNode::MappingType EMPTY_MAPPING = {};

const std::string& YamlNode::as_string() const {
    if (type_ == YamlNodeType::Scalar) {
        return std::get<std::string>(value_);
    }
    return EMPTY_STRING;
}

std::optional<int64_t> YamlNode::as_int() const noexcept {
    if (type_ != YamlNodeType::Scalar) return std::nullopt;
    const auto& s = std::get<std::string>(value_);
    if (s.empty()) return std::nullopt;

    try {
        size_t idx = 0;
        int base = 10;
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
            base = 16;
        } else if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0) {
            base = 2;
        }
        int64_t v = std::stoll(s, &idx, base);
        if (idx == s.size()) return v;
    } catch (...) {}
    return std::nullopt;
}

std::optional<uint64_t> YamlNode::as_uint() const noexcept {
    if (type_ != YamlNodeType::Scalar) return std::nullopt;
    const auto& s = std::get<std::string>(value_);
    if (s.empty()) return std::nullopt;

    try {
        size_t idx = 0;
        int base = 10;
        if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) {
            base = 16;
        } else if (s.rfind("0b", 0) == 0 || s.rfind("0B", 0) == 0) {
            base = 2;
        }
        uint64_t v = std::stoull(s, &idx, base);
        if (idx == s.size()) return v;
    } catch (...) {}
    return std::nullopt;
}

std::optional<bool> YamlNode::as_bool() const noexcept {
    if (type_ != YamlNodeType::Scalar) return std::nullopt;
    std::string s = std::get<std::string>(value_);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (s == "true" || s == "yes" || s == "on" || s == "1") return true;
    if (s == "false" || s == "no" || s == "off" || s == "0") return false;
    return std::nullopt;
}

const YamlNode::SequenceType& YamlNode::as_sequence() const {
    if (type_ == YamlNodeType::Sequence) {
        return std::get<SequenceType>(value_);
    }
    return EMPTY_SEQUENCE;
}

const YamlNode::MappingType& YamlNode::as_mapping() const {
    if (type_ == YamlNodeType::Mapping) {
        return std::get<MappingType>(value_);
    }
    return EMPTY_MAPPING;
}

bool YamlNode::has_key(const std::string& key) const noexcept {
    if (type_ != YamlNodeType::Mapping) return false;
    const auto& map = std::get<MappingType>(value_);
    for (const auto& [k, _] : map) {
        if (k == key) return true;
    }
    return false;
}

const YamlNode* YamlNode::get(const std::string& key) const noexcept {
    if (type_ != YamlNodeType::Mapping) return nullptr;
    const auto& map = std::get<MappingType>(value_);
    for (const auto& [k, node] : map) {
        if (k == key) return &node;
    }
    return nullptr;
}

const YamlNode* YamlNode::get(size_t index) const noexcept {
    if (type_ != YamlNodeType::Sequence) return nullptr;
    const auto& seq = std::get<SequenceType>(value_);
    if (index < seq.size()) return &seq[index];
    return nullptr;
}

void YamlNode::set_mapping_entry(const std::string& key, YamlNode node) {
    if (type_ != YamlNodeType::Mapping) {
        type_ = YamlNodeType::Mapping;
        value_ = MappingType{};
    }
    auto& map = std::get<MappingType>(value_);
    for (auto& [k, v] : map) {
        if (k == key) {
            v = std::move(node);
            return;
        }
    }
    map.emplace_back(key, std::move(node));
}

void YamlNode::append_sequence_entry(YamlNode node) {
    if (type_ != YamlNodeType::Sequence) {
        type_ = YamlNodeType::Sequence;
        value_ = SequenceType{};
    }
    auto& seq = std::get<SequenceType>(value_);
    seq.push_back(std::move(node));
}

// -----------------------------------------------------------------------------
// YAML Lexer & Parser Implementation
// -----------------------------------------------------------------------------

namespace {

enum class TokenType {
    Eof,
    Newline,
    Indent,
    Key,
    Scalar,
    Dash,
    LBracket,
    RBracket,
    LBrace,
    RBrace,
    Comma
};

struct Token {
    TokenType type{TokenType::Eof};
    std::string value;
    SourceLocation location{1, 1};
    size_t indent{0};
};

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input) {}

    Token next_token() {
        if (!buffered_tokens_.empty()) {
            Token t = buffered_tokens_.front();
            buffered_tokens_.erase(buffered_tokens_.begin());
            return t;
        }

        while (pos_ < input_.size()) {
            char c = input_[pos_];

            // Handle comments
            if (c == '#') {
                skip_comment();
                continue;
            }

            // Handle newline
            if (c == '\n' || (c == '\r' && pos_ + 1 < input_.size() && input_[pos_ + 1] == '\n')) {
                if (c == '\r') advance();
                SourceLocation loc = get_loc();
                advance();
                bol_ = true;
                return Token{TokenType::Newline, "\n", loc, 0};
            }

            // Handle indentation at beginning of line
            if (bol_) {
                size_t ind = 0;
                SourceLocation loc = get_loc();
                while (pos_ < input_.size() && (input_[pos_] == ' ' || input_[pos_] == '\t')) {
                    ind += (input_[pos_] == '\t' ? 2 : 1);
                    advance();
                }
                bol_ = false;

                // If line is empty or comment, loop continues
                if (pos_ < input_.size() && (input_[pos_] == '\n' || input_[pos_] == '\r' || input_[pos_] == '#')) {
                    continue;
                }

                return Token{TokenType::Indent, "", loc, ind};
            }

            // Whitespace inside line
            if (c == ' ' || c == '\t') {
                advance();
                continue;
            }

            SourceLocation loc = get_loc();

            if (c == '[') { advance(); return Token{TokenType::LBracket, "[", loc, 0}; }
            if (c == ']') { advance(); return Token{TokenType::RBracket, "]", loc, 0}; }
            if (c == '{') { advance(); return Token{TokenType::LBrace, "{", loc, 0}; }
            if (c == '}') { advance(); return Token{TokenType::RBrace, "}", loc, 0}; }
            if (c == ',') { advance(); return Token{TokenType::Comma, ",", loc, 0}; }

            // Dash '-' sequence item
            if (c == '-' && (pos_ + 1 >= input_.size() || input_[pos_ + 1] == ' ' || input_[pos_ + 1] == '\n' || input_[pos_ + 1] == '\r')) {
                advance();
                return Token{TokenType::Dash, "-", loc, 0};
            }

            // Quoted string
            if (c == '"' || c == '\'') {
                return lex_quoted(c);
            }

            // Plain scalar or key
            return lex_plain();
        }

        return Token{TokenType::Eof, "", get_loc(), 0};
    }

private:
    std::string_view input_;
    size_t pos_{0};
    size_t line_{1};
    size_t col_{1};
    bool bol_{true};
    std::vector<Token> buffered_tokens_;

    [[nodiscard]] SourceLocation get_loc() const noexcept {
        return SourceLocation{line_, col_};
    }

    void advance() {
        if (pos_ < input_.size()) {
            if (input_[pos_] == '\n') {
                line_++;
                col_ = 1;
            } else {
                col_++;
            }
            pos_++;
        }
    }

    void skip_comment() {
        while (pos_ < input_.size() && input_[pos_] != '\n' && input_[pos_] != '\r') {
            advance();
        }
    }

    Token lex_quoted(char quote) {
        SourceLocation loc = get_loc();
        advance(); // Skip opening quote
        std::string result;

        while (pos_ < input_.size()) {
            char c = input_[pos_];
            if (c == quote) {
                advance();
                break;
            }
            if (quote == '"' && c == '\\' && pos_ + 1 < input_.size()) {
                advance();
                char esc = input_[pos_];
                if (esc == 'n') result += '\n';
                else if (esc == 't') result += '\t';
                else if (esc == 'r') result += '\r';
                else if (esc == '\\') result += '\\';
                else if (esc == '"') result += '"';
                else result += esc;
                advance();
            } else {
                result += c;
                advance();
            }
        }

        // Check if followed by colon ':'
        size_t look = pos_;
        while (look < input_.size() && (input_[look] == ' ' || input_[look] == '\t')) look++;
        if (look < input_.size() && input_[look] == ':' &&
            (look + 1 >= input_.size() || input_[look + 1] == ' ' || input_[look + 1] == '\n' || input_[look + 1] == '\r')) {
            // It's a mapping key
            pos_ = look + 1;
            col_ += (pos_ - look);
            return Token{TokenType::Key, result, loc, 0};
        }

        return Token{TokenType::Scalar, result, loc, 0};
    }

    Token lex_plain() {
        SourceLocation loc = get_loc();
        std::string result;

        while (pos_ < input_.size()) {
            char c = input_[pos_];

            // Stop on colon if it's a key delimiter
            if (c == ':' && (pos_ + 1 >= input_.size() || input_[pos_ + 1] == ' ' || input_[pos_ + 1] == '\n' || input_[pos_ + 1] == '\r')) {
                std::string key = trim(result);
                advance(); // consume ':'
                return Token{TokenType::Key, key, loc, 0};
            }

            if (c == '\n' || c == '\r' || c == '#' || c == ',' || c == ']' || c == '}') {
                break;
            }

            result += c;
            advance();
        }

        std::string trimmed = trim(result);
        return Token{TokenType::Scalar, trimmed, loc, 0};
    }

    static std::string trim(const std::string& str) {
        auto first = str.find_first_not_of(" \t");
        if (first == std::string::npos) return "";
        auto last = str.find_last_not_of(" \t");
        return str.substr(first, last - first + 1);
    }
};

class ParserEngine {
public:
    explicit ParserEngine(std::string_view input) : lexer_(input) {
        advance();
    }

    std::optional<YamlNode> parse_root(YamlParseError& out_error) {
        skip_newlines();
        if (current_.type == TokenType::Eof) {
            return YamlNode(SourceLocation{1, 1});
        }

        size_t current_indent = 0;
        if (current_.type == TokenType::Indent) {
            current_indent = current_.indent;
            advance();
        }

        auto node = parse_node(current_indent, 0, out_error);
        if (!node) return std::nullopt;

        return node;
    }

private:
    Lexer lexer_;
    Token current_;

    void advance() {
        current_ = lexer_.next_token();
    }

    void skip_newlines() {
        while (current_.type == TokenType::Newline) {
            advance();
        }
    }

    std::optional<YamlNode> parse_node(size_t base_indent, size_t depth, YamlParseError& err) {
        if (depth > YamlParser::MAX_RECURSION_DEPTH) {
            err = {"Maximum YAML nesting recursion depth exceeded", current_.location};
            return std::nullopt;
        }

        if (current_.type == TokenType::LBracket) {
            return parse_flow_sequence(depth, err);
        }
        if (current_.type == TokenType::LBrace) {
            return parse_flow_mapping(depth, err);
        }
        if (current_.type == TokenType::Dash) {
            return parse_block_sequence(base_indent, depth, err);
        }
        if (current_.type == TokenType::Key) {
            return parse_block_mapping(base_indent, depth, err);
        }
        if (current_.type == TokenType::Scalar) {
            SourceLocation loc = current_.location;
            std::string val = current_.value;
            advance();
            return YamlNode(val, loc);
        }

        if (current_.type == TokenType::Eof || current_.type == TokenType::Newline) {
            return YamlNode(current_.location);
        }

        err = {"Unexpected YAML token '" + current_.value + "'", current_.location};
        return std::nullopt;
    }

    std::optional<YamlNode> parse_flow_sequence(size_t depth, YamlParseError& err) {
        SourceLocation loc = current_.location;
        advance(); // consume '['
        YamlNode::SequenceType seq;

        while (current_.type != TokenType::RBracket && current_.type != TokenType::Eof) {
            skip_newlines();
            if (current_.type == TokenType::RBracket) break;
            if (current_.type == TokenType::Indent) advance();

            auto item = parse_node(0, depth + 1, err);
            if (!item) return std::nullopt;
            seq.push_back(std::move(*item));

            skip_newlines();
            if (current_.type == TokenType::Comma) {
                advance();
            } else if (current_.type != TokenType::RBracket) {
                err = {"Expected ',' or ']' in flow sequence", current_.location};
                return std::nullopt;
            }
        }

        if (current_.type != TokenType::RBracket) {
            err = {"Unterminated flow sequence, expected ']'", current_.location};
            return std::nullopt;
        }
        advance(); // consume ']'
        return YamlNode(std::move(seq), loc);
    }

    std::optional<YamlNode> parse_flow_mapping(size_t depth, YamlParseError& err) {
        SourceLocation loc = current_.location;
        advance(); // consume '{'
        YamlNode::MappingType map;

        while (current_.type != TokenType::RBrace && current_.type != TokenType::Eof) {
            skip_newlines();
            if (current_.type == TokenType::RBrace) break;
            if (current_.type == TokenType::Indent) advance();

            if (current_.type != TokenType::Key && current_.type != TokenType::Scalar) {
                err = {"Expected key in flow mapping", current_.location};
                return std::nullopt;
            }
            std::string key = current_.value;
            advance();

            skip_newlines();
            auto val = parse_node(0, depth + 1, err);
            if (!val) return std::nullopt;
            map.emplace_back(std::move(key), std::move(*val));

            skip_newlines();
            if (current_.type == TokenType::Comma) {
                advance();
            } else if (current_.type != TokenType::RBrace) {
                err = {"Expected ',' or '}' in flow mapping", current_.location};
                return std::nullopt;
            }
        }

        if (current_.type != TokenType::RBrace) {
            err = {"Unterminated flow mapping, expected '}'", current_.location};
            return std::nullopt;
        }
        advance(); // consume '}'
        return YamlNode(std::move(map), loc);
    }

    std::optional<YamlNode> parse_block_sequence(size_t base_indent, size_t depth, YamlParseError& err) {
        SourceLocation loc = current_.location;
        YamlNode::SequenceType seq;

        while (current_.type == TokenType::Dash) {
            advance(); // consume '-'
            skip_newlines();

            size_t item_indent = base_indent + 2;
            if (current_.type == TokenType::Indent) {
                item_indent = current_.indent;
                advance();
            }

            auto item = parse_node(item_indent, depth + 1, err);
            if (!item) return std::nullopt;
            seq.push_back(std::move(*item));

            skip_newlines();
            if (current_.type == TokenType::Indent) {
                if (current_.indent == base_indent) {
                    advance();
                    if (current_.type != TokenType::Dash) {
                        break;
                    }
                } else if (current_.indent < base_indent) {
                    break;
                } else {
                    advance();
                }
            }
        }

        return YamlNode(std::move(seq), loc);
    }

    std::optional<YamlNode> parse_block_mapping(size_t base_indent, size_t depth, YamlParseError& err) {
        SourceLocation loc = current_.location;
        YamlNode::MappingType map;

        while (current_.type == TokenType::Key) {
            std::string key = current_.value;
            SourceLocation key_loc = current_.location;
            advance(); // consume key

            // Look at value
            if (current_.type == TokenType::Newline) {
                advance(); // consume newline
                skip_newlines();

                if (current_.type == TokenType::Indent) {
                    size_t child_indent = current_.indent;
                    if (child_indent > base_indent) {
                        advance();
                        auto val = parse_node(child_indent, depth + 1, err);
                        if (!val) return std::nullopt;
                        map.emplace_back(std::move(key), std::move(*val));
                    } else {
                        map.emplace_back(std::move(key), YamlNode(key_loc));
                    }
                } else {
                    map.emplace_back(std::move(key), YamlNode(key_loc));
                }
            } else {
                auto val = parse_node(base_indent, depth + 1, err);
                if (!val) return std::nullopt;
                map.emplace_back(std::move(key), std::move(*val));
            }

            skip_newlines();
            if (current_.type == TokenType::Indent) {
                if (current_.indent == base_indent) {
                    advance();
                    if (current_.type != TokenType::Key) {
                        break;
                    }
                } else if (current_.indent < base_indent) {
                    break;
                } else {
                    advance();
                }
            }
        }

        return YamlNode(std::move(map), loc);
    }
};

} // anonymous namespace

std::optional<YamlNode> YamlParser::parse(const std::string& input, YamlParseError& out_error) {
    if (input.empty()) {
        return YamlNode(SourceLocation{1, 1});
    }

    ParserEngine engine(input);
    return engine.parse_root(out_error);
}

} // namespace nxdev::manifest
