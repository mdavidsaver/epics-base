'''Minimal softIoc
'''

import logging
import importlib
import sys
import pkgutil

from typing import Set, Mapping
from dataclasses import dataclass, field

_log = logging.getLogger(__name__)

def base_load():
    import ctypes
    import code
    import argparse
    import os
    import atexit
    from . import path


    # The libraries we need to run up a soft IOC, don't load dbCore twice
    if path.OS_CLASS == "WIN32":
        Com = ctypes.WinDLL(path.get_lib("Com"), mode=ctypes.RTLD_GLOBAL)
    else:
        Com = ctypes.CDLL(path.get_lib("Com"), mode=ctypes.RTLD_GLOBAL)
    dbCore = ctypes.CDLL(path.get_lib("dbCore"), mode=ctypes.RTLD_GLOBAL)
    dbRecStd = ctypes.CDLL(path.get_lib("dbRecStd"), mode=ctypes.RTLD_GLOBAL)
    pvAccessIOC = ctypes.CDLL(path.get_lib("pvAccessIOC"), mode=ctypes.RTLD_GLOBAL)
    qsrv = ctypes.CDLL(path.get_lib("qsrv"), mode=ctypes.RTLD_GLOBAL)

    # The functions we need from those libraries

    epicsExitCallAtExits = Com.epicsExitCallAtExits
    epicsExitCallAtExits.restype = None

    @atexit.register
    def baseCleanup():
        epicsExitCallAtExits()

    iocshCmd = Com.iocshCmd
    iocshCmd.restype = ctypes.c_int
    iocshCmd.argtypes = [ctypes.c_char_p]

    iocshRegisterCommon = dbCore.iocshRegisterCommon
    iocshRegisterCommon.restype = None
    iocshRegisterCommon.argtypes = []

    # dbLoadDatabase(file, path, subs)
    dbLoadDatabase = dbCore.dbLoadDatabase
    dbLoadDatabase.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p]

    # pdbbase is valid after the DBD is loaded
    pdbbase = ctypes.c_void_p.in_dll(dbCore, "pdbbase")

    # softIoc_registerRecordDeviceDriver(pdbbase)
    #registerRecordDeviceDriver = dbRecStd.softIoc_registerRecordDeviceDriver
    registerRecordDeviceDriver = dbCore.registerAllRecordDeviceDrivers
    registerRecordDeviceDriver.argtypes = [ctypes.c_void_p]

    # dbLoadRecords(file, subs)
    dbLoadRecords = dbCore.dbLoadRecords
    dbLoadRecords.argtypes = [ctypes.c_char_p, ctypes.c_char_p]

    # iocInit()
    iocInit = dbCore.iocInit

    def ioc(cmd):
        '''Execute IOC shell command
        '''
        return iocshCmd(cmd.encode())

    def start_ioc(database=None, macros='', dbs=None):
        if dbs is None:
            dbs = []
        if database is not None:
            dbs += [(database, macros)]

        def out(msg, *args):
            sys.stderr.write(msg%args)
            sys.stderr.flush()

        iocshRegisterCommon()

        out('IOC Starting w/ %s \n', dbs)
        dbdpath = os.path.join(path.base_path, "dbd")
        for dbd in [b'base.dbd', b'PVAServerRegister.dbd', b'qsrv.dbd']:
            if dbLoadDatabase(dbd, dbdpath.encode(), None):
                raise RuntimeError('Error loading '+dbdpath)

        out('IOC dbd loaded\n')
        if registerRecordDeviceDriver(pdbbase):
            raise RuntimeError('Error registering')

        out('IOC rRDD\n')
        for database, macros in dbs:
            if dbLoadRecords(database.encode(), macros.encode()):
                raise RuntimeError('Error loading %s with %s'%(database, macros))
            out('IOC db loaded\n')
        if iocInit():
            raise RuntimeError('Error starting IOC')
        out('IOC Running\n')


    def main():
        class DbAction(argparse.Action):
            def __call__(self, parser, ns, values, opt):
                ns.database.append((values, ns.macros))
        parser = argparse.ArgumentParser(
            description="Run a cut down softIoc by calling the functions from Python")
        parser.add_argument('-m', '--macros', default="",
            help="Macro definitions when expanding database, e.g. -d \"macro=value,macro2=value2\"")
        parser.add_argument('-d', '--database', default=[], action=DbAction,
            help="Path to database file to load")
        args = parser.parse_args()
        start_ioc(dbs=args.database)
        code.interact(local={
            'exit':sys.exit,
            'ioc':ioc,
        })

@dataclass
class Plugin:
    name: str = field(init=False)
    path: str = field(repr=False)
    module: object = field(repr=False, default=None)
    requires: Set[str] = field(default_factory=set)
    wants: Set[str] = field(default_factory=set)
    conflicts: Set[str] = field(default_factory=set)
    before: Set[str] = field(default_factory=set)
    after: Set[str] = field(default_factory=set)

    def __post_init__(self):
        self.name = name = self.path
        sep = name.rfind('.')
        if sep!=-1:
            self.name = name[sep+1:]

def _build_dep_tree(root:Plugin, Ps: {str:Plugin}):
    todo, done = [root], set()

    while len(todo):
        P = todo.pop(0)
        done.add(P)
        # resolve lists of names to list of Plugin
        for l, req in [('requires', True), ('wants', False), ('conflicts', False)]:
            res = set()
            for name in getattr(P, l):
                try:
                    dep = Ps[name]
                except KeyError:
                    if req:
                        raise RuntimeError(f'{P.name} missing required {name}')
                else:
                    if dep not in done:
                        todo.append(dep)
                    res.add(dep)
            setattr(P, l, res)

def main(args=None):
    from argparse import ArgumentParser
    P = ArgumentParser(allow_abbrev=False)
    P.add_argument('-R', '--require', action='append', default=['base'])
    P.add_argument('-W', '--want', action='append', default=[])
    P.add_argument('--omit', action='append', default=[])
    P.add_argument('--list-plugins', action='store_true')

    args, rem = P.parse_known_args(args=args)

    logging.basicConfig(level=logging.DEBUG)

    from . import plugin

    Ps = []
    for _finder, name, _ispkg in pkgutil.iter_modules(plugin.__path__, plugin.__name__+'.'):
        try:
            mod = importlib.import_module(name)
            P = mod.module_info()
            P.module = mod
            # TODO: validate info
            Ps.append(P)
            del mod, P
        except:
            _log.exception("Unable to load IOC plugin %s", name)

    if args.list_plugins:
        Ps.sort(key=lambda P:P.path)
        for P in Ps:
            print(P)
        sys.exit(0)

    Ps = {P.name:P for P in Ps}

    # pseudo plugin to root the dependency tree
    root = Plugin(
        path=__name__,
        require=args.require,
        optional=args.optional,
        conflict=args.conflict,
    )

    _build_dep_tree(root, Ps)
