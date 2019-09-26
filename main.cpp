
#include "src/emu.h"
#include "src/i8080/internal/i8080_consts.h"
#include <cstdlib>

#include <iostream>
#include <fstream>
#include <functional>
#include <vector>
#include <tuple>
#include <limits>

// remove macros coming from C
#undef max
#undef min

struct cmd_state {
    bool is_args_valid;
    cmd_state() : is_args_valid(false) {}
};

struct emu_cmd_state : cmd_state {
    bool is_cpm_env;
    bool is_runnable;
    bool is_testing;
    bool is_showing_help;
    std::string bin_file;
    emu_cmd_state() : cmd_state(),
        is_cpm_env(false),
        is_runnable(false),
        is_testing(false),
        is_showing_help(false),
        bin_file("") {}
};

const std::string cmdline_help_msg = 
"\ni8080-emu, an emulator for the INTEL 8080 microprocessor with some CP/M 2.2 BIOS support.\n"
"Supports async interrupts, CPM 2.2 BDOS ops 2 and 9, and a simple command processor at CP/M WBOOT.\n\n"
"Usage: i8080-emu [options]\n"
"Options:\n"
"   -h\t--help\t\t\tPrint this help message.\n"
"   -e\t--env ENV\t\tSet the environment. \"default\" or \"cpm\".\n"
"   -f\t--file FILE\t\tExecute the file as i8080 binary.\n"
"   --run-all-tests\t\tRun all the test files under tests/.\n";

// Prints the help message and returns EXIT failure
int cmd_print_help_msg_and_exit(cmd_state & cmd_state, std::vector<std::string>::iterator &) {
    emu_cmd_state & emu_cmd_state = *(struct emu_cmd_state *)(&cmd_state);
    // if args were valid, return normally i.e. came here from --help/-h
    if (emu_cmd_state.is_args_valid) {
        emu_cmd_state.is_showing_help = true;
        std::cout << cmdline_help_msg.c_str();
        return 1;
    } else {
        std::cerr << cmdline_help_msg.c_str();
        return 0;
    }
}

int cmd_set_emu_env(cmd_state & cmd_state, std::vector<std::string>::iterator & opt_args_itr) {
    int success = 1;
    emu_cmd_state & emu_cmd_state = *(struct emu_cmd_state *)(&cmd_state);
    if (*opt_args_itr == "0" || *opt_args_itr == "DEFAULT" || *opt_args_itr == "default") {
        emu_cmd_state.is_cpm_env = false;
    } else if (*opt_args_itr == "1" || *opt_args_itr == "CPM" || *opt_args_itr == "cpm") {
        emu_cmd_state.is_cpm_env = true;
    } else {
        std::cout << "--env value invalid. Try --env cpm or --env default." << std::endl;
        success = 0;
    }
    return success;
}

int cmd_set_bin_file(cmd_state & cmd_state, std::vector<std::string>::iterator & opt_args_itr) {
    int success = 0;
    emu_cmd_state & emu_cmd_state = *(struct emu_cmd_state *)(&cmd_state);
    std::string & filename = *opt_args_itr;
    // Check if file exists and if it is at most 64KB in size
    std::ifstream bin_fstream(filename, std::ios::in | std::ios::binary);
    if (bin_fstream.is_open()) {
        // Extract all characters
        bin_fstream.ignore(std::numeric_limits<std::streamsize>::max());
        // check file read failure or too large
        if (!bin_fstream.fail() && bin_fstream.eof()) {
            std::streamsize length = bin_fstream.gcount();
            if (length > ADDR_T_MAX + 1) {
                std::cout << "File too large." << std::endl;
            } else {
                // success, file is readable and within size limits
                emu_cmd_state.bin_file = filename;
                emu_cmd_state.is_runnable = true;
                success = 1;
            }
        } else {
            std::cout << "File read error." << std::endl;
        }
        bin_fstream.close();
    } else {
        std::cout << "File could not be opened." << std::endl;
    }
    return success;
}

int cmd_set_test_run(cmd_state & cmd_state, std::vector<std::string>::iterator &) {
    emu_cmd_state & emu_cmd_state = *(struct emu_cmd_state *)(&cmd_state);
    emu_cmd_state.is_runnable = true;
    emu_cmd_state.is_testing = true;
    return 1;
}

using cmd_callback_fn = std::function<int(cmd_state & cmd_state, std::vector<std::string>::iterator & cmdline_args)>;
using cmd_handle = std::tuple<std::string, std::string, bool, cmd_callback_fn>;

// {<fullname>, <alias>, <has_args>, <callback>}
const std::vector<cmd_handle> CMDLINE_COMMANDS {
    cmd_handle("--help", "-h", false, cmd_callback_fn(cmd_print_help_msg_and_exit)),
    cmd_handle("--env", "-e", true, cmd_callback_fn(cmd_set_emu_env)),
    cmd_handle("--file", "-f", true, cmd_callback_fn(cmd_set_bin_file)),
    cmd_handle("--run-all-tests", "", false, cmd_callback_fn(cmd_set_test_run))
};

int process_cmdline(int argc, char ** argv, cmd_state & cmd_state) {

    std::vector<std::string> CMDLINE_ARGS;
    CMDLINE_ARGS.reserve(3);
    // if the last command exec was successful
    int exec_success = 0;

    if (argc < 2) {
        // not enough args, print help message and exit
        cmd_state.is_args_valid = false;
        return cmd_print_help_msg_and_exit(cmd_state, CMDLINE_ARGS.begin());
    } else {
        cmd_state.is_args_valid = true;

        // copy the cmd line args in argv
        for (int i = 1; i < argc; ++i) {
            CMDLINE_ARGS.push_back(std::string(argv[i]));
        }

        // Function that is called when command is identified
        cmd_callback_fn cmd_callback;
        bool cmd_has_args;

        auto args_itr = CMDLINE_ARGS.begin();
        while (args_itr != CMDLINE_ARGS.end()) {
            cmd_callback = nullptr;
            cmd_has_args = false;

            std::string & arg = *args_itr;
            // Check against all commands
            for (std::size_t j = 0; j < CMDLINE_COMMANDS.size(); ++j) {
                const cmd_handle & cmd_handle = CMDLINE_COMMANDS[j];
                // check if matches full name or alias
                const std::string & cmd_name = std::get<0>(cmd_handle);
                const std::string & cmd_alias = std::get<1>(cmd_handle);
                if (arg == cmd_name || (!cmd_alias.empty() && arg == cmd_alias)) {
                    cmd_has_args = std::get<2>(cmd_handle);
                    cmd_callback = std::get<3>(cmd_handle);
                }
            }

            if (cmd_callback) {
                // check if cmd args are past end of cmdline args
                args_itr++;
                if (cmd_has_args && args_itr == CMDLINE_ARGS.end()) {
                    std::cout << arg.c_str() << ": Missing arguments. See \"--help\" for usage." << std::endl;
                    exec_success = 0;
                } else {
                    exec_success = cmd_callback(cmd_state, args_itr);
                    // if the cmd had args, move past its last arg
                    if (cmd_has_args) args_itr++;
                }
            } else {
                std::cout << arg.c_str() << " is not a command. See \"--help\" for usage." << std::endl;
                exec_success = 0;
            }

            // if any command execution fails, quit
            if (!exec_success) break;
        }
    }

    return exec_success;
}

// See run.cpp. These functions show example usage of lib-i8080emu.
int run_all_tests();
int run_i8080_bin(const std::string & bin_file, bool is_cpm_env);

int main(int argc, char ** argv) {
    int exit_success = EXIT_FAILURE;
    //if (EMU_ENV == (enum emu_env)CPM) printf("\nCP/M Warm Boot. Type HELP for a list of commands.");
    emu_cmd_state EMU_CMD_STATE;
    int cmd_success = process_cmdline(argc, argv, EMU_CMD_STATE);

    if (cmd_success && EMU_CMD_STATE.is_runnable) {
        if (EMU_CMD_STATE.is_testing) {
            run_all_tests();
        } else {
            run_i8080_bin(EMU_CMD_STATE.bin_file, EMU_CMD_STATE.is_cpm_env);
        }
        exit_success = EXIT_SUCCESS;
    }
    // not runnable, no file provided
    if (!EMU_CMD_STATE.is_runnable && !EMU_CMD_STATE.is_showing_help && cmd_success) {
        std::cout << "Use --file to specify source of i8080 binary." << std::endl;
        exit_success = EXIT_FAILURE;
    }
    return exit_success;
}