Name: 		flowsec	
Version:	1
Release:	1
Summary:	nf_sender and nf_receiver daemons

BuildArch:	noarch
Group:		Productivity/Networking/Services
License:	GPL
URL:		http://localhost
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Source:		flowsec.tar.gz

%description
nf_sender and nf_receiver daemons for secure transport of NetFlow records

%package nf_receiver
Summary:	nf_receiver daemon
Group:		Productivity/Networking/Services
%description nf_receiver
nf_receiver daemon securely receives records from nf_sender and sends them to NetFlow collector

%package nf_sender
Summary:	nf_sender daemon
Group:		Producitivity/Networking/Services
%description nf_sender
nf_sender daemon receives records from NetFlow probe and sends them securely to nf_receiver

%prep
%setup

%build

%install
install -m 0755 -d $RPM_BUILD_ROOT/usr/bin
install -m 0755 -d $RPM_BUILD_ROOT/etc/sysconfig
install -m 0755 -d $RPM_BUILD_ROOT/etc/init.d
install -m 0755 -d $RPM_BUILD_ROOT/etc/rc.d/rc3.d
install -m 0755 -d $RPM_BUILD_ROOT/etc/rc.d/rc5.d
install -m 0755 -d $RPM_BUILD_ROOT/usr/share/man/man1

install -m 0755 bin/nf_receiver $RPM_BUILD_ROOT/usr/bin/nf_receiver
install -m 0755 conf/nf_receiver $RPM_BUILD_ROOT/etc/sysconfig/nf_receiver
install -m 0755 init/nf_receiver $RPM_BUILD_ROOT/etc/init.d/nf_receiver
install -m 0755 man/nf_receiver $RPM_BUILD_ROOT/usr/share/man/man1/nf_receiver.1

install -m 0755 bin/nf_sender $RPM_BUILD_ROOT/usr/bin/nf_sender
install -m 0755 conf/nf_sender $RPM_BUILD_ROOT/etc/sysconfig/nf_sender
install -m 0755 init/nf_sender $RPM_BUILD_ROOT/etc/init.d/nf_sender
install -m 0755 man/nf_sender $RPM_BUILD_ROOT/usr/share/man/man1/nf_sender.1

ln -s /etc/init.d/nf_receiver $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S82nf_receiver
ln -s /etc/init.d/nf_receiver $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S82nf_receiver

ln -s /etc/init.d/nf_sender $RPM_BUILD_ROOT/etc/rc.d/rc3.d/S82nf_sender
ln -s /etc/init.d/nf_sender $RPM_BUILD_ROOT/etc/rc.d/rc5.d/S82nf_sender

%clean
rm -rf $RPM_BUILD_ROOT

%files

%files nf_receiver
%defattr(-,root,root-,)
%doc /usr/share/man/man1/nf_receiver.1.gz
/usr/bin/nf_receiver
/etc/sysconfig/nf_receiver
/etc/init.d/nf_receiver
/etc/rc.d/rc3.d/S82nf_receiver
/etc/rc.d/rc5.d/S82nf_receiver

%files nf_sender
%defattr(-,root,root,-)
%doc /usr/share/man/man1/nf_sender.1.gz
/usr/bin/nf_sender
/etc/sysconfig/nf_sender
/etc/init.d/nf_sender
/etc/rc.d/rc3.d/S82nf_sender
/etc/rc.d/rc5.d/S82nf_sender


%changelog

