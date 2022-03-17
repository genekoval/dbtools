#pragma once

#include <filesystem>
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
    private:
        const options opts;

        auto analyze() const -> void;

        auto exec(
            std::string_view program,
            std::span<const std::string_view> args
        ) const -> void;

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

        auto init() const -> void;

        auto migrate() const -> void;

        auto restore(std::string_view file) const -> void;
    };
}
