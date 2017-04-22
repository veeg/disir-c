Name:    disir
Version: 0.1.0
Release: 1%{?dist}
Summary: Disir configuration management library

License: UNDETERMINED
Source0: %{name}-%{version}.tar.gz

BuildRequires: cmake

%description
Disir is a configuration managment library and infrastructre that enables
developers to define the lifetime evolution of configuration entries
that is upgradable across application updates.


%package devel
Summary: Development environment for Disir

%description devel
The complete development environment for the core Disir library,
the CLI interface and the frontend protobuf service library.


%prep
%setup -q


%build
cmake -DCMAKE_INSTALL_PREFIX=/usr .
%make_build


%install
%make_install


%files
%{_bindir}/disir
%{_libdir}/*.so
%{_libdir}/disir/plugins/dplugin_toml.so
%{_libdir}/disir/plugins/dplugin_json.so


%files devel
%doc %{_defaultdocdir}/disir/*
%{_includedir}/*
%{_libdir}/disir/plugins/dplugin_test*
