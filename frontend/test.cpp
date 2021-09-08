
#include <cstddef>
#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>

#include "i8080.h"
#include "i8080_opcodes.h"
#include <cpm80vm.hpp>
#include <test.hpp>
#include <util.hpp>

namespace testi8080 
{
static i8080_word_t on_interrupt(cpm80vm* vm) { return i8080_NOP; }

static int restart_then_interrupt(cpm80vm& vm, long ms_to_interrupt)
{
    vm.reset();
    std::mutex m_vm; // protects vm

    std::thread intgen_thread([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms_to_interrupt));
        std::unique_lock<std::mutex> lock(m_vm);
        vm.interrupt();
        lock.unlock();
    });

    cpm80vm_exitcode exitcode;
    while (true) {
        std::unique_lock<std::mutex> lock(m_vm);
        exitcode = vm.step();
        lock.unlock();
        if (exitcode != VM_Success) break;
    }

    intgen_thread.join();
    return exitcode;
}

static bool load_program(cpm80vm& vm, const char filepath[])
{
	char bytes[64 * 1024];
	auto res = util::read_bin_file(filepath, 0, bytes, 64 * 1024);
    bool reached_eof = res.first;
    std::size_t nread = res.second;

    if (!reached_eof) {
        std::cerr << "\nFile larger than 64K: " << filepath << ".";
        return false;
    }
    
    vm.program_write(bytes, 0x0100, 0x0100 + nread);
	return true;
}

bool run_test(const char filepath[], const struct test_params& params) 
{
    auto pre_test_msg = [&] {
        std::cout << "\n-------- running test: " << filepath << "\n";
    };
    auto post_test_msg = [](unsigned long cycles) {
        std::cout << "\n-------- executed in " << cycles << " cycles.\n\n";
    };

    if (params.emulate_cpm80)
    {
        cpm80vm vm = cpm80vm(64, nullptr, nullptr,
            params.test_interrupts ? on_interrupt : nullptr);

        bool success = load_program(vm, filepath);
        if (!success) return false;

        pre_test_msg();

        int exitcode;
        if (params.test_interrupts)
            exitcode = restart_then_interrupt(vm, 5000);
        else exitcode = vm.restart();

        post_test_msg(vm.get_cpu()->cycles);

        return exitcode == VM_ProgramExit || exitcode == VM_Success;
    } 
    else {
        // not implemented yet!

        return false;
    }
}

}
