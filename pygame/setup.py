#!/usr/bin/env python
#
# This is the distutils setup script for pygame.
# Full instructions are in "install.txt" or "install.html"
#
# To configure, compile, install, just run this script.

from __future__ import with_statement

DESCRIPTION = """Pygame is a Python wrapper module for the
SDL multimedia library. It contains python functions and classes
that will allow you to use SDL's support for playing cdroms,
audio and video output, and keyboard, mouse and joystick input."""

EXTRAS = {}

METADATA = {
    "name":             "pygame",
    "version":          "1.9.2a0",
    "license":          "LGPL",
    "url":              "http://www.pygame.org",
    "author":           "Pete Shinners, Rene Dudfield, Marcus von Appen, Bob Pendleton, others...",
    "author_email":     "pygame@seul.org",
    "description":      "Python Game Development",
    "long_description": DESCRIPTION,
}

import sys

if "bdist_msi" in sys.argv:
    # hack the version name to a format msi doesn't have trouble with
    METADATA["version"] = METADATA["version"].replace("pre", "a0")
    METADATA["version"] = METADATA["version"].replace("rc", "b0")
    METADATA["version"] = METADATA["version"].replace("release", "")
    

if not hasattr(sys, 'version_info') or sys.version_info < (2,3):
    raise SystemExit("Pygame requires Python version 2.3 or above.")

#get us to the correct directory
import os, sys
path = os.path.split(os.path.abspath(sys.argv[0]))[0]
os.chdir(path)
#os.environ["CFLAGS"] = "-W -Wall -Wpointer-arith -Wcast-qual -Winline " + \
#                       "-Wcast-align -Wconversion -Wstrict-prototypes " + \
#                       "-Wmissing-prototypes -Wmissing-declarations " + \
#                       "-Wnested-externs -Wshadow -Wredundant-decls"
if "-warnings" in sys.argv:
    os.environ["CFLAGS"] = "-W -Wimplicit-int " + \
                       "-Wimplicit-function-declaration " + \
                       "-Wimplicit -Wmain -Wreturn-type -Wunused -Wswitch " + \
                       "-Wcomment -Wtrigraphs -Wformat -Wchar-subscripts " + \
                       "-Wuninitialized -Wparentheses " +\
                       "-Wpointer-arith -Wcast-qual -Winline -Wcast-align " + \
                       "-Wconversion -Wstrict-prototypes " + \
                       "-Wmissing-prototypes -Wmissing-declarations " + \
                       "-Wnested-externs -Wshadow -Wredundant-decls"
    sys.argv.remove ("-warnings")

import os.path, glob, stat, shutil
import distutils.sysconfig
from distutils.core import setup, Extension, Command
from distutils.extension import read_setup_file
from distutils.command.install_data import install_data

# Retrieve the repository revision (HG node identifier), if possible.
def get_hg_identifier():
    from subprocess import Popen, PIPE

    ident = ""
    try:
        p = Popen(['hg', 'identify', '-i'], stdout=PIPE, stderr=PIPE)
        stdout, stderr = p.communicate()
        if stdout and not stderr:
            ident = stdout.decode('ascii').rstrip('\n')
    except Exception:
        pass
    return ident

revision = get_hg_identifier()

# Python 3.0 patch
if sys.version_info[0:2] == (3, 0):
    import distutils.version
    def _cmp(x, y):
        try:
            if x < y:
                return -1
            elif x == y:
                return 0
            return 1
        except TypeError:
            return NotImplemented
    distutils.version.cmp = _cmp
    del _cmp

def add_datafiles(data_files, dest_dir, pattern):
    """Add directory structures to data files according to a pattern"""
    src_dir, elements = pattern
    def do_directory(root_dest_path, root_src_path, elements):
        files = []
        for e in elements:
            if isinstance(e, list):
                src_dir, elems = e
                dest_path = '/'.join([root_dest_path, src_dir])
                src_path = os.path.join(root_src_path, src_dir)
                do_directory(dest_path, src_path, elems)
            else:
                files.extend(glob.glob(os.path.join(root_src_path, e)))
        if files:
            data_files.append((root_dest_path, files))
    do_directory(dest_dir, src_dir, elements)

# allow optionally using setuptools for bdist_egg.
if "-setuptools" in sys.argv:
    from setuptools import setup, find_packages
    sys.argv.remove ("-setuptools")


# NOTE: the bdist_mpkg_support is for darwin.
try:
    import bdist_mpkg_support
    from setuptools import setup, Extension
except ImportError:
    pass
else:
    EXTRAS.update({
        'options': bdist_mpkg_support.options,
        'setup_requires': ['bdist_mpkg>=0.4.2'],
        #'install_requires': ['pyobjc'],
        #'dependency_links': ['http://rene.f0o.com/~rene/stuff/macosx/']
    })

#headers to install
headers = glob.glob(os.path.join('src', '*.h'))
headers.remove(os.path.join('src', 'numeric_arrayobject.h'))
headers.remove(os.path.join('src', 'scale.h'))

# option for not installing the headers.
if "-noheaders" in sys.argv:
    headers = []
    sys.argv.remove ("-noheaders")


#sanity check for any arguments
if len(sys.argv) == 1:
    if sys.version_info[0] >= 3:
        reply = input('\nNo Arguments Given, Perform Default Install? [Y/n]')
    else:
        reply = raw_input('\nNo Arguments Given, Perform Default Install? [Y/n]')
    if not reply or reply[0].lower() != 'n':
        sys.argv.append('install')


#make sure there is a Setup file
if not os.path.isfile('Setup'):
    print ('\n\nWARNING, No "Setup" File Exists, Running "config.py"')
    import config
    config.main()
    print ('\nContinuing With "setup.py"')


try:
    s_mtime = os.stat("Setup")[stat.ST_MTIME]
    sin_mtime = os.stat("Setup.in")[stat.ST_MTIME]
    if sin_mtime > s_mtime:
        print ('\n\nWARNING, "Setup.in" newer than "Setup",'
               'you might need to modify "Setup".')
except:
    pass

# get compile info for all extensions
try: 
    extensions = read_setup_file('Setup')
except: 
    print ("""Error with the "Setup" file,
perhaps make a clean copy from "Setup.in".""")
    raise

#python 3.x: remove modules not yet ported
if sys.version_info >= (3, 0, 0):
    python3_skip = ['_movie',
                    '_numericsurfarray',
                    '_numericsndarray',
                   ]
#    if (sys.platform != 'linux2'):
#        python3_skip.append('scrap')
    tmp_extensions = extensions
    extensions = []
    for e in tmp_extensions:
        if e.name in python3_skip:
            print ("Skipping module %s for Python %s build." %
                   (e.name, sys.version))
        else:
            extensions.append(e)
    del tmp_extensions

#decide whether or not to enable new buffer protocol support
enable_newbuf = False
if sys.version_info >= (2, 6, 0):
    try:
        sys.pypy_version_info
    except AttributeError:
        enable_newbuf = True

if enable_newbuf:
    enable_newbuf_value = '1'
else:
    enable_newbuf_value = '0'
for e in extensions:
    e.define_macros.append(('ENABLE_NEWBUF', enable_newbuf_value))

#if new buffer protocol support is disabled then remove the testing framework
if not enable_newbuf:
    posn = None
    for i, e in enumerate(extensions):
        if e.name == 'newbuffer':
            posn = i
    if (posn is not None):
        del extensions[posn]

# if not building font, try replacing with ftfont
alternate_font = os.path.join('lib', 'font.py')
if os.path.exists(alternate_font):
    os.remove(alternate_font)
have_font = False
have_freetype = False
for e in extensions:
    if e.name == 'font':
        have_font = True
    if e.name == '_freetype':
        have_freetype = True
if not have_font and have_freetype:
    shutil.copyfile(os.path.join('lib', 'ftfont.py'), alternate_font)

#extra files to install
data_path = os.path.join(distutils.sysconfig.get_python_lib(), 'pygame')
pygame_data_files = []
data_files = [('pygame', pygame_data_files)]

#add files in distribution directory
pygame_data_files.append('LGPL')
pygame_data_files.append('readme.html')
pygame_data_files.append('install.html')

#add non .py files in lib directory
for f in glob.glob(os.path.join('lib', '*')):
    if not f[-3:] == '.py' and not f[-4:] == '.doc' and os.path.isfile(f):
        pygame_data_files.append(f)

#tests/fixtures
add_datafiles(data_files, 'pygame/tests',
              ['test',
                  [['fixtures',
                      [['xbm_cursors',
                          ['*.xbm']],
                       ['fonts',
                          ['*.ttf', '*.otf', '*.bdf', '*.png']]]]]])

#examples
add_datafiles(data_files, 'pygame/examples',
              ['examples',
                  ['readme.rst',
                   ['data',
                       ['*']],
                   ['macosx',
                       ['*.py',
                        ['aliens_app_example',
                            ['*.py',
                             'README.txt',
                             ['English.lproj',
                                 ['aliens.icns',
                                  ['MainMenu.nib',
                                      ['*']]]]]]]]]])

#docs
add_datafiles(data_files, 'pygame/docs',
              ['docs',
                  ['*.html',             # Navigation and help pages
                   '*.gif',              # Pygame logos
                   '*.js',               # For doc search
                   ['ref',               # Pygame reference
                       ['*.html',        # Reference pages
                        '*.js',          # Comments script
                        '*.json']],      # Comment data
                   ['tut',               # Tutorials
                       ['*.html',
                        ['chimp',
                            ['*.html',
                             '*.gif']],
                        ['intro',
                            ['*.html',
                             '*.gif',
                             '*.jpg']],
                        ['surfarray',
                            ['*.html',
                             '*.jpg']],
                        ['tom',
                            ['*.html',
                             '*.png']],
                        ['camera',
                            ['*.html',
                             '*.jpg']]]],
                   ['_static',            # Sphinx added support files
                        ['*.css',
                         '*.png',
                         '*.ico',
                         '*.js']],
                   ['_sources',           # Used for ref search
                        ['*.txt',
                         ['ref',
                            ['*.txt']]]]]])

#generate the version module
def parse_version(ver):
    from re import findall
    return ', '.join(s for s in findall('\d+', ver)[0:3])

def write_version_module(pygame_version, revision):
    vernum = parse_version(pygame_version)
    with open('version.py.in', 'r') as header_file:
        header = header_file.read()
    with open(os.path.join('lib', 'version.py'), 'w') as version_file:
        version_file.write(header)
        version_file.write('ver = "' + pygame_version + '"\n')
        version_file.write('vernum = ' + vernum + '\n')
        version_file.write('rev = "' + revision + '"\n')

write_version_module(METADATA['version'], revision)
              
#required. This will be filled if doing a Windows build.
cmdclass = {}

#try to find DLLs and copy them too  (only on windows)
if sys.platform == 'win32':

    from distutils.command.build_ext import build_ext
    # mingw32distutils is optional. But we need the mingw32 compiler(s).
    try:
        # Allow the choice between Win32 GUI and console DLLs.
        import mingw32distutils
    except ImportError:
        mingw32_compilers = ['ming32']
    else:
        mingw32_compilers = mingw32distutils.compilers
    if sys.version_info < (2, 4):
        try:
            import config
            # a separate method for finding dlls with mingw.
            if config.is_msys_mingw():

                # fix up the paths for msys compiling.
                import distutils_mods
                distutils.cygwinccompiler.Mingw32 = distutils_mods.mingcomp
        except ImportError:
            pass
        
    #add dependency DLLs to the project
    lib_dependencies = {}
    for e in extensions:
        if e.name.startswith('COPYLIB_'):
            lib_dependencies[e.name[8:]] = e.libraries

    def dependencies(roots):
        """Return a set of dependencies for the list of library file roots
        
        The return set is a dictionary keyed on library root name with values of 1.
        """

        root_set = {}
        for root in roots:
            try:
                deps = lib_dependencies[root]
            except KeyError:
                pass
            else:
                root_set[root] = 1
                root_set.update(dependencies(deps))
        return root_set

    the_dlls = {}
    required_dlls = {}
    for e in extensions:
        if e.name.startswith('COPYLIB_'):
            the_dlls[e.name[8:]] = e.library_dirs[0]
        else:
            required_dlls.update(dependencies(e.libraries))

    # join the required_dlls and the_dlls keys together.
    lib_names = {}
    for lib in list(required_dlls.keys()) + list(the_dlls.keys()):
        lib_names[lib] = 1

    for lib in lib_names.keys():
        #next DLL; a distutils bug requires the paths to have Windows separators
        f = the_dlls[lib].replace('/', os.sep)
        if f == '_':
            print ("WARNING, DLL for %s library not found." % lib)
        else:
            pygame_data_files.append(f)

    class WinBuildExt(build_ext):
        """This build_ext sets necessary environment variables for MinGW"""

        # __sdl_lib_dir is possible location of msvcrt replacement import
        # libraries, if they exist. Pygame module base only links to SDL so
        # should have the SDL library directory as its only -L option.
        for e in extensions:
            if e.name == 'base':
                __sdl_lib_dir = e.library_dirs[0].replace('/', os.sep)
                break
        
        def run(self):
            """Extended to set MINGW_ROOT_DIRECTORY, PATH and LIBRARY_PATH"""
            
            if self.compiler in mingw32_compilers:
                # Add MinGW environment variables.
                if 'MINGW_ROOT_DIRECTORY' not in os.environ:
                    # Use MinGW setup conifiguration file if present.
                    import mingwcfg
                    try:
                        mingw_root = mingwcfg.read()
                    except IOError:
                        raise RuntimeError(
                            "mingw32: required environment variable"
                            " MINGW_ROOT_DIRECTORY not set")
                    os.environ['MINGW_ROOT_DIRECTORY'] = mingw_root
                    path = os.environ['PATH']
                    os.environ['PATH'] = ';'.join([os.path.join(mingw_root, 'bin'),
                                                   path])
                if sys.version_info >= (2, 6):
                    # The Visual Studio 2008 C library is msvcr90.dll.
                    c_runtime_path = os.path.join(self.__sdl_lib_dir, 'msvcr90')
                elif sys.version_info >= (2, 4):
                    # The Visual Studio 2003 C library is msvcr71.dll.
                    c_runtime_path = os.path.join(self.__sdl_lib_dir, 'msvcr71')
                else:
                    # The Visual Studio 6.0 C library is msvcrt.dll,
                    # the MinGW default.
                    c_runtime_path = ''
                if c_runtime_path and os.path.isdir(c_runtime_path):
                    # Override the default msvcrt.dll linkage.
                    os.environ['LIBRARY_PATH'] = c_runtime_path
                elif not (c_runtime_path or
                          glob.glob(os.path.join(self.__sdl_lib_dir,
                                                 'msvcr*'))):
                    pass
                else:
                    raise RuntimeError("The dependencies are linked to"
                                       " the wrong C runtime for"
                                       " Python %i.%i" %
                                       sys.version_info[:2])
            build_ext.run(self)
    cmdclass['build_ext'] = WinBuildExt

    # Add the precompiled smooth scale MMX functions to transform.
    def replace_scale_mmx():
        for e in extensions:
            if e.name == 'transform':
                if '64 bit' in sys.version:
                    e.extra_objects.append(
                        os.path.join('obj', 'win64', 'scale_mmx.obj'))
                else:
                    e.extra_objects.append(
                        os.path.join('obj', 'win32', 'scale_mmx.obj'))
                for i in range(len(e.sources)):
                    if e.sources[i].endswith('scale_mmx.c'):
                        del e.sources[i]
                        return
    replace_scale_mmx()


#clean up the list of extensions
for e in extensions[:]:
    if e.name.startswith('COPYLIB_'):
        extensions.remove(e) #don't compile the COPYLIBs, just clean them
    else:
        e.name = 'pygame.' + e.name #prepend package name on modules


#data installer with improved intelligence over distutils
#data files are copied into the project directory instead
#of willy-nilly
class smart_install_data(install_data):
    def run(self):
        #need to change self.install_dir to the actual library dir
        install_cmd = self.get_finalized_command('install')
        self.install_dir = getattr(install_cmd, 'install_lib')
        return install_data.run(self)

cmdclass['install_data'] = smart_install_data

            
if "bdist_msi" in sys.argv:
    # if you are making an msi, we want it to overwrite files
    # we also want to include the repository revision in the file name
    from distutils.command import bdist_msi
    import msilib

    class bdist_msi_overwrite_on_install(bdist_msi.bdist_msi):
        def run(self):
            bdist_msi.bdist_msi.run(self)

            # Remove obsolete files.
            comp = "pygame1"  # Pygame component
            prop = comp   # Directory property
            records = [("surfarray.pyd", comp,
                        "SURFAR~1.PYD|surfarray.pyd", prop, 1),
                       ("sndarray.pyd", comp,
                        "SNDARRAY.PYD|sndarray.pyd", prop, 1),
                       ("camera.pyd", comp, "CAMERA.PYD|camera.pyd", prop, 1),
                       ("color.py", comp, "COLOR.PY|color.py", prop, 1),
                       ("color.pyc", comp, "COLOR.PYC|color.pyc", prop, 1),
                       ("color.pyo", comp, "COLOR.PYO|color.pyo", prop, 1)]
            msilib.add_data(self.db, "RemoveFile", records)

            # Overwrite outdated files.
            fullname = self.distribution.get_fullname()
            installer_name = self.get_installer_filename(fullname)           
            print ("changing %s to overwrite files on install" % installer_name)
            msilib.add_data(self.db, "Property", [("REINSTALLMODE", "amus")])
            self.db.Commit()
    
        def get_installer_filename(self, fullname):
            if revision:
                fullname += '-hg_' + revision
            return bdist_msi.bdist_msi.get_installer_filename(self, fullname)

    cmdclass['bdist_msi'] = bdist_msi_overwrite_on_install







# test command.  For doing 'python setup.py test'

class TestCommand(Command):
    user_options = [ ]

    def initialize_options(self):
        self._dir = os.getcwd()

    def finalize_options(self):
        pass

    def run(self):
        '''
        runs the tests with default options.
        '''
        import subprocess
        return subprocess.call([sys.executable, "run_tests.py"])

cmdclass['test'] = TestCommand

# Prune empty file lists.
date_files = [(path, files) for path, files in data_files if files]










#finally,
#call distutils with all needed info
PACKAGEDATA = {
       "cmdclass":    cmdclass,
       "packages":    ['pygame', 'pygame.gp2x', 'pygame.threads',
                       'pygame.tests',
                       'pygame.tests.test_utils',
                       'pygame.tests.run_tests__tests',
                       'pygame.tests.run_tests__tests.all_ok',
                       'pygame.tests.run_tests__tests.failures1',
                       'pygame.tests.run_tests__tests.incomplete',
                       'pygame.tests.run_tests__tests.infinite_loop',
                       'pygame.tests.run_tests__tests.print_stderr',
                       'pygame.tests.run_tests__tests.print_stdout',
                       'pygame.tests.run_tests__tests.incomplete_todo',
                       'pygame.tests.run_tests__tests.exclude',
                       'pygame.tests.run_tests__tests.timeout',
                       'pygame.tests.run_tests__tests.everything',
                       'pygame.docs',
                       'pygame.examples'],
       "package_dir": {'pygame': 'lib',
                       'pygame.threads': 'lib/threads',
                       'pygame.gp2x': 'lib/gp2x',
                       'pygame.tests': 'test',
                       'pygame.docs': 'docs',
                       'pygame.examples': 'examples'},
       "headers":     headers,
       "ext_modules": extensions,
       "data_files":  data_files,
}
PACKAGEDATA.update(METADATA)
PACKAGEDATA.update(EXTRAS)
setup(**PACKAGEDATA)


def remove_old_files():

    # try and figure out where we are installed.

    #pygame could be installed in a weird location because of 
    #  setuptools or something else.  The only sane way seems to be by trying
    #  first to import it, and see where the imported one is.
    # 
    # Otherwise we might delete some files from another installation.
    try:
        import pygame.base
        use_pygame = 1
    except:
        use_pygame = 0

    if use_pygame:
        install_path= os.path.split(pygame.base.__file__)[0]
        extension_ext = os.path.splitext(pygame.base.__file__)[1]
    else:
        if not os.path.exists(data_path):
            return

        install_path = data_path

        base_file = glob.glob(os.path.join(data_path, "base*"))
        if not base_file:
            return
        
        extension_ext = os.path.splitext(base_file[0])[1]



    # here are the .so/.pyd files we need to ask to remove.
    ext_to_remove = ["camera"]

    # here are the .py/.pyo/.pyc files we need to ask to remove.
    py_to_remove = ["color"]

    os.path.join(data_path, 'color.py')
    if os.name == "e32": # Don't warn on Symbian. The color.py is used as a wrapper.
        py_to_remove = []



    # See if any of the files are there.
    extension_files = ["%s%s" % (x, extension_ext) for x in ext_to_remove]

    py_files = ["%s%s" % (x, py_ext)
                for py_ext in [".py", ".pyc", ".pyo"]
                for x in py_to_remove]

    files = py_files + extension_files

    unwanted_files = []
    for f in files:
        unwanted_files.append( os.path.join( install_path, f ) )



    ask_remove = []
    for f in unwanted_files:
        if os.path.exists(f):
            ask_remove.append(f)

    for f in ask_remove:
        try:
            print("trying to remove old file :%s: ..." %f)
            os.remove(f)
            print("Successfully removed :%s:." % f)
        except:
            print("FAILED to remove old file :%s:" % f)



if "install" in sys.argv:
    # remove some old files.
    # only call after a successful install.  Should only reach here if there is
    #   a successful install... otherwise setup() raises an error.
    try:
        remove_old_files()
    except:
        pass



