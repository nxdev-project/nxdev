#pragma once

#include <nxdev/env/environment.hpp>
#include <nxdev/project/project.hpp>
#include <string>
#include <vector>
#include <optional>
#include <iostream>

namespace nxdev::doctor {

enum class CheckStatus {
    Pass,
    Warning,
    Error,
    Info,
    Skipped
};

struct CheckItem {
    std::string id;
    std::string category;
    CheckStatus status{CheckStatus::Pass};
    std::string message;
    std::string path;
    std::string remedy_suggestion;

    [[nodiscard]] std::string status_string() const;
    [[nodiscard]] std::string symbol() const;
};

enum class DoctorProfile {
    All,
    Build,
    Pack,
    Deploy,
    Run
};

class Doctor {
public:
    Doctor() = default;
    ~Doctor() = default;

    [[nodiscard]] bool has_errors() const noexcept { return error_count_ > 0; }
    [[nodiscard]] bool has_warnings() const noexcept { return warning_count_ > 0; }
    [[nodiscard]] size_t error_count() const noexcept { return error_count_; }
    [[nodiscard]] size_t warning_count() const noexcept { return warning_count_; }
    [[nodiscard]] const std::vector<CheckItem>& checks() const noexcept { return checks_; }

    void add_check(CheckItem item);

    /**
     * @brief Executes all doctor diagnostics for the given environment and project.
     */
    void run_diagnostics(
        const env::Environment& env,
        const std::optional<project::NXDevProject>& project,
        DoctorProfile profile = DoctorProfile::All
    );

    void print_human_report(std::ostream& os) const;
    [[nodiscard]] std::string to_json() const;

private:
    std::vector<CheckItem> checks_;
    size_t pass_count_{0};
    size_t warning_count_{0};
    size_t error_count_{0};
    size_t info_count_{0};
};

} // namespace nxdev::doctor
