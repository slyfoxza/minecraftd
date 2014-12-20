%global commit 3e653d766f8d383ce6f21d07b58592ea7696ddee
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:           minecraftd
Version:        1.0
Release:        0.1.20141221git%{shortcommit}%{?dist}
Summary:        Minecraft server daemon

License:        ASL 2.0
URL:            https://slyfoxza.github.io/minecraftd
Source0:        https://github.com/slyfoxza/minecraftd/archive/%{commit}/minecraftd-%{commit}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

Requires:       java-headless jpackage-utils
BuildRequires:  autoconf automake
BuildRequires:  pkgconfig(giomm-2.4) pkgconfig(libconfig) pkgconfig(libzip)
BuildRequires:  java-devel

%description


%prep
%setup -qn %{name}-%{commit}
autoreconf -i -f

%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/games/minecraft
mkdir -p $RPM_BUILD_ROOT%{_localstatedir}/log/minecraftd

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc README.md
%{_bindir}/minecraftd
%dir %{_sysconfdir}/minecraftd
%config(noreplace) %{_sysconfdir}/minecraftd/log4j2.xml
%config(noreplace) %{_sysconfdir}/minecraftd/minecraftd.conf
%dir %{_localstatedir}/games/minecraft
%dir %{_localstatedir}/log/minecraftd

%changelog
