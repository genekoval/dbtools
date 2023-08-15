#pragma once

#include <filesystem>
#include <netcore/netcore>
#include <pg++/pg++>
#include <span>
#include <string>
#include <vector>
#include <verp/verp>

namespace dbtools {
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
        pg::parameters parameters;
        std::optional<pg::client> client_storage;

        auto analyze() -> ext::task<>;

        auto api_schema_name() -> std::string_view;

        auto client() -> ext::task<std::reference_wrapper<pg::client>>;

        auto create_api_schema() -> ext::task<>;

        auto create_schema(std::string_view schema) -> ext::task<>;

        auto drop_api_schema() -> ext::task<>;

        auto drop_schema(std::string_view schema) -> ext::task<>;

        template <typename... Args>
        requires (std::convertible_to<Args, std::string_view> && ...)
        auto exec(std::string_view program, Args&&... args) -> ext::task<> {
            co_await netcore::proc::exec(program,
                "--dbname", opts.connection_string,
                std::forward<Args>(args)...
            );
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

        auto reset(std::string_view version) -> ext::task<>;

        auto restore(std::string_view file) -> ext::task<>;
    };
}
