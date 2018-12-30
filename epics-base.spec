%global epics_prefix %{_prefix}/lib/epics

Name: epics-base
Version: 7.0.2
Release: 1%{?dist}
URL: https://epics.anl.gov/
Summary: The Experimental Physics and Industrial Control Systems
License: EPICS Open License
Packager: Michael Davidsaver <mdavidsaver@gmail.com>
Source0: %{name}-%{version}.tar.gz

BuildRequires: gcc-c++ readline-devel ncurses-devel perl-devel

Group: System Environment/Libraries
Summary: EPICS Base libraries for clients and servers
%description
The Experimental Physics and Industrial Control System is a collection of
tools, libraries and applications for creating a distributed soft real-time
control systems.

This package contains host system shared libraries for clients and IOCs.

%package devel
Requires: epics-base%{?_isa} == %{version}
Group: Development/Libraries
Summary: Files needed to develop new EPICS applications
# some perl modules are missing a package declaration
Provides: perl(EPICS::Release) perl(EPICS::Copy)
%description devel
The Experimental Physics and Industrial Control System is a collection of
tools, libraries and applications for creating a distributed soft real-time
control systems.

Libraries, headers, and utilities needed to develop applications
targeted to the host system.

%package static
Requires: epics-base-devel%{?_isa} == %{version}
Group: Development/Libraries
Summary: Static libraries of EPICS
%description static
The Experimental Physics and Industrial Control System is a collection of
tools, libraries and applications for creating a distributed soft real-time
control systems.

Static version of EPICS libraries

%package util
Requires: epics-base%{?_isa} == %{version}
Group: Applications/System
Summary: EPICS CLI utilities including caget
%description util
The Experimental Physics and Industrial Control System is a collection of
tools, libraries and applications for creating a distributed soft real-time
control systems.

EPICS utilities such as caget and caput as well as the caRepeater daemon.

%prep
%autosetup

%build
# the epics makefiles don't have seperate build and install phase.

%install
# don't actually need to export $EPICS_HOST_ARCH, but do so to ensure consistency
export EPICS_HOST_ARCH=`%{_builddir}/%{?buildsubdir}/startup/EpicsHostArch`
# we will disable -rpath, but need code generators (antelope/flex/msi) to work
# during the build.
export LD_LIBRARY_PATH=%{buildroot}%{epics_prefix}/lib/${EPICS_HOST_ARCH}

# find-debuginfo.sh needs binaries under %{buildroot} to be writable.
# /usr/lib/rpm/fileattrs/elf.attr requires that that all ELF files be executable,
# even shared libraries which don't otherwise need to be.
# (debug auto dep. generation with semi-documented 'rpmbuild –rpmfcdebug')
make -C "%{_builddir}/%{?buildsubdir}" %{?_smp_mflags} \
LINKER_USE_RPATH=NO \
SHRLIB_VERSION=%{version} \
INSTALL_LOCATION="%{buildroot}%{epics_prefix}" \
FINAL_LOCATION=%{epics_prefix} \
BIN_PERMISSIONS=755 \
LIB_PERMISSIONS=644 \
SHRLIB_PERMISSIONS=755

# inject our prefix in case it is different from the patched default
sed -i -e 's|/usr/lib/epics|%{epics_prefix}|g' %{buildroot}%{epics_prefix}/bin/*/makeBase*

# copy runtime libraries (w/ SONAME) into default runtime linker path.
# leave buildtime librarys (w/o SONAME) under epics_prefix.
install -d %{buildroot}%{_libdir}
for lib in Com ca dbCore dbRecStd
do
  mv %{buildroot}%{epics_prefix}/lib/${EPICS_HOST_ARCH}/lib${lib}.so.* %{buildroot}%{_libdir}/
  # adjust symlink
  rm %{buildroot}%{epics_prefix}/lib/${EPICS_HOST_ARCH}/lib${lib}.so
  ln -sr %{buildroot}%{_libdir}/lib${lib}.so.* %{buildroot}%{epics_prefix}/lib/${EPICS_HOST_ARCH}/lib${lib}.so
done

# copy selected executables into default PATH
install -d %{buildroot}%{_bindir}
for exe in caget caput camonitor cainfo caRepeater softIoc
do
  mv %{buildroot}%{epics_prefix}/bin/${EPICS_HOST_ARCH}/${exe} %{buildroot}%{_bindir}/${exe}
done

# inject trampolines for perl scripts into default PATH (w/o .pl suffix)
for exe in makeBaseApp.pl makeBaseExt.pl
do
  cat <<EOF > %{buildroot}%{_bindir}/${exe%%.pl}
#!/bin/sh
set -e
exec %{epics_prefix}/bin/${EPICS_HOST_ARCH}/${exe} "\$@"
EOF
  chmod +x %{buildroot}%{_bindir}/${exe%%.pl}
done

# not bothering with the CA perl bindings
rm %{buildroot}%{epics_prefix}/lib/perl/CA.pm
rm -r %{buildroot}%{epics_prefix}/lib/perl/5.*
# or things which use them
rm %{buildroot}%{epics_prefix}/bin/*/capr.pl
rm -r %{buildroot}%{epics_prefix}/templates/makeBaseApp/top/caPerlApp

# %{_builddir} is included in several generated files
# in inconsquential places.  eg. bldTop in softIoc_registerRecordDeviceDriver.cpp
# rather than patching/stripping this all out, disable the check.
# Would be nice if this were more granular...
export QA_SKIP_BUILD_ROOT=1

%clean
rm -rf %{buildroot}

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%files
%defattr(-,root,root)

%{_libdir}/lib*.so.*

%files devel
%defattr(-,root,root)

%{_bindir}/softIoc
%{_bindir}/makeBase*

%{epics_prefix}/bin
%{epics_prefix}/lib/*/lib*.so

%{epics_prefix}/lib/perl/EPICS
%{epics_prefix}/lib/perl/DBD.pm
%{epics_prefix}/lib/perl/DBD
%{epics_prefix}/lib/perl/EpicsHostArch.pl

%{epics_prefix}/lib/pkgconfig
%{epics_prefix}/include
%{epics_prefix}/dbd
%{epics_prefix}/db
%{epics_prefix}/templates
%{epics_prefix}/configure
%{epics_prefix}/cfg

%{epics_prefix}/html

%files static
%defattr(-,root,root)

%{epics_prefix}/lib/*/lib*.a

%files util
%defattr(-,root,root)

%{_bindir}/ca*

%changelog

