# Title: Aqsis Package for RPM-based systems
# Author: Aqsis Team (packages@aqsis.org)

%define name        aqsis
%define version     1.3.0
%define release     1%{?dist}


Name:           %{name}
Version:        %{version}
Release:        %{release}
License:        GPLv2, LGPL
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  boost-devel >= 1.32.0
BuildRequires:  cmake >= 2.4.6
BuildRequires:  flex >= 2.5.4
BuildRequires:  gcc-c++
BuildRequires:  libtiff-devel >= 3.7.1
BuildRequires:  libjpeg-devel >= 6.2.0
BuildRequires:  bison >= 1.35.0
BuildRequires:  fltk-fluid >= 1.1.0
BuildRequires:  libxslt
BuildRequires:  OpenEXR-devel
BuildRequires:  zlib-devel >= 1.1.4
Requires:       boost >= 1.32.0
Requires:       xdg-utils >= 1.0.0
Group:          Applications/Multimedia
Summary:        Open source RenderMan-compliant 3D rendering solution
Url:            http://www.aqsis.org
Source:         %{name}-%{version}.tar.gz
#Source:         http://downloads.sourceforge.net/aqsis/%{name}-%{version}.tar.gz
#Source:         http://download.aqsis.org/builds/stable/source/tar/%{name}-%{version}.tar.gz

%description
Aqsis is a cross-platform photorealistic 3D rendering solution, based
on the RenderMan interface standard defined by Pixar Animation Studios.

This package contains a command-line renderer, a shader compiler for shaders
written using the RenderMan shading language, a texture pre-processor for
optimizing textures and a RIB processor.


%package devel
Requires:   %{name} = %{version}
Group:      Development/Libraries
Summary:    Development files for Aqsis

%description devel
Aqsis is a cross-platform photorealistic 3D rendering solution, based
on the RenderMan interface standard defined by Pixar Animation Studios.

This package contains various developer libraries to enable integration with
third-party applications.


%package data
Requires:   %{name} = %{version}
Group:      Applications/Multimedia
Summary:    Example content for Aqsis

%description data
Aqsis is a cross-platform photorealistic 3D rendering solution, based
on the RenderMan interface standard defined by Pixar Animation Studios.

This package contains example content, including additional scenes and shaders.


%prep


%build
export CFLAGS=$RPM_OPT_FLAGS
export CXXFLAGS=$RPM_OPT_FLAGS
mkdir -p "$RPM_BUILD_ROOT/BUILD"
cd "$RPM_BUILD_ROOT/BUILD"
cmake -DCMAKE_INSTALL_PREFIX="%{_prefix}" -DDEFAULT_RC_PATH=/ -DLIBDIR="%{_libdir}" ../
make %{?_smp_mflags}


%install
make install
mkdir -p "%{_datadir}/%{name}/desktop"
cp "$RPM_BUILD_ROOT/distribution/linux/*.*" "%{_datadir}/%{name}/desktop"
chmod a+rx "%{_datadir}/%{name}/scripts/mpanalyse.py"
chmod a+rx "%{_datadir}/%{name}/content/ribs/scenes/vase/render.sh"
chmod a+rx "%{_datadir}/%{name}/content/ribs/features/layeredshaders/render.sh"


%post
xdg-icon-resource install --novendor --size 192 %{_datadir}/%{name}/desktop/application.png aqsis
xdg-icon-resource install --novendor --context mimetypes --size 192 %{_datadir}/%{name}/desktop/mime.png application-x-slx
xdg-icon-resource install --novendor --context mimetypes --size 192 %{_datadir}/%{name}/desktop/mime.png model-x-rib
xdg-icon-resource install --novendor --context mimetypes --size 192 %{_datadir}/%{name}/desktop/mime.png model-x-rib-gzip
xdg-icon-resource install --novendor --context mimetypes --size 192 %{_datadir}/%{name}/desktop/mime.png text-x-sl
xdg-desktop-menu install --novendor %{_datadir}/%{name}/desktop/aqsis.desktop
xdg-desktop-menu install --novendor %{_datadir}/%{name}/desktop/aqsl.desktop
xdg-desktop-menu install --novendor %{_datadir}/%{name}/desktop/aqsltell.desktop
xdg-desktop-menu install --novendor %{_datadir}/%{name}/desktop/eqsl.desktop
xdg-desktop-menu install --novendor %{_datadir}/%{name}/desktop/piqsl.desktop
xdg-mime install --novendor %{_datadir}/%{name}/desktop/aqsis.xml


%preun
xdg-icon-resource uninstall --size 192 aqsis
xdg-icon-resource uninstall --context mimetypes --size 192 application-x-slx
xdg-icon-resource uninstall --context mimetypes --size 192 model-x-rib
xdg-icon-resource uninstall --context mimetypes --size 192 model-x-rib-gzip
xdg-icon-resource uninstall --context mimetypes --size 192 text-x-sl
xdg-desktop-menu uninstall %{_datadir}/%{name}/desktop/aqsis.desktop
xdg-desktop-menu uninstall %{_datadir}/%{name}/desktop/aqsl.desktop
xdg-desktop-menu uninstall %{_datadir}/%{name}/desktop/aqsltell.desktop
xdg-desktop-menu uninstall %{_datadir}/%{name}/desktop/eqsl.desktop
xdg-desktop-menu uninstall %{_datadir}/%{name}/desktop/piqsl.desktop
xdg-mime uninstall %{_datadir}/%{name}/desktop/aqsis.xml


%clean
rm -rf "$RPM_BUILD_ROOT"


%files
%defattr(-,root,root)
%doc AUTHORS COPYING README ReleaseNotes
%{_bindir}/aqsis
%{_bindir}/aqsl
%{_bindir}/aqsltell
%{_bindir}/eqsl
%{_bindir}/miqser
%{_bindir}/piqsl
%{_bindir}/teqser
%{_libdir}/%{name}/
%{_libdir}/*.so.*
%dir %{_sysconfdir}/%{name}
## Do not use noreplace with aqsis release
## This may definitly change in future releases.
%config %{_sysconfdir}/%{name}/aqsisrc
%dir %{_datadir}/%{name}
%{_datadir}/%{name}/desktop/
%{_datadir}/%{name}/scripts/
%{_datadir}/%{name}/shaders/


%files devel
%defattr(-,root,root)
%{_includedir}/%{name}/
%{_libdir}/*.so


%files data
%defattr(-,root,root)
%{_datadir}/%{name}/content/
%exclude %{_datadir}/%{name}/content/ribs/*/*/*.bat


%changelog
* Sun Apr 27 2008 Leon Tony Atkinson < latkinson [at] aqsis [dot] org > - 1.3.0.alpha
- Created new SPEC for latest Aqsis release, with some parts based on original SPEC
- Tested against Fedora 8
