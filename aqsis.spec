# Mo Jan 17 16:36:55 CET 2005 
# automatically created with specgen.sh

Name: aqsis
Version: 1.0.0
Release: FC1
Summary: RenderMan(tm)-compatible renderer
License: GPL
Group: Applications/Graphics
Vendor: none
URL: www.aqsis.com
Packager: Tobias Sauerwein
Source: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-root
Requires: /sbin/ldconfig, libtiff >= 3.5.7, libjpeg >= 6b, zlib >= 1.1.4, fltk
AutoReqProv: no

%description
A RenderMan(tm)-compatible renderer

%prep
%setup -q

%build
export CFLAGS="-O3"
export CXXFLAGS="-O3"
./configure --bindir=%{_bindir} --mandir=%{_mandir} --libdir=%{_libdir} --datadir=%{_datadir} --includedir=%{_includedir} --sysconfdir=%{_sysconfdir}

make

%install
rm -fr %{buildroot}

%makeinstall

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%clean
rm -fr %{buildroot}

%files
%defattr(-,root,root)
%doc ChangeLog COPYING AUTHORS README ribfiles
%{_bindir}/*
%{_libdir}/*
%dir %{_datadir}/aqsis/shaders
%{_datadir}/aqsis/shaders/*
%{_includedir}/*
%{_mandir}/man1/*

%changelog

* Tue Jul 13 2004 cgtobix <cgtobix@users.sourceforge.net>
- Update to the new display system
* Sun Mar 21 2004 cgtobix <cgtobix@users.sourceforge.net>
- Now building without debug information
* Thu Mar 18 2004 cgtobix <cgtobix@users.sourceforge.net>
- Make it working for RedHat9/Fedora
* Wed Oct 23 2002 Damien Miller <djm@mindrot.org>
- Make an RPM

