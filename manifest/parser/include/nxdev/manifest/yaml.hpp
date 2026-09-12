#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <variant>
#include <cstdint>
#include <cstddef>

namespace nxdev::manifest {

struct SourceLocation {
    size_t line{1};
    size_t column{1};
};

enum class YamlNodeType {
    Null,
    Scalar,
    Sequence,
    Mapping
};

class YamlNode {
public:
    using SequenceType = std::vector<YamlNode>;
    using MappingPair = std::pair<std::string, YamlNode>;
    using MappingType = std::vector<MappingPair>;

    YamlNode() : type_(YamlNodeType::Null) {}
    explicit YamlNode(SourceLocation loc) : type_(YamlNodeType::Null), loc_(loc) {}
    explicit YamlNode(std::string scalar, SourceLocation loc = {1, 1})
        : type_(YamlNodeType::Scalar), value_(std::move(scalar)), loc_(loc) {}
    explicit YamlNode(SequenceType seq, SourceLocation loc = {1, 1})
        : type_(YamlNodeType::Sequence), value_(std::move(seq)), loc_(loc) {}
    explicit YamlNode(MappingType map, SourceLocation loc = {1, 1})
        : type_(YamlNodeType::Mapping), value_(std::move(map)), loc_(loc) {}

    [[nodiscard]] YamlNodeType type() const noexcept { return type_; }
    [[nodiscard]] SourceLocation location() const noexcept { return loc_; }
    void set_location(SourceLocation loc) noexcept { loc_ = loc; }

    [[nodiscard]] bool is_null() const noexcept { return type_ == YamlNodeType::Null; }
    [[nodiscard]] bool is_scalar() const noexcept { return type_ == YamlNodeType::Scalar; }
    [[nodiscard]] bool is_sequence() const noexcept { return type_ == YamlNodeType::Sequence; }
    [[nodiscard]] bool is_mapping() const noexcept { return type_ == YamlNodeType::Mapping; }

    [[nodiscard]] const std::string& as_string() const;
    [[nodiscard]] std::optional<int64_t> as_int() const noexcept;
    [[nodiscard]] std::optional<uint64_t> as_uint() const noexcept;
    [[nodiscard]] std::optional<bool> as_bool() const noexcept;

    [[nodiscard]] const SequenceType& as_sequence() const;
    [[nodiscard]] const MappingType& as_mapping() const;

    [[nodiscard]] bool has_key(const std::string& key) const noexcept;
    [[nodiscard]] const YamlNode* get(const std::string& key) const noexcept;
    [[nodiscard]] const YamlNode* get(size_t index) const noexcept;

    void set_mapping_entry(const std::string& key, YamlNode node);
    void append_sequence_entry(YamlNode node);

private:
    YamlNodeType type_{YamlNodeType::Null};
    std::variant<std::monostate, std::string, SequenceType, MappingType> value_;
    SourceLocation loc_{1, 1};
};

struct YamlParseError {
    std::string message;
    SourceLocation location{1, 1};
};

class YamlParser {
public:
    static constexpr size_t MAX_RECURSION_DEPTH = 64;

    YamlParser() = default;
    ~YamlParser() = default;

    [[nodiscard]] std::optional<YamlNode> parse(const std::string& input, YamlParseError& out_error);
};

} // namespace nxdev::manifest
