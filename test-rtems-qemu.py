#!/usr/bin/env python3
"""
Run RTEMS test harness(es) with QEMU

  ./test-rtems-qemu.py \
   ./src/libCom/test/O.RTEMS-pc386/testspec \
   ./src/ioc/db/test/O.RTEMS-pc386/testspec \
   ./src/std/filters/test/O.RTEMS-pc386/testspec \
   ./src/std/rec/test/O.RTEMS-pc386/testspec

When planning to run tests, build Base with

make RTEMS_QEMU_FIXUPS=YES
"""

import logging
_log = logging.getLogger(__name__)

import os, sys, signal, re
from subprocess import PIPE, STDOUT, DEVNULL, TimeoutExpired, Popen
from shutil import copyfile, which
from tempfile import TemporaryDirectory

def getargs():
    import argparse
    P = argparse.ArgumentParser(description='Run testspec(s) for an RTEMS target')
    P.add_argument('specs', metavar='testspec', nargs='+')
    P.add_argument('--arch', metavar='ARCH', default='i386',
                   help='QEMU arch. name (default "i386")')
    P.add_argument('-t','--timeout', metavar='SECS', type=int, default=600,
                   help='subprocess timeout interval')
    P.add_argument('-D','--display', action='store_true',
                   help='Show the VGA console')
    P.add_argument('-n','--dry-run', action='store_true',
                   help="Don't start qemu, just print what would be done")
    P.add_argument('-l','--level', metavar='NAME', default='WARN',
                   type=logging.getLevelName,
                   help="Python log level (default WARN)")

    return P.parse_args()

# Don't forget "-no-reboot" or the first test will spin until timeout
targetinfo = {
    'RTEMS-pc386':{
        'cmd':['qemu-system-i386', '-no-reboot', '-m', '128',
               '-net', 'nic,model=ne2k_pci',
               '-net', 'user,hostname=qemuhost,tftp=%(tftp)s',
               '-serial', 'stdio',
               '-kernel', '%(image)s',
               '-append', '--console=com1', # For pc386, RTEMS >=4.10
              ],
    }
}

def parsespec(spec):
    with open(spec, 'r') as FP:
        # Lines have the form
        #  Key: Value with spaces
        # Split on ':' then strip key and value
        # Return a dictionary of {'Key':'Value ...', ...}
        return dict([map(str.strip,L.split(':',1)) for L in FP.readlines()])

def runspec(specfile, args):
    specdir = os.path.dirname(specfile)
    conf = parsespec(specfile)

    if 'Harness' not in conf or conf.get('OS-class','')!='RTEMS' or conf.get('Target-arch','') not in targetinfo:
        _log.warn('Skipping %s', specfile)
        return

    info = targetinfo[conf['Target-arch']]

    exe = which(info['cmd'][0])
    _log.debug('Emulator %s', exe)
    info['cmd'][0] = exe

    # Harness: dbTestHarness.boot; epicsRunDbTests
    harness = os.path.join(specdir, conf['Harness'].split(';')[0].strip())
    if harness.endswith('.boot'):
        harness = harness[:-5] # QEMU needs the ELF file
    _log.debug('Harness: %s', harness)

    if not os.path.isfile(harness):
        raise RuntimeError('Harness does not exist')

    with TemporaryDirectory() as D:
        params = {'tftp':D, 'image':harness}

        D = os.path.join(D, 'epics', 'qemuhost')
        os.makedirs(D)

        # Copy in TESTFILES
        for F in conf.get('Files','').split():
            src = os.path.join(specdir, F)
            dst = os.path.join(D, os.path.basename(F))
            _log.debug('Copy %s -> %s', src, dst)
            copyfile(src, dst)

        cmd = [I%params for I in info['cmd']]

        if not args.display:
            cmd += ['-display', 'none']

        _log.info('Cmd: %s', cmd)

        if args.dry_run:
            print(' '.join(cmd))
        else:
            if args.timeout and args.timeout>0:
                def timo(*args):
                    _log.error('QEMU Timeout')
                    sys.exit(2)
                signal.signal(signal.SIGALRM, timo)
                signal.alarm(args.timeout)
                _log.info("Start timeout of %f", args.timeout)

            P = Popen(cmd, stdin=DEVNULL, stderr=STDOUT, stdout=PIPE, universal_newlines=True)

            try:
                for L in P.stdout:
                    if not L:
                        break
                    if L[0]=='#':
                        pass
                    # "ok 1 -..."
                    # "ok 1 #..."
                    # "not ok 1 -..."
                    # "not ok 1 #..."
                    elif re.match(r'(?:not\s+)?ok\s+\d+\s+[-#].*', L):
                        pass
                    # "1..14"
                    # "2..24 #..."
                    elif re.match(r'\d+\.\.\d+(?:\s+#.*)?', L):
                        pass
                    elif L.startswith('***** '):
                        pass
                    elif L.startswith('Bail out!'):
                        pass
                    else:
                        _log.info('X %s', L[:-1])
                        continue
                    sys.stdout.write(L)
                    sys.stdout.flush()
                P.wait(10)
                _log.info('Done')
            except:
                if P.returncode is None:
                    try:
                        _log.warn('Force QEMU termination')
                        P.terminate()
                        P.wait(10)
                    except:
                        _log.err('Kill QEMU')
                        P.kill()
                raise

            if args.timeout>0:
                signal.alarm(0)
            return P.returncode

def main(args):
    for S in args.specs:
        _log.info('Test %s', S)
        runspec(S, args)

if __name__=='__main__':
    args = getargs()
    logging.basicConfig(level=args.level,
                        format='%(levelname)s %(message)s')
    main(args)
