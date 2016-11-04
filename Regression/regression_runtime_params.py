#!/usr/bin/python

import os
import ConfigParser
import pdb

class RuntimeParameters:
    def __init__(self, args):
        print( "os = " + os.name )
        self.args = args
        if not os.path.exists(args.config) :
            print("Couldn't find configuration-file \"" + args.config + "\"")
            sys.exit()

        user = None
        username_key = None
        if os.name == "posix":
            self.os_type = "POSIX"
            username_key = "USER"
            user = ""
        else:
            self.os_type = "WINDOWS"
            username_key = "USERNAME"
            user = os.environ["USERDOMAIN"] + '\\'
            
        user += os.environ[username_key]
        self.config = ConfigParser.SafeConfigParser({'password':'', 'username':user})
        self.config.read(args.config)
        self.config.set('ENVIRONMENT', 'username', os.environ[username_key])
        self._use_user_input_root = False
        self.PSP = None
    
    @property
    def suite(self):
        return self.args.suite
        
    @property
    def executable_path(self):
        return self.args.exe_path
    
    @property
    def measure_perf(self):
        return self.args.perf
    
    @property
    def hide_graphs(self):
        return self.args.hidegraphs
    
    @property
    def debug(self):
        return self.args.debug
    
    @property
    def quick_start(self):
        return self.args.quick_start
    
    @property
    def use_dlls(self):
        if self.args.dll_path is not None:
            return True
        else:
            return self.args.use_dlls
    
    @property
    def scons(self):
        return self.args.scons
    
    @property
    def label(self):
        return self.args.label

    @property
    def regression_config(self):
        return self.args.config
        
    @property
    def config(self):
        return self.config

    @property
    def hpc_head_node(self):
        return self.config.get('HPC', 'head_node')
        
    @property
    def hpc_node_group(self):
        return self.config.get('HPC', 'node_group')
        
    @property
    def hpc_user(self):
        return self.config.get('HPC', 'username')
        
    @property
    def hpc_password(self):
        return self.config.get('HPC', 'password')
        
    @property
    def cores_per_socket(self):
        return self.config.getint('HPC', 'num_cores_per_socket')
        
    @property
    def cores_per_node(self):
        return self.config.getint('HPC', 'num_cores_per_node')

    @property
    def local_sim_root(self):
        return self.config.get(self.os_type, 'local_sim_root')
        
    @property
    def input_path(self):
        return self.config.get(self.os_type, 'home_input')
        
    @property
    def local_bin_root(self):
        return self.config.get(self.os_type, 'local_bin_root')

    @property
    def sim_root(self):
        if self.local_execution or os.name=="posix":
            return self.config.get(self.os_type, 'local_sim_root')
        else:
            return self.config.get('ENVIRONMENT', 'sim_root')
        
    @property
    def shared_input(self):
        if self.local_execution or os.name=="posix":
            return self.config.get(self.os_type, 'local_input_root')
        else:
            return self.config.get('ENVIRONMENT', 'input_root')
        
    @property
    def bin_root(self):
        return self.config.get('ENVIRONMENT', 'bin_root')
        
    @property
    def user_input(self):
        if self.local_execution or os.name=="posix":
            return self.config.get(self.os_type, 'home_input')
        else:
            return self.config.get('ENVIRONMENT', 'home_input')
        
    @property
    def py_input(self):
        return self.config.get('ENVIRONMENT', 'py_input')
        
    @property
    def use_user_input_root(self):
        return self._use_user_input_root
        
    @use_user_input_root.setter
    def use_user_input_root(self, value):
        self._use_user_input_root = value
        
    @property
    def input_root(self):
        if not self.use_user_input_root:
            return self.shared_input
        else:
            return self.user_input
        
    @property
    def dll_path(self):
        return self.args.dll_path

    @property
    def dll_root(self):
        try:
            if self.local_execution:
                dll_root = self.config.get(self.os_type, 'local_dll_root')
            else:
                dll_root = self.config.get('ENVIRONMENT', 'dll_root')
        except Exception as ex:
            if self.local_execution:
                dll_root = self.config.get(self.os_type, 'local_bin_root')
            else:
                dll_root = self.config.get('ENVIRONMENT', 'bin_root')
        return dll_root

    @property
    def src_root(self):
        try:
            src_root = self.config.get('LOCAL-ENVIRONMENT', 'src_root')
        except Exception as ex:
            src_root = ".\\.."
        return src_root

    @property
    def all_outputs(self):
        return self.args.all_outputs
        
    @property
    def dts(self):
        return self.args.disable_schema_test
        
    @property
    def sec(self):
        return self.args.skip_emodule_check

    @property
    def constraints_dict(self):
        constraints_list = self.args.config_constraints
        constraints_dict = {}
        for raw_nvp in constraints_list:
            nvp = raw_nvp.split(":")
            constraints_dict[ nvp[0] ] = nvp[1]
        return constraints_dict
    
    @property
    def local_execution(self):
        return self.args.local



