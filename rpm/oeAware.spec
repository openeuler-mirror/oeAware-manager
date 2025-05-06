%global debug_package %{nil}
Name:       oeAware-manager
Version:    %{commit_id}
Release:    1
Summary:    OeAware server and client 
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz
Prefix:     %{_prefix}
BuildRequires: cmake make gcc-c++
BuildRequires: curl-devel yaml-cpp-devel log4cplus-devel gtest-devel gmock-devel
BuildRequires: libboundscheck numactl-devel git libnl3 libnl3-devel kernel-devel
BuildRequires: libbpf-devel clang

Requires: yaml-cpp curl log4cplus systemd libboundscheck acl patchelf libnl3 sysstat
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
install -D -m 0640 info_cmd.yaml                 %{buildroot}%{_sysconfdir}/oeAware/info_cmd.yaml
install -D -p -m 0644 oeaware.service          %{buildroot}%{_unitdir}/oeaware.service

#install plugin
install -D -m 0640 preload.yaml                %{buildroot}%{_sysconfdir}/oeAware/preload.yaml
install -D -m 0640 etc/analysis_config.yaml    %{buildroot}%{_sysconfdir}/oeAware/analysis_config.yaml
mkdir -p %{buildroot}%{_libdir}/oeAware-plugin/
mkdir -p %{buildroot}%{_includedir}/oeaware/data
install -dm 0755 %{buildroot}%{_prefix}/lib/smc
%ifarch aarch64
install -b -m740  ./build/libkperf/output/lib/*.so                %{buildroot}%{_libdir}
install -D -m 0640 etc/hardirq_tune.conf                          %{buildroot}%{_libdir}/oeAware-plugin/
%endif
install -b -m740 ./build/output/plugin/lib/*.so                   %{buildroot}%{_libdir}/oeAware-plugin/
install -b -m740 ./build/output/include/oeaware/*.h               %{buildroot}%{_includedir}/oeaware
install -b -m740 ./build/output/include/oeaware/data/*.h          %{buildroot}%{_includedir}/oeaware/data
install -b -m740 ./build/output/sdk/liboeaware-sdk.so             %{buildroot}%{_libdir}
install -D -m 0640 ./build/output/plugin/lib/thread_scenario.conf %{buildroot}%{_libdir}/oeAware-plugin/
install -D -m 0640 ./build/output/plugin/lib/ub_tune.conf         %{buildroot}%{_libdir}/oeAware-plugin/
install -D -m 0400 ./build/output/plugin/ko/smc_acc.ko            %{buildroot}%{_prefix}/lib/smc
install -D -m 0640 ./build/output/plugin/lib/xcall.yaml           %{buildroot}%{_libdir}/oeAware-plugin/
install -D -m 0640 ./build/output/plugin/lib/smc_acc.yaml         %{buildroot}%{_libdir}/oeAware-plugin/
%preun
%systemd_preun oeaware.service

%post
if ! grep -q "oeaware:" /etc/group; then
        groupadd oeaware
        setfacl -m g:oeaware:r /usr/lib64/liboeaware-sdk.so
fi
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
%attr(0640, root, root) %{_sysconfdir}/oeAware/info_cmd.yaml
%attr(0640, root, root) %{_sysconfdir}/oeAware/preload.yaml
%attr(0640, root, root) %{_sysconfdir}/oeAware/analysis_config.yaml
%attr(0644, root, root) %{_unitdir}/oeaware.service
%attr(0640, root, root) %{_libdir}/oeAware-plugin/ub_tune.conf
%attr(0640, root, root) %{_libdir}/oeAware-plugin/thread_scenario.conf
%attr(0640, root, root) %{_libdir}/oeAware-plugin/xcall.yaml
%attr(0640, root, root) %{_libdir}/oeAware-plugin/smc_acc.yaml
%attr(0400, root, root) %{_prefix}/lib/smc/smc_acc.ko

%ifarch aarch64
%attr(0755, root, root) %{_libdir}/libkperf.so
%attr(0755, root, root) %{_libdir}/libsym.so
%attr(0640, root, root) %{_libdir}/oeAware-plugin/hardirq_tune.conf
%endif
%attr(0440, root, root) %{_libdir}/oeAware-plugin/*.so
%attr(0440, root, root) %{_libdir}/liboeaware-sdk.so

%files devel
%attr(0644, root, root) %{_includedir}/oeaware/*.h
%attr(0644, root, root) %{_includedir}/oeaware/data/*.h

%changelog
