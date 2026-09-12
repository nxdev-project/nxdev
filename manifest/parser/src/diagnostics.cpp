#include <nxdev/manifest/diagnostics.hpp>
#include <sstream>

namespace nxdev::manifest {

std::string Diagnostic::severity_string() const noexcept {
    switch (severity) {
        case DiagnosticSeverity::Error: return "error";
        case DiagnosticSeverity::Warning: return "warning";
        case DiagnosticSeverity::Info: return "info";
        case DiagnosticSeverity::Note: return "note";
    }
    return "unknown";
}

std::string Diagnostic::to_string() const {
    std::ostringstream oss;
    if (!filename.empty()) {
        oss << filename << ":" << location.line << ":" << location.column << ": ";
    } else {
        oss << location.line << ":" << location.column << ": ";
    }

    oss << severity_string() << " [" << code << "]: ";
    if (!field_path.empty()) {
        oss << field_path << ": ";
    }
    oss << message;
    return oss.str();
}

void DiagnosticCollector::add(Diagnostic diag) {
    if (diag.severity == DiagnosticSeverity::Error) {
        error_count_++;
    } else if (diag.severity == DiagnosticSeverity::Warning) {
        warning_count_++;
    }
    diagnostics_.push_back(std::move(diag));
}

void DiagnosticCollector::add_error(std::string_view code, std::string message,
                                    std::string field_path, SourceLocation loc, std::string filename) {
    add(Diagnostic{
        .severity = DiagnosticSeverity::Error,
        .code = std::string(code),
        .message = std::move(message),
        .field_path = std::move(field_path),
        .filename = std::move(filename),
        .location = loc
    });
}

void DiagnosticCollector::add_warning(std::string_view code, std::string message,
                                      std::string field_path, SourceLocation loc, std::string filename) {
    add(Diagnostic{
        .severity = DiagnosticSeverity::Warning,
        .code = std::string(code),
        .message = std::move(message),
        .field_path = std::move(field_path),
        .filename = std::move(filename),
        .location = loc
    });
}

void DiagnosticCollector::add_info(std::string_view code, std::string message,
                                   std::string field_path, SourceLocation loc, std::string filename) {
    add(Diagnostic{
        .severity = DiagnosticSeverity::Info,
        .code = std::string(code),
        .message = std::move(message),
        .field_path = std::move(field_path),
        .filename = std::move(filename),
        .location = loc
    });
}

bool DiagnosticCollector::has_errors() const noexcept {
    return error_count_ > 0;
}

bool DiagnosticCollector::has_warnings() const noexcept {
    return warning_count_ > 0;
}

void DiagnosticCollector::print_summary(std::ostream& os) const {
    for (const auto& diag : diagnostics_) {
        os << diag.to_string() << "\n";
    }
    if (error_count_ > 0 || warning_count_ > 0) {
        os << "Validation finished with " << error_count_ << " error(s), "
           << warning_count_ << " warning(s).\n";
    }
}

void DiagnosticCollector::clear() {
    diagnostics_.clear();
    error_count_ = 0;
    warning_count_ = 0;
}

} // namespace nxdev::manifest
