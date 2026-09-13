#include <nxdev/cli/commands.hpp>
#include <nxdev/doctor/doctor.hpp>
#include <iostream>

namespace nxdev::cli {

int DoctorCommand::execute(std::span<const std::string> args, CommandContext& ctx) {
    bool as_json = false;
    doctor::DoctorProfile profile = doctor::DoctorProfile::All;

    for (const auto& a : args) {
        if (a == "--json") as_json = true;
        else if (a == "--build") profile = doctor::DoctorProfile::Build;
        else if (a == "--pack") profile = doctor::DoctorProfile::Pack;
        else if (a == "--deploy" || a == "--run") profile = doctor::DoctorProfile::Run;
    }

    doctor::Doctor doc;
    doc.run_diagnostics(ctx.env, ctx.project, profile);

    if (as_json) {
        std::cout << doc.to_json() << "\n";
    } else {
        doc.print_human_report(std::cout);
    }

    return doc.has_errors() ? 1 : 0;
}

} // namespace nxdev::cli
