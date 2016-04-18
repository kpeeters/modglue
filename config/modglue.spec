Summary: C++ library for handling of multiple co-processes
Name: modglue
Version: 1.16
Release: 1
License: GPL
Group: Development/Libraries
Source: http://cadabra.phi-sci.com/modglue-1.16.tar.gz
URL: http://cadabra.phi-sci.com/
Packager: Kasper Peeters <kasper.peeters@phi-sci.com>
BuildRoot: /tmp/modglue-1.16

%if 0%{?suse_version}
BuildRequires: libsigc++2-devel
%endif

# SLE fails because of broken libtool
%if 0%{?sles_version}
BuildRequires: libsigc++2-devel
%endif

%if 0%{?centos_version}
BuildRequires: libsigc++20-devel
%endif

# RHEL-6 barfs on this?!?
%if 0%{?rhel_version} 
BuildRequires: libsigc++20-devel
%endif

%if 0%{?fedora}
BuildRequires: libsigc++20-devel
%endif

%if 0%{?mdkversion}
BuildRequires: libsigc++2.0-devel
%endif

%if 0%{?mandriva_version} 
BuildRequires: libsigc++2.0-devel
%endif

BuildRequires: gcc-c++, pkgconfig, libtool
Prefix: /usr

%description
Modglue is a C++ library with classes for forking external processes
and asynchronous reading from streams. It takes away the burden of all
subtleties involving the Unix fork call. The asynchronous read
facility enables one to read on multiple input streams at the same
time, without losing any of the standard C++ stream facilities.

%prep
%setup
./configure --prefix=/usr --libdir=/%_lib

# libdir is a disaster. On Ubuntu, a prefix=/usr leads to a @libdir@
# of /lib, so you need @prefix@/@libdir@ in the makefile. On Fedora,
# @libdir@ needs to be set to an absolute path, so you need the 
# prefixing slash.

%build
make

%files
%defattr(-,root,root)
%dir /usr/include/modglue
/usr/include/modglue/ext_process.hh
/usr/include/modglue/process.hh
/usr/include/modglue/pipe.hh
/usr/include/modglue/main.hh
/usr/bin/ptywrap
/usr/bin/prompt
%_libdir/libmodglue.la
%_libdir/libmodglue.a
%_libdir/libmodglue.so
%_libdir/libmodglue.so.1
%_libdir/libmodglue.so.1.0.16
%_libdir/pkgconfig/modglue.pc
/usr/share/man/man1/prompt.1.*
/usr/share/man/man1/ptywrap.1.*

%install
make DESTDIR="$RPM_BUILD_ROOT" DEVDESTDIR="$RPM_BUILD_ROOT" install
