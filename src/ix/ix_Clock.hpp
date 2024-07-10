#pragma once

#include "ix.hpp"
#include "ix_file.hpp"

class ix_Clock
{
    uint64_t m_recorded;

  public:
    ix_Clock();

    void capture();

    double elaplsed_sec() const;
    double elaplsed_ms() const;
    double elaplsed_us() const;
    double elaplsed_ns() const;

    double lap_sec();
    double lap_ms();
    double lap_us();
    double lap_ns();

    void report_sec(const char *title, const ix_FileHandle *file = nullptr) const;
    void report_ms(const char *title, const ix_FileHandle *file = nullptr) const;
    void report_us(const char *title, const ix_FileHandle *file = nullptr) const;
    void report_ns(const char *title, const ix_FileHandle *file = nullptr) const;

    struct BenchmarkOption
    {
        size_t num_warmups;
        size_t num_trials;

        BenchmarkOption();
    };

    // TODO: Auto-detect the proper unit of time to print.

    template <typename F>
    void benchmark_sec(const char *title, const F &f, const BenchmarkOption &option = BenchmarkOption(),
                       const ix_FileHandle *file = nullptr)
    {
        // Warnup
        for (size_t i = 0; i < option.num_warmups; i++)
        {
            f();
        }

        capture();
        for (size_t i = 0; i < option.num_trials; i++)
        {
            f();
        }

        double elapsed = elaplsed_sec() / static_cast<double>(option.num_trials);

        if (file == nullptr)
        {
            file = &ix_FileHandle::of_stdout();
        }
        file->write_stringf("[ix_Clock] Benchmark result: %9.3f sec - %s\n", elapsed, title);
    }

    template <typename F>
    void benchmark_ms(const char *title, const F &f, const BenchmarkOption &option = BenchmarkOption(),
                      const ix_FileHandle *file = nullptr)
    {
        for (size_t i = 0; i < option.num_warmups; i++)
        {
            f();
        }

        capture();
        for (size_t i = 0; i < option.num_trials; i++)
        {
            f();
        }

        double elapsed = elaplsed_ms() / static_cast<double>(option.num_trials);
        if (file == nullptr)
        {
            file = &ix_FileHandle::of_stdout();
        }
        file->write_stringf("[ix_Clock] Benchmark result: %9.3f ms - %s\n", elapsed, title);
    }

    template <typename F>
    void benchmark_us(const char *title, const F &f, const BenchmarkOption &option = BenchmarkOption(),
                      const ix_FileHandle *file = nullptr)
    {
        // Warnup
        for (size_t i = 0; i < option.num_warmups; i++)
        {
            f();
        }

        capture();
        for (size_t i = 0; i < option.num_trials; i++)
        {
            f();
        }

        double elapsed = elaplsed_us() / static_cast<double>(option.num_trials);
        if (file == nullptr)
        {
            file = &ix_FileHandle::of_stdout();
        }
        file->write_stringf("[ix_Clock] Benchmark result: %9.3f us - %s\n", elapsed, title);
    }

    template <typename F>
    void benchmark_ns(const char *title, const F &f, const BenchmarkOption &option = BenchmarkOption(),
                      const ix_FileHandle *file = nullptr)
    {
        // Warnup
        for (size_t i = 0; i < option.num_warmups; i++)
        {
            f();
        }

        capture();
        for (size_t i = 0; i < option.num_trials; i++)
        {
            f();
        }

        double elapsed = elaplsed_ns() / static_cast<double>(option.num_trials);
        if (file == nullptr)
        {
            file = &ix_FileHandle::of_stdout();
        }
        file->write_stringf("[ix_Clock] Benchmark result: %9.3f ns - %s\n", elapsed, title);
    }
};
