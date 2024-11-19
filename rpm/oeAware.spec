%global debug_package %{nil}
Name:       oeAware-buildbysrc
Version:    v2.0.0
Release:    1
Summary:    OeAware server and client 
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz
Prefix:     %{_prefix}
BuildRequires: cmake make gcc-c++
BuildRequires: curl-devel yaml-cpp-devel log4cplus-devel gtest-devel gmock-devel
BuildRequires: libboundscheck numactl-devel git

Requires: yaml-cpp curl log4cplus systemd libboundscheck
Obsoletes:  oeAware-collector < v2.0.0
Obsoletes:  oeAware-scenario < v2.0.0
Obsoletes:  oeAware-tune < v2.0.0

%description
%{name} provides server and client to manager plugins.

%package devel
Summary: Development files for plugin of oeaware
Requires: %{name} = %{version}-%{release}

%description devel
Development files for plugin of oeaware

%prep
%autosetup -n %{name}-%{version} -p1

%build
bash build.sh

%install
#install server
install -D -m 0750 build/output/bin/oeaware    %{buildroot}%{_bindir}/oeaware
install -D -m 0750 build/output/bin/oeawarectl %{buildroot}%{_bindir}/oeawarectl
install -D -m 0640 config.yaml                 %{buildroot}%{_sysconfdir}/oeAware/config.yaml
install -D -p -m 0644 oeaware.service          %{buildroot}%{_unitdir}/oeaware.service

#install plugin
mkdir -p %{buildroot}%{_libdir}/oeAware-plugin/
mkdir -p %{buildroot}%{_includedir}/oeaware/interface
install -dm 0755 %{buildroot}%{_prefix}/lib/smc
%ifarch aarch64
install -b -m740  ./build/libkperf/output/lib/*.so                %{buildroot}%{_libdir}
%endif
install -b -m740 ./build/output/plugin/lib/*.so                   %{buildroot}%{_libdir}/oeAware-plugin/
install -b -m740 ./build/output/include/*.h                       %{buildroot}%{_includedir}/oeaware
install -b -m740 ./build/output/include/interface/*.h             %{buildroot}%{_includedir}/oeaware/interface
install -b -m740 ./build/output/sdk/liboeaware-sdk.so             %{buildroot}%{_libdir}
install -D -m 0640 ./build/output/plugin/lib/thread_scenario.conf %{buildroot}%{_libdir}/oeAware-plugin/
install -D -m 0640 ./build/output/plugin/lib/ub_tune.conf         %{buildroot}%{_libdir}/oeAware-plugin/
install -D -m 0400 ./build/output/plugin/ko/smc_acc.ko            %{buildroot}%{_prefix}/lib/smc

%preun
%systemd_preun oeaware.service

%post
systemctl start oeaware.service
chcon -t modules_object_t %{_prefix}/lib/smc/smc_acc.ko >/dev/null 2>&1
exit 0

%posttrans
. /etc/os-release || :
if [ "${VERSION}" == "22.03 (LTS-SP4)" ]; then
        systemctl enable oeaware.service
fi

%files
%attr(0750, root, root) %{_bindir}/oeaware
%attr(0750, root, root) %{_bindir}/oeawarectl
%attr(0640, root, root) %{_sysconfdir}/oeAware/config.yaml
%attr(0644, root, root) %{_unitdir}/oeaware.service
%attr(0640, root, root) %{_libdir}/oeAware-plugin/ub_tune.conf
%attr(0640, root, root) %{_libdir}/oeAware-plugin/thread_scenario.conf
%attr(0400, root, root) %{_prefix}/lib/smc/smc_acc.ko

%ifarch aarch64
%attr(0440, root, root) %{_libdir}/libkperf.so
%attr(0440, root, root) %{_libdir}/libsym.so
%endif
%attr(0440, root, root) %{_libdir}/oeAware-plugin/*.so
%attr(0440, root, root) %{_libdir}/liboeaware-sdk.so

%files devel
%attr(0440, root, root) %{_includedir}/oeaware/*.h
%attr(0440, root, root) %{_includedir}/oeaware/interface/*.h

%changelog
* The 19 2024 LHesperus <liuchanggeng@huawei.com> -v2.0.0-1
- Package init
