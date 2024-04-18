Name:       oeAware
Version:    1.0
Release:    1
Summary:    OeAware server and client 
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz

BuildRequires: cmake make gcc g++
BuildRequires: boost
BuildRequires: curl
BuildRequires: log4cplus-devel
BuildRequires: yaml-cpp-devel
BuildRequires: gtest-devel gmock-devel

BuildRequires: oeAware-collector
BuildRequires: oeAware-scenario

%description
OeAware provides server and client to manager plugins.

%prep
%autosetup -n %{name}-%{version}

%build
mkdir build
cd build 
cmake ..
make

%install
install -D -m 0750 build/src/plugin_mgr/oeAware %{buildroot}%{_bindir}/oeAware
install -D -m 0750 build/src/client/oeawarectl %{buildroot}%{_bindir}/oeawarectl
install -D -m 0640 config.yaml %{buildroot}%{_sysconfdir}/oeAware/config.yaml
install -D -p -m 0644 oeAware.service %{buildroot}%{_unitdir}/oeAware.service

%files
%attr(0750, root, root) %{_bindir}/oeAware
%attr(0750, root, root) %{_bindir}/oeawarectl
%attr(0640, root, root) %{_sysconfdir}/oeAware/config.yaml
%attr(0644, root, root) %{_unitdir}/oeAware.service

%changelog


