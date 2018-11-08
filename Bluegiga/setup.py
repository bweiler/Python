from distutils.core import setup, Extension
#-DPLATFORM_WIN
module1 = Extension('bluetooth',
					  include_dirs = ['/usr/local/include'],
                      sources = ['main.c','cmd_def.c','uart.c','stubs.c'],
                      libraries = ['setupapi'],
                      library_dirs = ['C:\mingw64\lib64'])
setup (name = 'bluetooth',
       version = '1.0',
       description = 'This is a demo package',
       ext_modules = [module1])
	   