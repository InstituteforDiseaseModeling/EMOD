#!/usr/bin/python

class CommandlineGenerator(object):
    def __init__(self, exe_path, options, params):
        self._exe_path = exe_path
        self._options  = options
        self._params   = params
        
    @property
    def Executable(self):
        return self._exe_path
    
    @property
    def Options(self):
        options = []
        for k,v in self._options.items():
            if k[-1] == ':':
                options.append(k + v)   # if the option ends in ':', don't insert a space
            else:
                options.extend([k,v])   # otherwise let join (below) add a space

        return ' '.join(options)
    
    @property
    def Params(self):
        return ' '.join(self._params)
    
    @property
    def Commandline(self):
        return ' '.join([self.Executable, self.Options, self.Params])

        

