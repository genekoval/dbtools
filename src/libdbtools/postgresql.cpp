#include <dbtools/postgresql.h>

#include <ext/unix.h>
#include <fmt/format.h>
#include <timber/timber>

namespace fs = std::filesystem;

namespace dbtools {
    postgresql::postgresql(options&& opts) : opts(std::move(opts)) {}

    auto postgresql::analyze() const -> void {
        $(opts.client_program, "--command", "ANALYZE");
    }

    auto postgresql::dump(std::string_view file) const -> void {
        $(opts.dump_program, "--format", "custom", "--file", file);
        TIMBER_DEBUG("Saved database dump to: {}", file);
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
        const auto file = std::string(data_schema) + sql_extension;
        const auto path = (opts.sql_directory / file).string();
        sql("--file", path);

        update();
    }

    auto postgresql::restore(std::string_view file) const -> void {
        $(opts.restore_program, "--clean", "--if-exists", file);
        analyze();
    }

    auto postgresql::update() const -> void {
        const auto file = std::string(api_schema) + sql_extension;
        const auto path = (opts.sql_directory / file).string();
        sql("--file", path);
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
