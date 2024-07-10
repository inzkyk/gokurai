#include "ix_Clock.hpp"
#include "ix_doctest.hpp"

#include <sokol_time.h>

ix_Clock::BenchmarkOption::BenchmarkOption()
    : num_warmups(3),
      num_trials(10)
{
}

ix_Clock::ix_Clock()
{
    capture();
}

void ix_Clock::capture()
{
    m_recorded = stm_now();
}

double ix_Clock::elaplsed_sec() const
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    return stm_sec(now - previous);
}

double ix_Clock::elaplsed_ms() const
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    return stm_ms(now - previous);
}

double ix_Clock::elaplsed_us() const
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    return stm_us(now - previous);
}

double ix_Clock::elaplsed_ns() const
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    return stm_ns(now - previous);
}

double ix_Clock::lap_sec()
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    m_recorded = now;
    return stm_sec(now - previous);
}

double ix_Clock::lap_ms()
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    m_recorded = now;
    return stm_ms(now - previous);
}

double ix_Clock::lap_us()
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    m_recorded = now;
    return stm_us(now - previous);
}

double ix_Clock::lap_ns()
{
    const uint64_t now = stm_now();
    const uint64_t previous = m_recorded;
    m_recorded = now;
    return stm_ns(now - previous);
}

ix_FORCE_INLINE static const ix_FileHandle *this_or_stdout(const ix_FileHandle *file)
{
    return (file != nullptr) ? file : &ix_FileHandle::of_stdout();
}

void ix_Clock::report_sec(const char *title, const ix_FileHandle *file) const
{
    const double d = elaplsed_sec();
    this_or_stdout(file)->write_stringf("[ix_Clock] %s: %f sec\n", title, d);
}

void ix_Clock::report_ms(const char *title, const ix_FileHandle *file) const
{
    const double d = elaplsed_ms();
    this_or_stdout(file)->write_stringf("[ix_Clock] %s: %f ms\n", title, d);
}

void ix_Clock::report_us(const char *title, const ix_FileHandle *file) const
{
    const double d = elaplsed_us();
    this_or_stdout(file)->write_stringf("[ix_Clock] %s: %f us\n", title, d);
}

void ix_Clock::report_ns(const char *title, const ix_FileHandle *file) const
{
    const double d = elaplsed_ns();
    this_or_stdout(file)->write_stringf("[ix_Clock] %s: %f ns\n", title, d);
}

ix_TEST_CASE("ix_Clock::elaplsed")
{
    // without capture()
    {
        const ix_Clock c;

        const double ns = c.elaplsed_ns();
        ix_EXPECT(0.0 <= ns);
        ix_EXPECT(ns <= 1000000.0);

        const double us = c.elaplsed_us();
        ix_EXPECT(0.0 <= us);
        ix_EXPECT(us <= 1000000.0);

        const double ms = c.elaplsed_ms();
        ix_EXPECT(0.0 <= ms);
        ix_EXPECT(ms <= 100.0);

        const double sec = c.elaplsed_sec();
        ix_EXPECT(0.0 <= sec);
        ix_EXPECT(sec <= 1.0);
    }

    // with capture()
    {
        ix_Clock c;

        c.capture();
        const double ns = c.elaplsed_ns();
        ix_EXPECT(0.0 <= ns);
        ix_EXPECT(ns <= 1000000.0);

        c.capture();
        const double us = c.elaplsed_us();
        ix_EXPECT(0.0 <= us);
        ix_EXPECT(us <= 1000000.0);

        c.capture();
        const double ms = c.elaplsed_ms();
        ix_EXPECT(0.0 <= ms);
        ix_EXPECT(ms <= 100.0);

        c.capture();
        const double sec = c.elaplsed_sec();
        ix_EXPECT(0.0 <= sec);
        ix_EXPECT(sec <= 1.0);
    }
}

ix_TEST_CASE("ix_Clock::lap")
{
    // without capture()
    {
        ix_Clock c;

        const double ns = c.lap_ns();
        ix_EXPECT(0.0 <= ns);
        ix_EXPECT(ns <= 1000000.0);

        const double us = c.lap_us();
        ix_EXPECT(0.0 <= us);
        ix_EXPECT(us <= 1000000.0);

        const double ms = c.lap_ms();
        ix_EXPECT(0.0 <= ms);
        ix_EXPECT(ms <= 100.0);

        const double sec = c.lap_sec();
        ix_EXPECT(0.0 <= sec);
        ix_EXPECT(sec <= 1.0);
    }

    // with capture()
    {
        ix_Clock c;

        c.capture();
        const double ns = c.lap_ns();
        ix_EXPECT(0.0 <= ns);
        ix_EXPECT(ns <= 1000000.0);

        c.capture();
        const double us = c.lap_us();
        ix_EXPECT(0.0 <= us);
        ix_EXPECT(us <= 1000000.0);

        c.capture();
        const double ms = c.lap_ms();
        ix_EXPECT(0.0 <= ms);
        ix_EXPECT(ms <= 100.0);

        c.capture();
        const double sec = c.lap_sec();
        ix_EXPECT(0.0 <= sec);
        ix_EXPECT(sec <= 1.0);
    }
}

ix_TEST_CASE("ix_Clock: report")
{
    const ix_FileHandle file = ix_FileHandle::null();
    const ix_Clock c;

    c.report_sec("test", &file);
    c.report_ms("test", &file);
    c.report_us("test", &file);
    c.report_ns("test", &file);
}

ix_TEST_CASE("ix_Clock: benchmark")
{
    ix_Clock c;
    const ix_Clock::BenchmarkOption option;
    const ix_FileHandle null = ix_FileHandle::null();

    auto f = []() {
        size_t sum = 0;
        for (size_t i = 0; i < 1000; i++)
        {
            sum += i;
        }
        ix_EXPECT(sum == 499500);
    };

    c.benchmark_sec("sum", f, option, &null);
    c.benchmark_ms("sum", f, option, &null);
    c.benchmark_us("sum", f, option, &null);
    c.benchmark_ns("sum", f, option, &null);
}
