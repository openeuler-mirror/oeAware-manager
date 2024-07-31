Name:       oeAware-manager
Version:    v1.0.2
Release:    1
Summary:    OeAware server and client 
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz

BuildRequires: cmake make gcc-c++
BuildRequires: boost-devel
BuildRequires: curl-devel
BuildRequires: log4cplus-devel
BuildRequires: yaml-cpp-devel
BuildRequires: gtest-devel gmock-devel

Requires: oeAware-collector >= v1.0.2
Requires: oeAware-scenario  >= v1.0.2
Requires: oeAware-tune      >= v1.0.0
Requires: graphviz yaml-cpp curl log4cplus boost systemd

%description
%{name} provides server and client to manager plugins.

%prep
%autosetup -n %{name}-%{version} -p1

%build
mkdir build
cd build 
cmake ..
make %{?_smp_mflags}

%install
install -D -m 0750 build/src/plugin_mgr/oeaware %{buildroot}%{_bindir}/oeaware
install -D -m 0750 build/src/client/oeawarectl %{buildroot}%{_bindir}/oeawarectl
install -D -m 0640 config.yaml %{buildroot}%{_sysconfdir}/oeAware/config.yaml
install -D -p -m 0644 oeaware.service %{buildroot}%{_unitdir}/oeaware.service

%preun
%systemd_preun oeaware.service

%post
systemctl start oeaware.service
systemctl enable oeaware.service

%files
%attr(0750, root, root) %{_bindir}/oeaware
%attr(0750, root, root) %{_bindir}/oeawarectl
%attr(0640, root, root) %{_sysconfdir}/oeAware/config.yaml
%attr(0644, root, root) %{_unitdir}/oeaware.service

%changelog


