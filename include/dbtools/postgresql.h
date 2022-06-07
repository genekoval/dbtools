#pragma once

#include <filesystem>
#include <pqxx/pqxx>
#include <span>
#include <string>
#include <vector>

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

        static auto read_schema_version(
            pqxx::transaction_base& tx
        ) -> std::optional<std::string>;
    private:
        static constexpr auto api_schema = "api";
        static constexpr auto data_schema = "data";
        static constexpr auto sql_extension = ".sql";

        const options opts;

        auto analyze() const -> void;

        auto exec(
            std::string_view program,
            std::span<const std::string_view> args
        ) const -> void;

        auto migrate_data(std::string_view version) const -> void;

        template <typename ...Args>
        auto sql(Args&&... args) const -> void {
            $(opts.client_program,
                "--set", "ON_ERROR_STOP=1",
                "--quiet",
                std::forward<Args>(args)...
            );
        }

        auto update(std::string_view version) const -> void;

        auto wait_exec(
            std::string_view program,
            std::span<const std::string_view> args
        ) const -> void;

        template <typename ...Args>
        auto $(std::string_view program, Args&&... args) const -> void {
            const auto arg_list = std::vector<std::string_view> {args...};
            wait_exec(program, arg_list);
        }
    public:
        postgresql(options&& opts);

        auto dump(std::string_view file) const -> void;

        auto exec(std::span<const std::string_view> args) const -> void;

        auto init(std::string_view version) const -> void;

        auto migrate(std::string_view version) const -> void;

        auto restore(std::string_view file) const -> void;
    };
}
