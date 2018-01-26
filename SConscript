import rtconfig
from building import *

# get current directory
cwd = GetCurrentDir()

# The set of source files associated with this SConscript file.

src  = [cwd + '/rt-thread/nes_cmd.c']
src += [cwd + '/K6502.cpp']
src += [cwd + '/InfoNES.cpp']
src += [cwd + '/InfoNES_Mapper.cpp']
src += [cwd + '/InfoNES_pAPU.cpp']
src += [cwd + '/rt-thread/InfoNES_System_RTT.cpp']
	 
#add for include file
path  = [cwd]
path += [cwd + '/mapper/']
path += [cwd + '/rt-thread/']

#
CPPDEFINES = ['']

group = DefineGroup('NES', src, depend = ['RT_USING_GUIENGINE'], CPPPATH = path, CPPDEFINES = CPPDEFINES)

Return('group')
