#include <dbtools/postgresql.h>

#include <fmt/format.h>
#include <pqxx/pqxx>

namespace fs = std::filesystem;

namespace {
    namespace internal {
        constexpr auto migration_dir = "migration";

        auto read_schema_version(
            pqxx::transaction_base& tx
        ) -> std::optional<std::string> {
            const auto version_exists = tx.exec1(
                "SELECT exists("
                    "SELECT * FROM pg_proc WHERE proname = 'schema_version'"
                ")"
            )[0].as<bool>();

            if (!version_exists) return {};

            return tx.exec1(
                "SELECT schema_version()"
            )[0].as<std::string>();
        }

        auto write_schema_version(
            pqxx::transaction_base& tx,
            std::string_view version
        ) -> void {
            tx.exec0(fmt::format(
                "CREATE OR REPLACE FUNCTION schema_version() "
                "RETURNS text AS $$ BEGIN "
                    "RETURN '{}'; "
                "END; $$ "
                "IMMUTABLE "
                "LANGUAGE plpgsql",
                version
            ));
        }
    }
}

namespace dbtools {
    auto postgresql::migrate(std::string_view version) const -> void {
        migrate_data(version);
        update(version);
    }

    auto postgresql::migrate_data(std::string_view version) const -> void {
        const auto set_search_path = fmt::format(
            "SET search_path TO {}",
            data_schema
        );

        auto connection = pqxx::connection(opts.connection_string);
        auto tx = pqxx::nontransaction(connection);
        tx.exec0(set_search_path);

        const auto schema_version = internal::read_schema_version(tx)
            .value_or("0.1.0");

        // Nothing to migrate.
        if (version == schema_version) return;

        if (schema_version > version) {
            throw std::runtime_error(fmt::format(
                "schema version ({}) is greater than app version ({}): "
                "downgrades are not supported",
                schema_version,
                version
            ));
        }

        const auto migration_dir = opts.sql_directory / internal::migration_dir;

        if (!fs::exists(migration_dir)) return;

        if (!fs::is_directory(migration_dir)) {
            throw std::runtime_error(fmt::format(
                "{}: not a directory",
                migration_dir.string()
            ));
        }

        // The map automatically sorts the scripts based on version.
        auto migrations = std::map<std::string, fs::path>();

        for (const auto& entry : fs::directory_iterator(migration_dir)) {
            const auto& path = entry.path();

            if (!(
                entry.is_regular_file() && path.extension() == sql_extension
            )) continue;

            const auto& ver = path.stem().native();

            if (ver < schema_version || ver >= version) continue;
            migrations[ver] = path;
        }

        auto it = migrations.begin();
        const auto end = migrations.end();

        while (it != end) {
            const auto& [key, value] = *it++;
            const auto& file = value.native();

            fmt::print("migrate {}\n", key);

           sql(
               "--command", set_search_path,
               "--file", file
            );

            const auto next = it == end ? version : it->first;
            internal::write_schema_version(tx, next);
        }
    }

    auto postgresql::update(std::string_view version) const -> void {
        const auto file = std::string(api_schema) + sql_extension;
        const auto path = (opts.sql_directory / file).string();
        sql("--file", path);

        auto connection = pqxx::connection(opts.connection_string);
        auto tx = pqxx::nontransaction(connection);
        internal::write_schema_version(tx, version);
    }
}
