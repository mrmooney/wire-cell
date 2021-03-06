#!/usr/bin/env python

TOP = '.'
APPNAME = 'WireCell'

# the sequence matters
subdirs = ['data',
           'nav',
           'signal',
           'sst',
           'tiling',
           'rootvis',
           'graph',
           'examples',
           'matrix', 
           '2dtoy',
           'lsp',
]

def options(opt):
    opt.load('doxygen', tooldir='waf-tools')
    opt.load('find_package', tooldir='waf-tools')
    opt.add_option('--build-debug', default='-O2',
                   help="Build with debug symbols")
    opt.add_option('--doxygen-tarball', default=None,
                   help="Build Doxygen documentation to a tarball")

def configure(cfg):
    cfg.env.append_unique('CXXFLAGS',['--std=c++11'])
    cfg.load( "compiler_cxx" )
    cfg.env.LIBPATH_XDATA = [cfg.env['PREFIX'] + '/lib']
    cfg.env.LIBPATH_XDATA += [cfg.env['PREFIX'] + '/lib64']
    cfg.env.INCLUDES_XDATA  = [cfg.env['PREFIX'] + '/include']
    print cfg.env.LIBPATH_XDATA
    print cfg.env.INCLUDES_XDATA

    cfg.load('doxygen', tooldir='waf-tools')
    cfg.load('find_package', tooldir='waf-tools')
    cfg.env.CXXFLAGS += [cfg.options.build_debug]
    cfg.check_boost(lib='system filesystem graph')

    cfg.check_cxx(lib = "WireCellXdataRoot",
                  header_name="WireCellXdataRoot/Wire.h",
                  use='XDATA ROOTSYS', uselib_store='XDATA')

def build(bld):
    bld.load('find_package', tooldir='waf-tools')
    bld.recurse(subdirs)
    if bld.env.DOXYGEN and bld.options.doxygen_tarball:
        bld(features="doxygen", doxyfile=bld.path.find_resource('Doxyfile'),
            doxy_tar = bld.options.doxygen_tarball)

