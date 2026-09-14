#include <nxdev/exec/process.hpp>

namespace nxdev::exec {

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    const ProcessResourcePolicy& policy
) {
    return ControlledProcessRunner::execute(binary, args, policy);
}

ProcessResult ProcessExecutor::execute(
    const std::string& binary,
    const std::vector<std::string>& args,
    uint32_t timeout_ms,
    const std::string& working_dir
) {
    ProcessResourcePolicy policy;
    policy.timeout_ms = timeout_ms;
    policy.working_dir = working_dir;
    policy.max_output_tail_bytes = 1048576; // 1 MiB bounded tail for general CLI tool executions
    return ControlledProcessRunner::execute(binary, args, policy);
}

ProcessResult ProcessExecutor::probe_version(
    const std::string& binary_path,
    const std::string& version_flag,
    uint32_t timeout_ms
) {
    return execute(binary_path, {version_flag}, timeout_ms);
}

} // namespace nxdev::exec
