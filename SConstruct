import os

env=Environment(ENV=os.environ)
env['CC']='g++'
env['CCFLAGS']='-O2 -Wall'
env['CPPPATH']=['#include/']
env.SetOption("num_jobs",2);

# debug
env['CCFLAGS']+=' -DDEBUG=1 -g3'

# we want to build an interpreter, not lib. Otherwise comment this out.:
env['CCFLAGS']+=' -Drun_interpreter=main '
env['LIBS']=['readline']

Export('env')

objs=SConscript("src/SConscript")
env.Program("escm",objs)


