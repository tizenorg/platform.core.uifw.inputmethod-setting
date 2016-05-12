Name:       org.tizen.inputmethod-setting
Summary:    Input Method Setting Application
Version:    0.5.3
Release:    1
Group:      Graphics & UI Framework/Input
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz

BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(isf)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  cmake
BuildRequires:  efl-extension-devel
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  model-build-features
BuildRequires:  pkgconfig(capi-ui-inputmethod-manager)

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

%if "%{?profile}" == "mobile"
CFLAGS+=" -D_MOBILE";
CXXFLAGS+=" -D_MOBILE";
%endif

%if "%{?profile}" == "wearable"
CFLAGS+=" -D_WEARABLE";
CXXFLAGS+=" -D_WEARABLE";
%if "%{model_build_feature_formfactor}" == "circle"
CFLAGS+=" -D_CIRCLE";
CXXFLAGS+=" -D_CIRCLE";
%endif
%endif

rm -rf CMakeFiles
rm -rf CMakeCache.txt
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \
        -DTZ_SYS_RO_APP=%TZ_SYS_RO_APP \
        -DTZ_SYS_RO_PACKAGES=%TZ_SYS_RO_PACKAGES \
        -DTZ_SYS_RO_ICONS=%TZ_SYS_RO_ICONS
make %{?jobs:-j%jobs}

%post
/sbin/ldconfig

mkdir -p %{TZ_SYS_RO_APP}/%{name}/bin
mkdir -p %{TZ_SYS_RO_APP}/%{name}/res

%postun -p /sbin/ldconfig

%install
rm -rf %{buildroot}

%make_install
%find_lang inputmethod-setting

%files -f inputmethod-setting.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{TZ_SYS_RO_APP}/%{name}/bin/*
%{TZ_SYS_RO_ICONS}/default/small/*
%{TZ_SYS_RO_PACKAGES}/%{name}.xml
%license LICENSE
