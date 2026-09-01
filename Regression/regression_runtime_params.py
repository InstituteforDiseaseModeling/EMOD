import os
import sys
import configparser


class RuntimeParameters:
    def __init__(self, args):
        print("os = " + os.name)
        self.args = args
        if not os.path.exists(args.config) :
            print("Couldn't find configuration-file \"" + args.config + "\"")
            sys.exit()

        if os.name == "posix":
            self.os_type = "POSIX"
        else:
            self.os_type = "WINDOWS"

        # This silly '2' thing is just a wacky change I made that worked. Leaving it as config was broken
        self.config2 = configparser.ConfigParser()
        self.config2.read(args.config)
        self.display()

    def display(self):
        print("[arg] Suite:                      ", self.suite)
        print("[arg] Executable path:            ", self.executable_path)
        print("[arg] Run in perf mode:           ", self.measure_perf)
        print("[arg] Hide graphs on mismatch:    ", self.hide_graphs)
        print("[arg] Use DLLs:                   ", self.use_dlls)
        print("[arg] SCons:                      ", self.scons)
        print("[arg] Print error msg to screen:  ", self.print_error)
        print("[arg] Config file:                ", self.regression_config)
        print("[arg] Compare all outputs:        ", self.all_outputs)
        print("[arg] Disable schema test:        ", self.disable_schema_test)
        print("[arg] Component tests:            ", self.component_tests)
        print("[arg] Component tests show output:", self.component_tests_show_output)
        print("[arg] Config constraints:         ", self.constraints_dict)
        print("[cfg] DLL root:                   ", self.dll_root)
        print("[cfg] Input root:                 ", self.input_root)
        print("[cfg] Local bin root:             ", self.local_bin_root)
        print("[cfg] Local sim root:             ", self.local_sim_root)
        print("[cfg] Source root:                ", self.src_root)
        return

    @property
    def suite(self):
        return self.args.suite

    @property
    def executable_path(self):
        path = self.args.exe_path
        if not path:
            if self.scons:
                path = "../build/x64/Release/Eradication/Eradication"
                if os.name == "nt":
                    path += ".exe"
            else:
                path = "../Eradication/x64/Release/Eradication.exe"

        return path

    @property
    def measure_perf(self):
        return self.args.perf

    @property
    def hide_graphs(self):
        return self.args.hidegraphs

    @property
    def use_dlls(self):
        return self.args.use_dlls

    @property
    def scons(self):
        return self.args.scons

    @property
    def print_error(self):
        return self.args.print_error

    @property
    def regression_config(self):
        return self.args.config

    @property
    def config(self):
        return self.config2

    @property
    def local_sim_root(self):
        return self.config2.get(self.os_type, 'local_sim_root')

    @property
    def local_bin_root(self):
        return self.config2.get(self.os_type, 'local_bin_root')

    @property
    def input_root(self):
        return self.config2.get(self.os_type, 'local_input_root')

    @property
    def dll_root(self):
        return self.config2.get(self.os_type, 'local_bin_root')

    @property
    def src_root(self):
        return ".."

    @property
    def all_outputs(self):
        return self.args.all_outputs

    @property
    def disable_schema_test(self):
        return self.args.disable_schema_test

    @property
    def component_tests(self):
        return self.args.component_tests

    @property
    def component_tests_show_output(self):
        return self.args.component_tests_show_output

    @property
    def constraints_dict(self):
        constraints_dict = {}
        if self.args.config_constraints and len(self.args.config_constraints) > 0:
            constraints_list = self.args.config_constraints.split(",")
            for raw_nvp in constraints_list:
                nvp = raw_nvp.split(":")
                constraints_dict[nvp[0]] = nvp[1]
        return constraints_dict
