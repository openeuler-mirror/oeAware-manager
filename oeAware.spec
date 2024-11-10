%global debug_package %{nil}
Name:       oeAware-manager
Version:    v2.0.0
Release:    1
Summary:    OeAware server and client 
License:    MulanPSL2
URL:        https://gitee.com/openeuler/%{name}
Source0:    %{name}-%{version}.tar.gz

BuildRequires: cmake make gcc-c++
BuildRequires: curl-devel yaml-cpp-devel log4cplus-devel gtest-devel gmock-devel
BuildRequires: libboundscheck numactl-devel git

Requires: yaml-cpp curl log4cplus systemd libboundscheck

%description
%{name} provides server and client to manager plugins.

%prep
%autosetup -n %{name}-%{version} -p1

%build
bash build.sh

%install
#install server
install -D -m 0750 build/bin/oeaware %{buildroot}%{_bindir}/oeaware
install -D -m 0750 build/bin/oeawarectl %{buildroot}%{_bindir}/oeawarectl
install -D -m 0640 config.yaml %{buildroot}%{_sysconfdir}/oeAware/config.yaml
install -D -p -m 0644 oeaware.service %{buildroot}%{_unitdir}/oeaware.service

#install plugin
mkdir -p ${RPM_BUILD_ROOT}%{_libdir}/oeAware-plugin/
mkdir -p ${RPM_BUILD_ROOT}%{_includedir}/oeaware
%ifarch aarch64
install -b -m740  ./build/libkperf/output/lib/*.so     ${RPM_BUILD_ROOT}%{_libdir}
%endif
install -b -m740  ./build/output/plugin/lib/*.so     ${RPM_BUILD_ROOT}%{_libdir}/oeAware-plugin/
install -b -m740 ./build/include/*                   ${RPM_BUILD_ROOT}%{_includedir}/oeaware
install -b -m740 ./build/src/sdk/liboeaware-sdk.so   ${RPM_BUILD_ROOT}%{_libdir}
%preun
%systemd_preun oeaware.service

%post
systemctl start oeaware.service

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

%ifarch aarch64
%attr(0440, root, root) %{_libdir}/libkperf.so
%attr(0440, root, root) %{_libdir}/libsym.so
%endif
%attr(0440, root, root) %{_libdir}/oeAware-plugin/*.so
%attr(0440, root, root) %{_includedir}/oeaware/*.h

%attr(0440, root, root) %{_libdir}/liboeaware-sdk.so
%changelog


