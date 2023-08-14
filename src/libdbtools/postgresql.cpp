#include <dbtools/postgresql.h>

#include <fmt/format.h>
#include <timber/timber>

namespace dbtools {
    postgresql::postgresql(options&& opts) :
        opts(std::move(opts)),
        parameters(pg::parameters::parse(this->opts.connection_string))
    {}

    auto postgresql::analyze() -> ext::task<> {
        auto& client = (co_await this->client()).get();
        co_await client.exec("ANALYZE");
    }

    auto postgresql::api_schema_name() -> std::string_view {
        return parameters.params.at("user");
    }

    auto postgresql::client() -> ext::task<std::reference_wrapper<pg::client>> {
        if (!client_storage) client_storage = co_await pg::connect(parameters);
        co_return *client_storage;
    }

    auto postgresql::create_api_schema() -> ext::task<> {
        co_await create_schema(api_schema_name());
    }

    auto postgresql::create_schema(std::string_view schema) -> ext::task<> {
        auto& client = (co_await this->client()).get();
        co_await client.exec(fmt::format("CREATE SCHEMA {}", schema));
    }

    auto postgresql::drop_api_schema() -> ext::task<> {
        co_await drop_schema(api_schema_name());
    }

    auto postgresql::drop_schema(std::string_view schema) -> ext::task<> {
        auto& client = (co_await this->client()).get();
        co_await client.exec(fmt::format(
            "DROP SCHEMA IF EXISTS {} CASCADE",
            schema
        ));
    }

    auto postgresql::dump(std::string_view file) -> ext::task<> {
        co_await exec(opts.dump_program, "--format", "custom", "--file", file);
        TIMBER_DEBUG("Saved database dump to: {}", file);
    }

    auto postgresql::init(std::string_view version) -> ext::task<> {
        const auto file = std::string(data_schema) + sql_extension;
        const auto path = (opts.sql_directory / file).string();
        co_await sql("--file", path);

        co_await update(verp::version(version));
    }

    auto postgresql::restore(std::string_view file) -> ext::task<> {
        co_await exec(opts.restore_program, "--clean", "--if-exists", file);
        co_await analyze();
    }
}
