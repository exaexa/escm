import os

env=Environment(ENV=os.environ)
env['CC']='g++'
env['CCFLAGS']='-O2 -Wall'
env['CPPPATH']=['#include/']
env.SetOption("num_jobs",2);

Export('env')

objs=SConscript("src/SConscript")
env.SharedLibrary("escm",objs)


