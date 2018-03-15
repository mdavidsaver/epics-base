#!/usr/bin/env python

from __future__ import print_function

import logging
_log = logging.getLogger(__name__)

import os

def getargs():
    from argparse import ArgumentParser
    P = ArgumentParser()
    P.add_argument('-T', '--top', default=os.getcwd(), help='$(TOP)')
    P.add_argument('-a', '--target', default='invalid', help='$(T_A)')
    P.add_argument('-o', '--os', help='$(OS_CLASS)')
    P.add_argument('-t', '--type', default='lib', help='lib or bin')
    P.add_argument('-v', '--verbose', default=logging.INFO, action='store_const', const=logging.DEBUG)
    P.add_argument('-e', '--escape', action='store_true', help='Escape $ in $ORIGIN')
    P.add_argument('path', nargs='*')
    return P.parse_args()

class HandleELF(object):
    def __init__(self, args):
        self.origin = '\$ORIGIN' if args.escape else '$ORIGIN'
        self.top = args.top
        self.srcdir = args.srcdir
    def __call__(self, path):
        path = os.path.normpath(path)
        _log.debug('Consider "%s"', path)
        # pass through paths relative to build dir '.' and '..'
        if not os.path.isabs(path):
            _log.debug(' Relative')
        else:
            pref = os.path.commonprefix([self.top, path])
            # ignore if no common prefix (eg. /usr and /opt, or /usr/lib and /usr/local)
            if pref in ('', '/', '/usr'):
                _log.debug(' No common path prefix')
            else:
                path = os.path.relpath(path, self.srcdir)
                path = os.path.join(self.origin, path)
                _log.debug(' Mangle as "%s"', path)

        return path

class HandleMachO(HandleELF):
    def __init__(self, args):
        HandleELF.__init__(self, args)
        self.origin = '@loader_path'
        # cf.
        # https://developer.apple.com/library/content/documentation/DeveloperTools/Conceptual/DynamicLibraries/100-Articles/RunpathDependentLibraries.html
        # also 'man dyld'

def main(args):
    args.top = os.path.normpath(os.path.join(os.getcwd(), args.top))
    assert os.path.isabs(args.top), args.top

    # absolute path of loader (exec or lib containing these RPATH entries)
    args.srcdir = os.path.join(args.top, args.type, args.target) # eg. '/blah/module/bin/linux-x86'
    _log.debug('From "%s"', args.srcdir)

    if args.os in ('Linux', 'freebsd', 'solaris'):
        fn = HandleELF(args)
    elif args.os in ('Darwin', 'iOS'):
        fn = HandleMachO(args)
    else:
        fn = lambda x:x
        _log.debug("Unhandled OS_CLASS=%s", args.os)

    print(' '.join(map(fn, args.path)))

if __name__=='__main__':
    args = getargs()
    logging.basicConfig(level=args.verbose)
    main(args)
