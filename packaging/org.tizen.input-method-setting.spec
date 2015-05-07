%bcond_with x
%bcond_with wayland

Name:       org.tizen.inputmethod-setting
Summary:    Setting Application for ISF
Version:    0.0.1
Release:    1
Group:      System Environment/Libraries
License:    Apache License, Version 2.0
Source0:    %{name}-%{version}.tar.gz

%if "%{?tizen_profile_name}" == "wearable"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(isf)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  cmake
BuildRequires:  efl-extension-devel

%description
Setting Application for ISF.

%prep
%setup -q -n %{name}-%{version}

%build
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

rm -rf CMakeFiles
rm -rf CMakeCache.txt

%if %{with wayland}
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -Dwith_wayland=TRUE
%else
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
%endif
make %{?jobs:-j%jobs}

%post
/sbin/ldconfig

mkdir -p /usr/apps/org.tizen.inputmethod-setting/bin
mkdir -p /usr/apps/org.tizen.inputmethod-setting/res

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/usr/share/license

%files
%manifest org.tizen.inputmethod-setting.manifest
%defattr(-,root,root,-)
/usr/apps/org.tizen.inputmethod-setting/bin/*
/usr/apps/org.tizen.inputmethod-setting/res/locale/*
/usr/share/license/*
/usr/share/packages/org.tizen.inputmethod-setting.xml
/usr/share/icons/default/small/*
