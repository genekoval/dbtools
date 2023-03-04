#include <dbtools/postgresql.h>

using netcore::proc::code;

namespace {
    namespace internal {
        class argv {
            char** handle;
        public:
            argv(
                std::string_view program,
                std::span<const std::string_view> args
            ) :
                handle(new char*[args.size() + 2])
            {
                handle[args.size() + 1] = nullptr;

                handle[0] = new char[program.size() + 1];
                std::memcpy(handle[0], program.data(), program.size());
                handle[0][program.size()] = '\0';

                for (auto i = 1; i <= args.size(); ++i) {
                    const auto arg = args[i - 1];

                    handle[i] = new char[arg.size() + 1];
                    std::memcpy(handle[i], arg.data(), arg.size());
                    handle[i][arg.size()] = '\0';
                }
            }

            ~argv() {
                auto* current = handle;

                while (current) {
                    delete[] *current;
                    ++current;
                }

                delete[] handle;
            }

            auto data() const noexcept -> char** {
                return handle;
            }
        };

        [[noreturn]]
        auto exec(
            std::string_view program,
            std::span<const std::string_view> args
        ) -> void {
            auto signals = sigset_t();
            sigemptyset(&signals);
            sigprocmask(SIG_SETMASK, &signals, nullptr);

            const auto argv = internal::argv(program, args);
            execvp(*argv.data(), argv.data());

            fmt::print(stderr, "{}: {}\n", program, strerror(errno));
            exit(errno);
        }

        auto fork(
            std::string_view program,
            std::span<const std::string_view> args
        ) -> ext::task<netcore::proc::state> {
            auto child = netcore::proc::fork();

            if (child) co_return co_await child.wait();

            exec(program, args);
        }
    }
}

namespace dbtools::detail {
    auto exec(
        std::string_view program,
        std::span<const std::string_view> args
    ) -> ext::task<> {
        TIMBER_DEBUG("EXEC {} {}", program, fmt::join(args, " "));

        const auto state = co_await internal::fork(program, args);

        if (state.code != code::exited) {
            throw std::runtime_error(fmt::format(
                "{} did not exit properly",
                program
            ));
        }
        else if (state.status != 0) {
            throw std::runtime_error(fmt::format(
                "{} exited with code {}",
                program,
                state.status
            ));
        }

        TIMBER_DEBUG("{} exited with code {}", program, state.status);
    }
}
