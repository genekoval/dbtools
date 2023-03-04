#include <dbtools/postgresql.h>

#include <fmt/format.h>

namespace fs = std::filesystem;

namespace {
}

namespace dbtools {
    auto postgresql::migrate(std::string_view version) -> ext::task<> {
        const auto v = verp::version(version);

        co_await migrate_data(v);
        co_await update(v);
    }

    auto postgresql::migrate_data(const verp::version& version) -> ext::task<> {
        constexpr auto migration_directory = "migration";

        const auto set_search_path = fmt::format(
            "SET search_path TO {}",
            data_schema
        );

        auto& client = (co_await this->client()).get();
        co_await client.exec(set_search_path);

        const auto schema_version =
            (co_await this->schema_version()).value_or(verp::version());

        // Nothing to migrate.
        if (version == schema_version) co_return;

        if (schema_version > version) {
            throw std::runtime_error(fmt::format(
                "schema version ({}) is greater than app version ({}): "
                "downgrades are not supported",
                schema_version,
                version
            ));
        }

        const auto dir = opts.sql_directory / migration_directory;

        if (!fs::exists(dir)) co_return;

        if (!fs::is_directory(dir)) {
            throw std::runtime_error(fmt::format(
                "{}: not a directory",
                dir.native()
            ));
        }

        // The map automatically sorts the scripts based on version.
        auto migrations = std::map<verp::version, fs::path>();

        for (const auto& entry : fs::directory_iterator(dir)) {
            const auto& path = entry.path();

            if (!(
                entry.is_regular_file() && path.extension() == sql_extension
            )) continue;

            const auto ver = verp::version(path.stem().native());

            if (ver < schema_version || ver >= version) continue;
            migrations[ver] = path;
        }

        auto it = migrations.begin();
        const auto end = migrations.end();

        while (it != end) {
            const auto& [ver, path] = *it++;
            const auto& file = path.native();

            TIMBER_INFO("migrate {}\n", ver);

            co_await sql(
               "--command", set_search_path,
               "--file", file
            );

            co_await this->schema_version(it == end ? version : ver);
        }
    }

    auto postgresql::schema_version() ->
        ext::task<std::optional<verp::version>>
    {
        auto& client = (co_await this->client()).get();

        const auto version_exists = co_await client.fetch<bool>(
            "SELECT exists("
                "SELECT * FROM pg_proc WHERE proname = 'schema_version'"
            ")"
        );

        if (!version_exists) co_return std::nullopt;

        const auto version_string = co_await client.fetch<std::string>(
            "SELECT data.schema_version()"
        );

        co_return verp::version(version_string);
    }

    auto postgresql::schema_version(
        const verp::version& version
    ) -> ext::task<> {
        auto& client = (co_await this->client()).get();

        co_await client.exec(fmt::format(
            "CREATE OR REPLACE FUNCTION data.schema_version() "
            "RETURNS text AS $$ BEGIN "
                "RETURN '{}'; "
            "END; $$ "
            "IMMUTABLE "
            "LANGUAGE plpgsql",
            version
        ));
    }

    auto postgresql::update(const verp::version& version) -> ext::task<> {
        const auto file = std::string(api_schema) + sql_extension;
        const auto path = (opts.sql_directory / file).string();
        co_await sql("--file", path);

        co_await schema_version(version);
    }
}
