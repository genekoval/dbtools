#pragma once

#include <filesystem>
#include <pg++/pg++>
#include <span>
#include <string>
#include <vector>
#include <verp/verp>

namespace dbtools {
    namespace detail {
        auto exec(
            std::string_view program,
            std::span<const std::string_view> args
        ) -> ext::task<>;
    }

    class postgresql {
    public:
        struct options {
            std::string client_program = "psql";
            std::string connection_string;
            std::string dump_program = "pg_dump";
            std::string restore_program = "pg_restore";
            std::filesystem::path sql_directory;
        };
    private:
        static constexpr auto api_schema = "api";
        static constexpr auto data_schema = "data";
        static constexpr auto sql_extension = ".sql";

        const options opts;
        std::optional<pg::client> client_storage;

        auto analyze() -> ext::task<>;

        auto client() -> ext::task<std::reference_wrapper<pg::client>>;

        template <typename... Args>
        requires (std::convertible_to<Args, std::string_view> && ...)
        auto exec(std::string_view program, Args&&... args) -> ext::task<> {
            const auto argv = std::vector<std::string_view> {
                "--dbname", opts.connection_string,
                args...
            };

            co_await detail::exec(program, argv);
        }

        auto migrate_data(const verp::version& version) -> ext::task<>;

        auto schema_version() -> ext::task<std::optional<verp::version>>;

        auto schema_version(const verp::version& version) -> ext::task<>;

        template <typename ...Args>
        auto sql(Args&&... args) -> ext::task<> {
            co_await exec(opts.client_program,
                "--set", "ON_ERROR_STOP=1",
                "--quiet",
                std::forward<Args>(args)...
            );
        }

        auto update(const verp::version& version) -> ext::task<>;
    public:
        postgresql(options&& opts);

        auto dump(std::string_view file) -> ext::task<>;

        auto init(std::string_view version) -> ext::task<>;

        auto migrate(std::string_view version) -> ext::task<>;

        auto restore(std::string_view file) -> ext::task<>;
    };
}
