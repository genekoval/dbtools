#include <dbtools/postgresql.h>

#include <fmt/format.h>
#include <timber/timber>

namespace dbtools {
    postgresql::postgresql(options&& opts) : opts(std::move(opts)) {}

    auto postgresql::analyze() -> ext::task<> {
        auto& client = (co_await this->client()).get();
        co_await client.exec("ANALYZE");
    }

    auto postgresql::client() -> ext::task<std::reference_wrapper<pg::client>> {
        if (client_storage) co_return *client_storage;

        const auto params = pg::parameters::parse(opts.connection_string);
        client_storage = co_await pg::connect(params);
        co_return *client_storage;
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
