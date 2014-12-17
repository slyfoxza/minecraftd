%global commit c7e924dee053accbbf101e6c7fc30f09fc2fa8fb
%global shortcommit %(c=%{commit}; echo ${c:0:7})

Name:           minecraftd
Version:        1.0
Release:        0.1.20141217git%{shortcommit}%{?dist}
Summary:        Minecraft server daemon

License:        ASL 2.0
URL:            https://slyfoxza.github.io/minecraftd
Source0:        https://github.com/slyfoxza/minecraftd/archive/%{commit}/minecraftd-%{commit}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires:  autoconf automake

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


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc README.md
%{_bindir}/minecraftd
%config(noreplace) %{_sysconfdir}/minecraftd/minecraftd.conf

%changelog
