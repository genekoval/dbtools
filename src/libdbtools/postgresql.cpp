#include <dbtools/postgresql.h>

#include <ext/unix.h>
#include <fmt/format.h>
#include <timber/timber>

namespace fs = std::filesystem;

namespace {
    constexpr auto api_schema = "api.sql";
    constexpr auto data_schema = "data.sql";
    constexpr auto stop_on_error = "--set=ON_ERROR_STOP=1";
}

namespace dbtools {
    postgresql::postgresql(options&& opts) : opts(std::move(opts)) {}

    auto postgresql::analyze() const -> void {
        $(opts.client_program, "--command", "ANALYZE");
    }

    auto postgresql::dump(std::string_view file) const -> void {
        $(opts.dump_program, "--format", "custom", "--file", file);
        DEBUG() << "Saved database dump to: " << file;
    }

    auto postgresql::exec(
        std::string_view program,
        std::span<const std::string_view> args
    ) const -> void {
        auto arguments = std::vector<std::string_view> {
            "--dbname", opts.connection_string
        };

        std::copy(args.begin(), args.end(), std::back_inserter(arguments));

        ext::exec(program, arguments);
    }

    auto postgresql::exec(
        std::span<const std::string_view> args
    ) const -> void {
        exec(opts.client_program, args);
    }

    auto postgresql::init() const -> void {
        const auto schema = (opts.sql_directory / data_schema).string();
        $(opts.client_program, stop_on_error, "--file", schema);
        migrate();
    }

    auto postgresql::migrate() const -> void {
        const auto schema = (opts.sql_directory / api_schema).string();
        $(opts.client_program, stop_on_error, "--file", schema);
    }

    auto postgresql::restore(std::string_view file) const -> void {
        $(opts.restore_program, "--clean", "--if-exists", file);
        analyze();
    }

    auto postgresql::wait_exec(
        std::string_view program,
        std::span<const std::string_view> args
    ) const -> void {
        const auto parent = ext::process::fork();

        if (!parent) exec(program, args);

        const auto process = *parent;
        const auto exit = process.wait();

        if (exit.code == CLD_EXITED) {
            if (exit.status == 0) return;

            throw std::runtime_error(fmt::format(
                "{} exited with code {}",
                program,
                exit.status
            ));
        }

        if (exit.code == CLD_KILLED || exit.code == CLD_DUMPED) {
            throw std::runtime_error(fmt::format(
                "{} was killed by signal {}",
                program,
                exit.status
            ));
        }

        throw std::runtime_error(fmt::format(
            "{} did not succeed",
            program
        ));
    }
}
