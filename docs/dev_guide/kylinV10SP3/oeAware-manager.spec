Name:       oeAware-manager
Version:    v2.1.0
Release:    1
Summary:    OeAware is a framework for implementing low-load collection, sensing, and tuning on openEuler.
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz
Prefix:     %{_prefix}
BuildRequires: cmake make gcc-c++
BuildRequires: curl-devel yaml-cpp-devel log4cplus-devel
BuildRequires: libboundscheck numactl-devel git libnl3 libnl3-devel kernel-devel
BuildRequires: libbpf-devel clang

Requires: yaml-cpp curl log4cplus systemd libboundscheck acl libnl3 sysstat

%ifarch aarch64
BuildRequires: libkperf libkperf-devel
Requires: libkperf
%endif

Obsoletes:  oeAware-collector < v2.0.0
Obsoletes:  oeAware-scenario < v2.0.0
Obsoletes:  oeAware-tune < v2.0.0

%description
%{name} provides server and client components to manage plugins, which include collectors, scenarios, and tunes.

%package devel
Summary: Development files for plugin of oeaware
Requires: %{name} = %{version}-%{release}

%description devel
Development files for plugin of oeaware

%prep
%autosetup -n %{name}-%{version} -p1

%build
sh build.sh -k --release

%install
#install server
install -D -m 0750 build/output/bin/oeaware    %{buildroot}%{_bindir}/oeaware
install -D -m 0750 build/output/bin/oeawarectl %{buildroot}%{_bindir}/oeawarectl
install -D -p -m 0644 oeaware.service          %{buildroot}%{_unitdir}/oeaware.service

#install config
mkdir -p %{buildroot}%{_sysconfdir}/oeAware/plugin
install -D -m 0640 etc/plugin/*.yaml                              %{buildroot}%{_sysconfdir}/oeAware/plugin/
install -D -m 0640 etc/plugin/*.conf                              %{buildroot}%{_sysconfdir}/oeAware/plugin/
install -D -m 0640 etc/*.yaml                                     %{buildroot}%{_sysconfdir}/oeAware/
mkdir -p %{buildroot}%{_includedir}/oeaware/data

#install header
install -b -m740 ./build/output/include/oeaware/*.h               %{buildroot}%{_includedir}/oeaware
install -b -m740 ./build/output/include/oeaware/data/*.h          %{buildroot}%{_includedir}/oeaware/data

#install plugin so
mkdir -p %{buildroot}%{_libdir}/oeAware-plugin/
install -b -m740 ./build/output/plugin/lib/*.so                   %{buildroot}%{_libdir}/oeAware-plugin/


#install sdk
install -b -m740 ./build/output/sdk/liboeaware-sdk.so             %{buildroot}%{_libdir}

%preun
%systemd_preun oeaware.service

%post
if ! grep -q "oeaware:" /etc/group; then
        groupadd oeaware
fi
systemctl start oeaware.service
exit 0

%posttrans
. /etc/os-release || :
if [ "${VERSION}" == "22.03 (LTS-SP4)" ]; then
        systemctl enable oeaware.service
fi

%files
%attr(0750, root, root) %{_bindir}/oeaware
%attr(0750, root, root) %{_bindir}/oeawarectl
%attr(0640, root, root) %{_sysconfdir}/oeAware/*.yaml
%attr(0644, root, root) %{_unitdir}/oeaware.service
%attr(0640, root, root) %{_sysconfdir}/oeAware/plugin/*.yaml
%attr(0640, root, root) %{_sysconfdir}/oeAware/plugin/*.conf
%attr(0440, root, root) %{_libdir}/oeAware-plugin/*.so
%attr(0444, root, root) %{_libdir}/liboeaware-sdk.so

%files devel
%attr(0644, root, root) %{_includedir}/oeaware/*.h
%attr(0644, root, root) %{_includedir}/oeaware/data/*.h

%changelog
