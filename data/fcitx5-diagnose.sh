#!/usr/bin/env bash

shopt -s extglob nullglob globstar
export TEXTDOMAIN=fcitx5

__test_bash_unicode() {
    local magic_str='${1}'$'\xe4'$'\xb8'$'\x80'
    local magic_replace=${magic_str//\$\{/$'\n'$\{}
    ! [ "${magic_str}" = "${magic_replace}" ]
}

if type gettext &> /dev/null && __test_bash_unicode; then
    _() {
        gettext "$@"
    }
else
    _() {
        echo "$@"
    }
fi

#############################
# utility
#############################

array_push() {
    eval "${1}"'=("${'"${1}"'[@]}" "${@:2}")'
}

_find_file() {
    local "${1}"
    eval "${2}"'=()'
    while IFS= read -r -d '' "${1}"; do
        array_push "${2}" "${!1}"
    done < <(find "${@:3}" -print0 2> /dev/null)
}

find_file() {
    # Avoid variable name conflict (Not that anyone is using the internal
    # variable name though...)
    if [[ ${1} = __find_file_line ]]; then
        _find_file __find_file_line2 "$@"
    else
        _find_file __find_file_line "$@"
    fi
}

str_match_glob() {
    local pattern=$1
    local str=$2
    case "$2" in
        $pattern)
            return 0
            ;;
    esac
    return 1
}

str_match_regex() {
    local pattern=$1
    local str=$2
    [[ $str =~ $pattern ]]
}

add_and_check_file() {
    local prefix="$1"
    local file="$2"
    local inode
    inode="$(stat -L --printf='%i' "${file}" 2> /dev/null)" || return 0
    local varname="___add_and_check_file_${prefix}_${inode}"
    [ ! -z "${!varname}" ] && return 1
    eval "${varname}=1"
    return 0
}

unique_file_array() {
    for f in "${@:3}"; do
        add_and_check_file "${1}" "${f}" && {
            array_push "${2}" "${f}"
        }
    done
}

print_array() {
    for ele in "$@"; do
        echo "${ele}"
    done
}

repeat_str() {
    local i
    local n="$1"
    local str="$2"
    local res=""
    for ((i = 0;i < n;i++)); do
        res="${res}${str}"
    done
    echo "${res}"
}

# require `shopt -s nullglob` and the argument needs to be a glob
find_in_path() {
    local w="$1"
    local IFS=':'
    local p
    local f
    local fs
    for p in ${PATH}; do
        eval 'fs=("${p}/"'"${w}"')'
        for f in "${fs[@]}"; do
            echo "$f"
        done
    done
}

__get_pretty_name() {
    local _home=$(realpath ~ 2> /dev/null || echo ~)
    local _orig=$(realpath "${1}" 2> /dev/null || echo "${1}")
    if [[ ${_orig}/ =~ ^${_home}/ ]]; then
        echo "~${_orig#${_home}}"
    else
        echo "${_orig}"
    fi
}

fcitx_exe="$(command -v fcitx5 2> /dev/null)"

__conf_dir_init() {
    # Don't do any fancy check here, it's the user's fault, which we should detect
    # later, if it is set to some non-sense value.
    if [[ -n ${XDG_CONFIG_HOME} ]]; then
        _xdg_conf_home=${XDG_CONFIG_HOME}
    else
        _xdg_conf_home=~/.config
    fi
    fx_conf_home=${_xdg_conf_home}/fcitx5
    xdg_conf_pretty_name=$(__get_pretty_name "${_xdg_conf_home}")
    fx_conf_pretty_name=$(__get_pretty_name "${fx_conf_home}")
}
__conf_dir_init

get_from_config_file() {
    local file="$1"
    local key="$2"
    local value
    value=$(sed -ne "s=^${key}\=\(.*\)=\1=gp" "$file" 2> /dev/null)
    [ -z "$value" ] && return 1
    echo "${value}"
    return 0
}

# Read everything within a section into an associative array
get_config_section() {
    local var=$1
    local file=$2
    local section=$3
    local line
    local in_section=0
    while read -r line; do
        line=${line%%#*}
        if [[ $line =~ ^\[(.*)\] ]]; then
            ((in_section)) && return
            if [[ $section = ${BASH_REMATCH[1]} ]]; then
                in_section=1
            fi
            continue
        fi
        ((in_section)) || continue
        if [[ $line =~ ^([^=]*)=(.*) ]]; then
            eval $var'["${BASH_REMATCH[1]}"]="${BASH_REMATCH[2]}"'
        fi
    done < "${file}"
}

get_locale() {
    local name=$1
    str_match_glob 'LC_*' "$name" || str_match_glob 'LANG' "$name" || {
        name="LC_$name"
    }
    [ -z "${LC_ALL}" ] || {
        echo "${LC_ALL}"
        return
    }
    [ -z "${!name}" ] || {
        echo "${!name}"
        return
    }
    [ -z "${LANG}" ] || {
        echo "${LANG}"
        return
    }
    echo "POSIX"
}

if type dbus-send &> /dev/null; then
    dbus_exe=dbus-send
    dbus_get_name_owner() {
        local address
        address=$(dbus-send --print-reply=literal --dest=org.freedesktop.DBus \
            /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner \
            "string:$1" 2> /dev/null) || return 1
        echo -n "${address##* }"
    }
    dbus_get_pid() {
        local pid
        pid=$(dbus-send --print-reply=literal --dest=org.freedesktop.DBus \
            /org/freedesktop/DBus org.freedesktop.DBus.GetConnectionUnixProcessID \
            "string:$1" 2> /dev/null) || return 1
        echo -n "${pid##* }"
    }
    dbus_debuginfo() {
        local debuginfo
        debuginfo=$(dbus-send --print-reply=literal --dest=org.fcitx.Fcitx5 \
            /controller org.fcitx.Fcitx.Controller1.DebugInfo 2> /dev/null) || return 1
        echo -n "${debuginfo}"
    }
elif qdbus_exe=$(command -v qdbus 2> /dev/null) || \
        qdbus_exe=$(command -v qdbus-qt4 2> /dev/null) || \
        qdbus_exe=$(command -v qdbus-qt5 2> /dev/null); then
    dbus_exe=${qdbus_exe}
    dbus_get_name_owner() {
        "${qdbus_exe}" org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.GetNameOwner "$1" 2> /dev/null
    }
    dbus_get_pid() {
        "${qdbus_exe}" org.freedesktop.DBus /org/freedesktop/DBus \
            org.freedesktop.DBus.GetConnectionUnixProcessID "$1" 2> /dev/null
    }
    dbus_debuginfo() {
        "${qdbus_exe}" org.fcitx.Fcitx5 /controller \
            org.fcitx.Fcitx.Controller1.DebugInfo 2> /dev/null
    }
else
    dbus_exe=
    dbus_get_name_owner() {
        return 1
    }
    dbus_get_pid() {
        return 1
    }
fi

print_process_info() {
    ps -o pid=,args= --pid "$1" 2> /dev/null && return
    cmdline=''
    [[ -d /proc/$1 ]] && {
        cmdline=$(cat /proc/$1/cmdline) || cmdline=$(cat /proc/$1/comm) || \
            cmdline=$(readlink /proc/$1/exe)
    } 2> /dev/null
    echo "$1 ${cmdline}"
}

# Detect DE

_detectDE_XDG_CURRENT() {
    for desktop in $(echo "${XDG_CURRENT_DESKTOP}" | tr ":" "\n"); do
        case "${desktop}" in
            GNOME)
            DE=gnome
            return
            ;;
            KDE)
            DE=kde
            return
            ;;
            LXDE)
            DE=lxde
            return
            ;;
            XFCE)
            DE=xfce
            return
            ;;
            Deepin)
            DE=deepin
            return
            ;;
            X-Cinnamon)
            DE=cinnamon
            return
            ;;
        esac
    done
    return 1
}

_detectDE_classic() {
    if [ x"$KDE_FULL_SESSION" = x"true" ]; then
        DE=kde
    elif xprop -root KDE_FULL_SESSION 2> /dev/null | \
        grep ' = \"true\"$' > /dev/null 2>&1; then
        DE=kde
    elif [ x"$GNOME_DESKTOP_SESSION_ID" != x"" ]; then
        DE=gnome
    elif [ x"$MATE_DESKTOP_SESSION_ID" != x"" ]; then
        DE=mate
    elif dbus_get_name_owner org.gnome.SessionManager > /dev/null; then
        DE=gnome
    elif xprop -root _DT_SAVE_MODE 2> /dev/null | \
        grep ' = \"xfce4\"$' >/dev/null 2>&1; then
        DE=xfce
    elif xprop -root 2> /dev/null | \
        grep -i '^xfce_desktop_window' >/dev/null 2>&1; then
        DE=xfce
    else
        return 1
    fi
}

_detectDE_SESSION() {
    case "$DESKTOP_SESSION" in
        gnome)
            DE=gnome
            ;;
        LXDE|Lubuntu)
            DE=lxde
            ;;
        xfce|xfce4|'Xfce Session')
            DE=xfce
            ;;
        Deepin)
            DE=deepin
            ;;
        cinnamon)
            DE=cinnamon
            ;;
        *)
            return 1
            ;;
    esac
}

_detectDE_uname() {
    case "$(uname 2>/dev/null)" in
        Darwin)
            DE=darwin
            ;;
        *)
            return 1
            ;;
    esac
}

detectDE() {
    # see https://bugs.freedesktop.org/show_bug.cgi?id=34164
    unset GREP_OPTIONS

    _detectDE_XDG_CURRENT || _detectDE_classic || \
        _detectDE_SESSION || _detectDE_uname || {
        DE=generic
    }
    if [ x"$DE" = x"gnome" ]; then
        # gnome-default-applications-properties is only available in GNOME 2.x
        # but not in GNOME 3.x
        command -v gnome-default-applications-properties > /dev/null 2>&1 || \
            DE="gnome3"
        command -v gnome-shell &> /dev/null && DE="gnome3"
    fi
}

maybe_gnome3() {
    [[ $DE = gnome3 ]] && return 0
    [[ $DE = generic ]] && command -v gnome-shell &> /dev/null && return 0
    return 1
}

detectDE

# user and uid

detect_user() {
    if command -v id &> /dev/null; then
        cur_user=$(id -un)
        cur_uid=$(id -u)
    else
        if [[ -n $UID ]]; then
            cur_uid=$UID
        elif [[ -d /proc/$$/ ]]; then
            cur_uid=$(stat -c %u /proc/$$/)
        else
            cur_uid=""
        fi
        if command -v whoami &> /dev/null; then
            cur_user=$(whoami)
        elif [[ -d /proc/$$/ ]]; then
            cur_user=$(stat -c %U /proc/$$/)
        elif [[ -n $USER ]]; then
            cur_user=$USER
        else
            cur_user=""
        fi
    fi
}

detect_user &> /dev/null

try_open() {
    (exec < "$1") &> /dev/null
}

_check_open_root() {
    for f in /proc/1/environ /proc/1/mem /proc/kcore /proc/kmem; do
        try_open "$f" && return 0
    done
    if command -v readlink &> /dev/null; then
        for f in /proc/1/exe /proc/1/cwd /proc/1/root; do
            readlink "$f" &> /dev/null && return 0
        done
    fi
    return 1
}

check_is_root() {
    if [[ $cur_uid = 0 ]]; then
        return 0
    elif [[ $cur_user = root ]]; then
        return 0
    elif [[ -n $cur_uid ]] && [[ -n $cur_user ]]; then
        return 1
    elif _check_open_root; then
        return 0
    fi
    return 1
}

# ldd

run_ldd() {
    [ -f "$1" ] || return 1
    local __line
    local soname
    while read __line; do
        [[ $__line =~ ^[\ $'\t']*([^\ $'\t'].*) ]] || continue
        __line="${BASH_REMATCH[1]}"
        [[ $__line =~ ^([-_.a-zA-Z0-9]+)[\ $'\t']([^\ $'\t'].*) ]] || \
            continue
        soname="${BASH_REMATCH[1]}"
        __line="${BASH_REMATCH[2]}"
        [[ $__line =~ not[\ $'\t']+found ]] && echo "$soname"
    done <<< "$(LANG=C LC_ALL=C ldd "$1" 2> /dev/null)"
}

#############################
# print
#############################

# tty and color
__istty=0
__use_color=0

check_istty() {
    [ -t 1 ] && {
        __istty=1
    } || {
        __istty=0
    }
    case "${_use_color}" in
        true)
            __use_color=1
            ;;
        false)
            __use_color=0
            ;;
        auto)
            __use_color=$__istty
            ;;
    esac
}

print_tty_ctrl() {
    ((__use_color)) || return
    echo -ne '\e['"${1}"'m'
}

replace_reset() {
    local line
    local IFS=$'\n'
    if [ ! -z "$1" ]; then
        while read line; do
            echo "${line//$'\e'[0m/$'\e'[${1}m}"
        done
        [ -z "${line}" ] || {
            echo -n "${line//$'\e'[0m/$'\e'[${1}m}"
        }
    else
        cat
    fi
}

__replace_line() {
    local IFS=$'\n'
    local __line=${1//\$\{/$'\n'$\{}
    shift
    local __varname
    echo "${__line}" | while read __line; do
        if [[ ${__line} =~ ^\$\{([_a-zA-Z0-9]+)\} ]]; then
            __varname="${BASH_REMATCH[1]}"
            echo -n "${__line/\$\{${__varname}\}/${!__varname}}"
        else
            echo -n "${__line}"
        fi
    done
    echo
}

__replace_vars() {
    local IFS=$'\n'
    local __line
    while read __line; do
        __replace_line "${__line}" "$@"
    done
    [ -z "${__line}" ] || {
        echo -n "$(__replace_line "${__line}" "$@")"
    }
}

print_eval() {
    echo "$1" | __replace_vars "${@:2}"
}

# print inline
code_inline() {
    print_tty_ctrl '01;36'
    echo -n '`'"$1"'`' | replace_reset '01;36'
    print_tty_ctrl '0'
}

print_link() {
    local text="$1"
    local url="$2"
    print_tty_ctrl '01;33'
    echo -n "[$text]($url)" | replace_reset '01;33'
    print_tty_ctrl '0'
}

escape_url_get() {
    local get="$1"
    echo -n "$get" | sed -e 's/&/%26/g' -e 's/+/%2B/g' -e 's/ /+/g'
}

print_google_link() {
    local text="$1"
    local url="https://www.google.com/search?q=$(escape_url_get "${text}")"
    print_link "${text}" "${url}"
}

print_not_found() {
    print_eval "$(_ '${1} not found.')" "$(code_inline $1)"
}

# indent levels and list index counters
__current_level=0
__list_indexes=(0)

set_cur_level() {
    local level="$1"
    local indexes=()
    local i
    if ((level >= 0)); then
        ((__current_level = level))
        for ((i = 0;i <= __current_level;i++)); do
            ((indexes[i] = __list_indexes[i]))
        done
        __list_indexes=("${indexes[@]}")
    else
        ((__current_level = 0))
        __list_indexes=()
    fi
}

increase_cur_level() {
    local level="$1"
    ((level = __current_level + level))
    set_cur_level "$level"
}

# print blocks
__need_blank_line=0

write_paragraph() {
    local str="$1"
    local p1="$2"
    local p2="$3"
    local code="$4"
    local prefix="$(repeat_str "${__current_level}" "    ")"
    local line
    local i=0
    local whole_prefix
    local IFS=$'\n'
    ((__need_blank_line)) && echo
    {
        while read line; do
            [ -z "${code}" ] || print_tty_ctrl "${code}"
            {
                ((i == 0)) && {
                    whole_prefix="${prefix}${p1}"
                } || {
                    whole_prefix="${prefix}${p2}"
                }
                ((i++))
                [ -z "${line}" ] && {
                    echo
                } || {
                    echo "${whole_prefix}${line}"
                }
            } | replace_reset "${code}"
            [ -z "${code}" ] || print_tty_ctrl "0"
        done
    } <<< "${str}"
    __need_blank_line=1
}

write_eval() {
    write_paragraph "$(print_eval "$@")"
}

write_error() {
    write_paragraph "**${1}**" "${2}" "${3}" '01;31'
}

write_error_eval() {
    write_error "$(print_eval "$@")"
}

write_quote_str() {
    local str="$1"
    increase_cur_level 1
    __need_blank_line=0
    echo
    write_paragraph "${str}" '' '' '01;35'
    echo
    __need_blank_line=0
    increase_cur_level -1
}

write_quote_cmd() {
    local cmd_output_str cmd_ret_val
    cmd_output_str="$("$@" 2>&1)"
    cmd_ret_val=$?
    write_quote_str "${cmd_output_str}"
    return $cmd_ret_val
}

write_title() {
    local level="$1"
    local title="$2"
    local prefix='######'
    prefix="${prefix::$level}"
    ((__need_blank_line)) && echo
    print_tty_ctrl '01;34'
    echo "${prefix} ${title}" | replace_reset '01;34'
    print_tty_ctrl '0'
    __need_blank_line=0
    set_cur_level -1
}

write_order_list() {
    local str="$1"
    local index
    increase_cur_level -1
    increase_cur_level 1
    ((index = ++__list_indexes[__current_level - 1]))
    ((${#index} > 2)) && index="${index: -2}"
    index="${index}.   "
    increase_cur_level -1
    write_paragraph "${str}" "${index::4}" '    ' '01;32'
    increase_cur_level 1
}

write_order_list_eval() {
    write_order_list "$(print_eval "$@")"
}

# write_list() {
#     local str="$1"
#     increase_cur_level -1
#     write_paragraph "${str}" '*   ' '    ' '01;32'
#     increase_cur_level 1
# }


#############################
# print tips and links
#############################

wiki_url="http://fcitx-im.org/wiki"

beginner_guide_link() {
    print_link "$(_ "Beginner's Guide")" \
        "${wiki_url}$(_ /Beginner%27s_Guide)"
}

set_env_link() {
    local env_name="$1"
    local value="$2"
    local fmt
    fmt=$(_ 'Please set environment variable ${env_name} to "${value}" using the tool your distribution provides or add ${1} to your ${2}. See ${link}.')
    local link
    link=$(print_link \
        "$(_ 'Input Method Related Environment Variables: ')${env_name}" \
        "${wiki_url}$(_ '/Input_method_related_environment_variables')#${env_name}")
    write_error_eval "${fmt}" "$(code_inline "export ${env_name}=${value}")" \
        "$(code_inline '~/.xprofile')"
}

gnome_36_check_gsettings() {
    gsettings get org.gnome.settings-daemon.plugins.keyboard \
        active 2> /dev/null || return 1
}

gnome_36_link() {
    # Do nothing if the DE is not gnome3
    maybe_gnome3 || return 1
    local link ibus_activated fmt
    link=$(print_link \
        "$(_ 'Note for GNOME Later than 3.6')" \
        "${wiki_url}$(_ '/Note_for_GNOME_Later_than_3.6')")

    # Check if the gsettings key exists
    if ibus_activated=$(gnome_36_check_gsettings); then
        [[ $ibus_activated = 'false' ]] && return 1
        g36_disable_ibus=$(code_inline 'gsettings set org.gnome.settings-daemon.plugins.keyboard active false')
        fmt=$(_ 'If you are using ${1}, you may want to uninstall ${2}, remove ${3} or use the command ${g36_disable_ibus} to disable IBus integration in order to use any input method other than ${2}. See ${link} for more detail.')
    else
        fmt=$(_ 'If you are using ${1}, you may want to uninstall ${2} or remove ${3} in order to use any input method other than ${2}. See ${link} for more detail as well as alternative solutions.')
    fi
    write_error_eval "${fmt}" "$(code_inline 'gnome>=3.6')" \
        "$(code_inline 'ibus')" "$(code_inline 'ibus-daemon')"
}

no_xim_link() {
    local fmt
    fmt=$(_ 'To see some application specific problems you may have when using xim, check ${link1}. For other more general problems of using XIM including application freezing, see ${link2}.')
    local link1
    link1=$(print_link \
        "$(_ 'Hall of Shame for Linux IME Support')" \
        "${wiki_url}$(_ '/Hall_of_Shame_for_Linux_IME_Support')")
    local link2
    link2=$(print_link \
        "$(_ 'here')" \
        "${wiki_url}$(_ '/XIM')")
    write_error_eval "${fmt}"
}


#############################
# system info
#############################

ldpaths=()
init_ld_paths() {
    local IFS=$'\n'
    unique_file_array ldpath ldpaths $(ldconfig -p 2> /dev/null | grep '=>' | \
        sed -e 's:.* => \(.*\)/[^/]*$:\1:g' | sort -u) \
        {/usr,,/usr/local}/lib*
}
init_ld_paths

fcitx_lib_path=()
init_fcitx_lib_path() {
    local path
    local __fcitx_lib_path
    fcitx_lib_path=(/usr/lib/fcitx5/ /usr/local/lib/fcitx5/ '@FCITX_INSTALL_ADDONDIR@')
}
init_fcitx_lib_path

find_fcitx_lib() {
    find_file "$1" -H "${fcitx_lib_path[@]}" -name "$2" -or -name "$2.so"
}

check_system() {
    write_title 1 "$(_ 'System Info:')"
    write_order_list "$(code_inline 'uname -a'):"
    if type uname &> /dev/null; then
        write_quote_cmd uname -a
    else
        write_error "$(print_not_found 'uname')"
    fi
    if type lsb_release &> /dev/null; then
        write_order_list "$(code_inline 'lsb_release -a'):"
        write_quote_cmd lsb_release -a
        write_order_list "$(code_inline 'lsb_release -d'):"
        write_quote_cmd lsb_release -d
    else
        write_order_list "$(code_inline lsb_release):"
        write_paragraph "$(print_not_found 'lsb_release')"
    fi
    write_order_list "$(code_inline /etc/lsb-release):"
    if [ -f /etc/lsb-release ]; then
        write_quote_cmd cat /etc/lsb-release
    else
        write_paragraph "$(print_not_found '/etc/lsb-release')"
    fi
    write_order_list "$(code_inline /etc/os-release):"
    if [ -f /etc/os-release ]; then
        write_quote_cmd cat /etc/os-release
    else
        write_paragraph "$(print_not_found '/etc/os-release')"
    fi
    write_order_list "$(_ 'Desktop Environment:')"
    # TODO check unity
    if [[ -z $DE ]] || [[ $DE = generic ]]; then
        write_eval "$(_ 'Cannot determine desktop environment.')"
    else
        write_eval "$(_ 'Desktop environment is ${1}.')" \
            "$(code_inline "${DE}")"
    fi
    write_order_list "$(_ 'XDG SESSION TYPE:')"
    write_quote_str "XDG_SESSION_TYPE='${XDG_SESSION_TYPE}'"
    write_order_list "$(_ 'Bash Version:')"
    write_quote_str "BASH_VERSION='${BASH_VERSION}'"
}

check_env() {
    write_title 1 "$(_ 'Environment:')"
    write_order_list "DISPLAY:"
    write_quote_str "DISPLAY='${DISPLAY}'"
    write_quote_str "WAYLAND_DISPLAY='${WAYLAND_DISPLAY}'"
    write_order_list "$(_ 'Keyboard Layout:')"
    increase_cur_level 1
    write_order_list "$(code_inline setxkbmap):"
    if type setxkbmap &> /dev/null; then
        write_quote_cmd setxkbmap -print
    else
        write_paragraph "$(print_not_found 'setxkbmap')"
    fi
    write_order_list "$(code_inline xprop):"
    if type xprop &> /dev/null; then
        write_quote_cmd xprop -root _XKB_RULES_NAMES
    else
        write_paragraph "$(print_not_found 'xprop')"
    fi
    increase_cur_level -1
    write_order_list "$(_ 'Locale:')"
    if type locale &> /dev/null; then
        increase_cur_level 1
        write_order_list "$(_ 'All locales:')"
        write_quote_str "$(locale -a 2> /dev/null)"
        write_order_list "$(_ 'Current locale:')"
        write_quote_str "$(locale 2> /dev/null)"
        locale_error="$(locale 2>&1 > /dev/null)"
        if [[ -n $locale_error ]]; then
            write_error_eval "$(_ 'Error occurs when running ${1}. Please check your locale settings.')" \
                "$(code_inline "locale")"
            write_quote_str "${locale_error}"
        fi
        increase_cur_level -1
    else
        write_paragraph "$(print_not_found 'locale')"
    fi
    write_order_list "$(_ 'Directories:')"
    increase_cur_level 1
    write_order_list "$(_ 'Home:')"
    write_quote_str ~
    write_order_list "$(code_inline '${XDG_CONFIG_HOME}'):"
    if [[ -z ${XDG_CONFIG_HOME} ]]; then
        write_eval "$(_ 'Environment variable ${1} is not set.')" \
            "$(code_inline 'XDG_CONFIG_HOME')"
    else
        write_eval \
            "$(_ 'Environment variable ${1} is set to ${2}.')" \
            "$(code_inline 'XDG_CONFIG_HOME')" \
            "$(code_inline "${XDG_CONFIG_HOME}")"
    fi
    write_eval "$(_ 'Current value of ${1} is ${2} (${3}).')" \
        "$(code_inline 'XDG_CONFIG_HOME')" \
        "$(code_inline "${xdg_conf_pretty_name}")" \
        "$(code_inline "${_xdg_conf_home}")"
    write_order_list_eval "$(_ '${1} Settings Directory:')" Fcitx5
    write_eval \
        "$(_ 'Current ${1} settings directory is ${2} (${3}).')" \
         fcitx5 \
        "$(code_inline "${fx_conf_pretty_name}")" \
        "$(code_inline "${fx_conf_home}")"
    increase_cur_level -1

    write_order_list "$(_ 'Current user:')"
    write_eval "$(_ 'The script is run as ${1} (${2}).')" \
        "${cur_user}" "${cur_uid}"
    if check_is_root; then
        increase_cur_level 1
        local has_sudo_var=0
        write_order_list_eval "$(_ '${1} Environment Variables:')" \
            "$(code_inline 'sudo')"
        check_sudo_env() {
            local env_name=${1}
            if [[ -n ${!env_name} ]]; then
                has_sudo_var=1
                write_eval "$(_ '${1} is set to ${2}.')" \
                    "${env_name}" "${!env_name}"
            else
                write_eval "$(_ '${1} is not set.')" "${env_name}"
            fi
        }
        check_sudo_env SUDO_COMMAND
        check_sudo_env SUDO_USER
        check_sudo_env SUDO_UID
        check_sudo_env SUDO_GID
        write_order_list "$(_ 'Running as root:')"
        if ((has_sudo_var)); then
            write_error_eval \
                "$(_ 'You are probably using ${1} to run this script. This means the result of this script may not be accurate. See ${2} for more information.')" \
                "$(code_inline 'sudo')" \
                "$(print_google_link "$(_ "sudo environment variables")")"
        else
            write_error_eval \
                "$(_ 'You are probably logging in as ${1} or using ${2} to run this script. This either means you have security problems or the result of this script may not be accurate. See ${3} or ${4} for more information.')" \
                "$(code_inline 'root')" "$(code_inline 'sudo')" \
                "$(print_google_link "$(_ "Why is it bad to run as root")")" \
                "$(print_google_link "$(_ "sudo environment variables")")"
        fi
        increase_cur_level -1
    fi
}

check_fcitx() {
    local IFS=$'\n'
    write_title 1 "$(_ 'Fcitx State:')"
    write_order_list "$(_ 'executable:')"
    if [[ -z "$fcitx_exe" ]]; then
        write_error "$(_ 'Cannot find fcitx5 executable!')"
        __need_blank_line=0
        write_error_eval "$(_ 'Please check ${1} for how to install fcitx5.')" \
                         "$(beginner_guide_link)"
        exit 1
    else
        write_eval "$(_ 'Found ${1} at ${2}.')" fcitx5 "$(code_inline "${fcitx_exe}")"
    fi
    write_order_list "$(_ 'version:')"
    version=$("$fcitx_exe" -v 2> /dev/null | \
        sed -e 's/.*fcitx version: \([0-9.]*\).*/\1/g')
    write_eval "$(_ 'Fcitx version: ${1}')" "$(code_inline "${version}")"
    write_order_list "$(_ 'process:')"
    psoutput=$(ps -Ao pid,comm)
    process=()
    while read line; do
        if [[ $line =~ ^([0-9]*)\ .*fcitx5.* ]]; then
            [ "${BASH_REMATCH[1]}" = "$$" ] && continue
            array_push process "${line}"
        fi
    done <<< "${psoutput}"
    if ! ((${#process[@]})); then
        write_error_eval "$(_ '${1} is not running.')" Fcitx5
        __need_blank_line=0
        write_error_eval "$(_ 'Please check the Configure link of your distribution in ${1} for how to setup ${2} autostart.')" "$(beginner_guide_link)" fcitx5
        return 1
    fi
    local pcount="${#process[@]}"
    if ((pcount > 1)); then
        write_eval "$(_ 'Found ${1} ${2} processes:')" "${#process[@]}" fcitx5
    else
        write_eval "$(_ 'Found ${1} ${2} process:')" "${#process[@]}" fcitx5
    fi
    write_quote_cmd print_array "${process[@]}"
    write_order_list "$(code_inline "fcitx5-remote"):"
    if type fcitx5-remote &> /dev/null; then
        if ! fcitx5-remote &> /dev/null; then
            write_error_eval "$(_ 'Cannot connect to ${1} correctly.')" fcitx5
        else
            write_eval "$(_ '${1} works properly.')" \
                       "$(code_inline "fcitx5-remote")"
        fi
    else
        write_error "$(print_not_found "fcitx5-remote")"
    fi
    write_order_list "$(_ "DBus interface:")"
    local dbus_name=org.fcitx.Fcitx5
    if [[ -n ${dbus_exe} ]]; then
        write_eval  "$(_ 'Using ${1} to check dbus.')" \
                    "$(code_inline "${dbus_exe}")"
        owner_name=$(dbus_get_name_owner ${dbus_name})
        owner_pid=$(dbus_get_pid ${dbus_name})
        if [[ -n ${owner_name} ]]; then
            write_eval  "$(_ 'Owner of DBus name ${1} is ${2}.')" \
                        "$(code_inline "${dbus_name}")" \
                        "$(code_inline "${owner_name}")"
        else
            write_error_eval "$(_ 'Cannot find DBus name ${1} owner.')" \
                             "$(code_inline "${dbus_name}")"
        fi
        if [[ -n ${owner_pid} ]]; then
            write_eval  "$(_ 'PID of DBus name ${1} owner is ${2}.')" \
                        "$(code_inline "${dbus_name}")" \
                        "$(code_inline "${owner_pid}")"
        else
            write_error_eval "$(_ 'Cannot find pid of DBus name ${1} owner.')" \
                             "$(code_inline "${dbus_name}")"
        fi
        if [[ -n ${owner_name} ]]; then
            write_eval "$(_ 'Debug information from dbus:')"
            write_quote_str "$(dbus_debuginfo)"
        fi
    else
        write_error "$(_ "Unable to find a program to check dbus.")"
    fi
}

_find_config_gtk() {
    [ -n "${_config_tool_gtk_exe}" ] && {
        echo "${_config_tool_gtk_exe}"
        return 0
    }
    local config_gtk
    config_gtk="$(command -v "fcitx5-config-gtk" 2> /dev/null)" || return 1
    echo "${config_gtk}"
    _config_tool_gtk_exe="${config_gtk}"
}

_check_config_gtk_version() {
    local version=$1
    local config_gtk
    [ -z "${_config_tool_gtk_version}" ] && {
        config_gtk="$(_find_config_gtk)" || return 1
        ld_info="$(ldd "$config_gtk" 2> /dev/null)" ||
        ld_info="$(objdump -p "$config_gtk" 2> /dev/null)" || return 1
        if [[ $ld_info =~ libgtk[-._a-zA-Z0-9]*3[-._a-zA-Z0-9]*\.so ]]; then
            _config_tool_gtk_version=3
        elif [[ $ld_info =~ libgtk[-._a-zA-Z0-9]*2[-._a-zA-Z0-9]*\.so ]]; then
            _config_tool_gtk_version=2
        else
            return 1
        fi
    }
    [ "${_config_tool_gtk_version}" = "$version" ]
}

_check_config_gtk() {
    local version=$1
    local config_gtk config_gtk_name
    write_order_list_eval "$(_ 'Config GUI for gtk${1}:')" "${version}"
    if ! config_gtk="$(command -v "fcitx5-config-gtk${version}" 2> /dev/null)"; then
        if ! _check_config_gtk_version "${version}"; then
            write_error_eval \
                "$(_ 'Config GUI for gtk${1} not found.')" "${version}"
            return 1
        else
            config_gtk=$(_find_config_gtk)
            config_gtk_name="fcitx5-config-gtk"
        fi
    else
        config_gtk_name="fcitx5-config-gtk${version}"
    fi
    write_eval "$(_ 'Found ${1} at ${2}.')" \
        "$(code_inline "${config_gtk_name}")" \
        "$(code_inline "${config_gtk}")"
}

_check_config_qt() {
    local config_qt config_qt_name
    config_qt_name="fcitx5-config-qt"
    write_order_list_eval "$(_ 'Config GUI for qt:')" "${version}"
    if ! config_qt="$(command -v "${config_qt_name}" 2> /dev/null)"; then
        write_error "$(_ 'Config GUI for qt not found.')"
        return 1
    fi
    write_eval "$(_ 'Found ${1} at ${2}.')" \
        "$(code_inline "${config_qt_name}")" \
        "$(code_inline "${config_qt}")"
}

_check_config_kcm() {
    local version=$1
    local kcm_shell config_kcm
    write_order_list "$(_ 'Config GUI for kde:')"
    if ! kcm_shell="$(command -v "kcmshell${version}" 2> /dev/null)"; then
        write_error "$(print_not_found "kcmshell${version}")"
        return 1
    fi
    config_kcm="$(${kcm_shell} --list 2> /dev/null | grep -i fcitx5)" && {
        write_eval "$(_ 'Found ${1} kcm module.')" fcitx5
        write_quote_str "${config_kcm}"
        return 0
    }
    return 1
}

check_config_ui() {
    local IFS=$'\n'
    write_title 1 "$(_ 'Fcitx Configure UI:')"
    write_order_list "$(_ 'Config Tool Wrapper:')"
    if ! fcitx_configtool="$(command -v fcitx5-configtool 2> /dev/null)"; then
        write_error_eval "$(_ 'Cannot find ${1} executable!')" fcitx5-configtool
    else
        write_eval "$(_ 'Found ${1} at ${2}.')" \
                   fcitx5-configtool \
                   "$(code_inline "${fcitx_configtool}")"
    fi
    local config_backend_found=0
    _check_config_qt && config_backend_found=1
    _check_config_kcm 5 && config_backend_found=1
    if ((!config_backend_found)) && [[ -n "$DISPLAY$WAYLAND_DISPLAY" ]]; then
        write_error_eval "$(_ 'Cannot find a GUI config tool, please install one of ${1}, or ${2}.')" \
                         "$(code_inline kcm-fcitx5)" "$(code_inline fcitx5-config-qt)"
    fi
}


#############################
# front end
#############################

_env_correct() {
    write_eval \
        "$(_ 'Environment variable ${1} is set to "${2}" correctly.')" \
        "$1" "$2"
}

_env_incorrect() {
    write_error_eval \
        "$(_ 'Environment variable ${1} is "${2}" instead of "${3}". Please check if you have exported it incorrectly in any of your init files.')" \
        "$1" "$3" "$2"
}

check_xim() {
    write_title 2 "Xim:"
    xim_name=fcitx
    write_order_list "$(code_inline '${XMODIFIERS}'):"
    if [ -z "${XMODIFIERS}" ]; then
        write_error_eval "$(_ 'XMODIFIERS is not set')"
        set_env_link XMODIFIERS '@im=fcitx'
        __need_blank_line=0
    elif [ "${XMODIFIERS}" = '@im=fcitx' ]; then
        _env_correct 'XMODIFIERS' '@im=fcitx'
        __need_blank_line=0
    else
        _env_incorrect 'XMODIFIERS' '@im=fcitx' "${XMODIFIERS}"
        set_env_link XMODIFIERS '@im=fcitx'
        if [[ ${XMODIFIERS} =~ @im=([-_0-9a-zA-Z]+) ]]; then
            xim_name="${BASH_REMATCH[1]}"
        else
            __need_blank_line=0
            write_error_eval "$(_ 'Cannot interpret XMODIFIERS: ${1}.')" \
                "${XMODIFIERS}"
        fi
        if [[ ${xim_name} = ibus ]]; then
            __need_blank_line=0
            gnome_36_link || __need_blank_line=1
        fi
    fi
    write_eval "$(_ 'Xim Server Name from Environment variable is ${1}.')" \
        "${xim_name}"
    write_order_list "$(_ 'XIM_SERVERS on root window:')"
    local atom_name=XIM_SERVERS
    if ! type xprop &> /dev/null; then
        write_error "$(print_not_found 'xprop')"
    else
        xprop=$(xprop -root -notype -f "${atom_name}" \
            '32a' ' $0\n' "${atom_name}" 2> /dev/null)
        if [[ ${xprop} =~ ^${atom_name}\ @server=(.*)$ ]]; then
            xim_server_name="${BASH_REMATCH[1]}"
            if [ "${xim_server_name}" = "${xim_name}" ]; then
                write_paragraph "$(_ 'Xim server name is the same with that set in the environment variable.')"
            else
                write_error_eval "$(_ 'Xim server name: "${1}" is different from that set in the environment variable: "${2}".')" \
                    "${xim_server_name}" "${xim_name}"
            fi
        else
            write_error "$(_ 'Cannot find xim_server on root window.')"
        fi
    fi
    local _LC_CTYPE=$(get_locale CTYPE)
    # TODO: this is actually caused by font issue and hopefully it'll be fixed for emacs
    # in the future.
    if type emacs &> /dev/null &&
        ! str_match_regex '^(zh|ja|ko)([._].*|)$' "${_LC_CTYPE}"; then
        write_order_list "$(_ 'XIM for Emacs:')"
        write_error_eval \
            "$(_ 'Your LC_CTYPE is set to ${1} instead of one of zh, ja, ko. You may not be able to use input method in emacs because of an really old emacs bug that upstream refuse to fix for years.')" "${_LC_CTYPE}"
    fi
    if ! str_match_regex '.[Uu][Tt][Ff]-?8$' "${_LC_CTYPE}"; then
        write_order_list "$(_ 'XIM encoding:')"
        write_error_eval \
            "$(_ 'Your LC_CTYPE is set to ${1} whose encoding is not UTF-8. You may have trouble committing strings using XIM.')" "${_LC_CTYPE}"
    fi
}

_check_toolkit_env() {
    local name="$1"
    local env_names=("${@:2}")
    local env_name
    write_order_list "${name} - $(code_inline '${'"${env_names[0]}"'}'):"
    for env_name in "${env_names[@]}"; do
        [ -z "${!env_name}" ] || break
    done
    if [ -z "${!env_name}" ]; then
        set_env_link "${env_name}" 'fcitx'
        if [ "${name}" != qt4 ]; then
            write_error_eval "$(_ 'It is OK to use ${1} built-in Wayland im module if your compositor fully supports text-input protocol used by ${1}.')" "${name}"
        fi
    elif [ "${!env_name}" = 'fcitx' ]; then
        _env_correct "${env_name}" 'fcitx'
    else
        _env_incorrect "${env_name}" 'fcitx' "${!env_name}"
        __need_blank_line=0
        if [ "${!env_name}" = 'xim' ]; then
            write_error_eval "$(_ 'You are using xim in ${1} programs.')" \
                "${name}"
            no_xim_link
        else
            write_error_eval \
                "$(_ 'You may have trouble using fcitx in ${1} programs.')" \
                "${name}"
            if [ "${!env_name}" = "ibus" ] && [ "${name}" = 'qt' ]; then
                __need_blank_line=0
                gnome_36_link || __need_blank_line=1
            fi
        fi
        set_env_link "${env_name}" 'fcitx'
    fi
}

find_qt_modules() {
    local qt_dirs _qt_modules
    find_file qt_dirs -H "${ldpaths[@]}" -type d -name '*qt*'
    find_file _qt_modules -H "${qt_dirs[@]}" -type f -iname '*fcitx*.so'
    qt_modules=()
    unique_file_array qt_modules qt_modules "${_qt_modules[@]}"
}

check_qt() {
    write_title 2 "Qt:"
    _check_toolkit_env qt4 QT4_IM_MODULE QT_IM_MODULE
    _check_toolkit_env qt5 QT_IM_MODULE
    find_qt_modules
    qt4_module_found=''
    qt5_module_found=''
    qt6_module_found=''
    write_order_list "$(_ 'Qt IM module files:')"
    echo
    for file in "${qt_modules[@]}"; do
        basename=$(basename "${file}")
        __need_blank_line=0
        qt_version=qt
        if [[ $file =~ qt5 ]]; then
            qt_version=qt5
        elif [[ $file =~ qt6 ]]; then
            qt_version=qt6
        fi

        if [[ ${basename} =~ im-fcitx5 ]] &&
            [[ ${file} =~ plugins/inputmethods ]]; then
            write_eval "$(_ 'Found ${3} im module for ${2}: ${1}.')" \
                "$(code_inline "${file}")" qt4 fcitx5
            qt4_module_found=1
        elif [[ ${basename} =~ fcitx5platforminputcontextplugin ]] &&
            [[ ${file} =~ plugins/platforminputcontexts ]]; then
            write_eval "$(_ 'Found ${3} im module for ${2}: ${1}.')" \
                "$(code_inline "${file}")" ${qt_version} fcitx5
            if [[ ${qt_version} != "qt6" ]]; then
                qt5_module_found=1
            else
                qt6_module_found=1
            fi
        elif [[ ${file} =~ /fcitx5/qt ]]; then
            write_eval "$(_ 'Found ${1} ${2} module: ${3}.')" \
                        fcitx5 ${qt_version} "$(code_inline "${file}")"
        else
            write_eval "$(_ 'Found unknown ${1} qt module: ${2}.')" \
                       fcitx \
                       "$(code_inline "${file}")"
        fi
    done
    write_eval "$(_ 'Following error may not be accurate because guessing Qt version from path depends on how your distribution packages Qt. It is not a critical error if you do not use any Qt application with certain version of Qt or you are using text-input support by Qt under Wayland.')"
    if [ -z "${qt4_module_found}" ]; then
        __need_blank_line=0
        write_error_eval \
            "$(_ 'Cannot find ${1} input method module for ${2}.')" fcitx5 Qt4
    fi
    if [ -z "${qt5_module_found}" ]; then
        __need_blank_line=0
        write_error_eval \
            "$(_ 'Cannot find ${1} input method module for ${2}.')" fcitx5 Qt5
    fi
    if [ -z "${qt6_module_found}" ]; then
        __need_blank_line=0
        write_error_eval \
            "$(_ 'Cannot find ${1} input method module for ${2}.')" fcitx5 Qt6
    fi
}

init_gtk_dirs() {
    local version="$1"
    local gtk_dirs_name="__gtk${version}_dirs"
    eval '((${#'"${gtk_dirs_name}"'[@]}))' || {
        find_file "${gtk_dirs_name}" -H "${ldpaths[@]}" -type d \
            '(' -name "gtk-${version}*" -o -name 'gtk' ')'
    }
    eval 'gtk_dirs=("${'"${gtk_dirs_name}"'[@]}")'
}

find_gtk_query_immodules() {
    gtk_query_immodules=()
    local version="$1"
    init_gtk_dirs "${version}"
    [ "${#gtk_dirs[@]}" = 0 ] && return
    local IFS=$'\n'
    local query_im_lib
    find_file query_im_lib -H "${gtk_dirs[@]}" -type f \
        -name "gtk-query-immodules-${version}*"
    unique_file_array "gtk_query_immodules_${version}" gtk_query_immodules \
        $(find_in_path "gtk-query-immodules-${version}*") \
        "${query_im_lib[@]}"
}

reg_gtk_query_output() {
    local version="$1"
    while read line; do
        regex='"(/[^"]*\.so)"'
        [[ $line =~ $regex ]] || continue
        file=${BASH_REMATCH[1]}
        add_and_check_file "__gtk_immodule_files_${version}" "${file}" && {
            array_push "gtk_immodule_files_${version}" "${file}"
        }
    done <<< "$2"
}

reg_gtk_query_output_gio() {
    local version="$1"
    local dir="$3"
    while read line; do
        regex='(lib[^ /]*.so)'
        [[ $line =~ $regex ]] || continue
        file="$dir/${BASH_REMATCH[1]}"
        add_and_check_file "__gtk_immodule_files_${version}" "${file}" && {
            array_push "gtk_immodule_files_${version}" "${file}"
        }
    done <<< "$2"
}

check_gtk_immodule_file() {
    local version=$1
    local gtk_immodule_files
    local all_exists=1
    write_order_list "gtk ${version}:"
    eval 'gtk_immodule_files=("${gtk_immodule_files_'"${version}"'[@]}")'
    for file in "${gtk_immodule_files[@]}"; do
        [[ -f "${file}" ]] || {
            all_exists=0
            write_error_eval \
                "$(_ 'Gtk ${1} immodule file ${2} does not exist.')" \
                "${version}" \
                "${file}"
        }
    done
    ((all_exists)) && \
        write_eval "$(_ 'All found Gtk ${1} immodule files exist.')" \
        "${version}"
}

check_gtk_query_immodule() {
    local version="$1"
    local IFS=$'\n'
    find_gtk_query_immodules "${version}"
    local module_found=0
    local query_found=0
    write_order_list "gtk ${version}:"

    for query_immodule in "${gtk_query_immodules[@]}"; do
        query_output=$("${query_immodule}")
        real_version=''
        version_line=''
        while read line; do
            regex='[Cc]reated.*gtk-query-immodules.*gtk\+-*([0-9][^ ]+)$'
            [[ $line =~ $regex ]] && {
                real_version="${BASH_REMATCH[1]}"
                version_line="${line}"
                break
            }
        done <<< "${query_output}"
        if [[ -n $version_line ]]; then
            regex="^${version}\."
            if [[ $real_version =~ $regex ]]; then
                query_found=1
                write_command=write_eval
            else
                write_command=write_error_eval
            fi
            "$write_command" \
                "$(_ 'Found ${3} for gtk ${1} at ${2}.')" \
                "$(code_inline "${real_version}")" \
                "$(code_inline "${query_immodule}")" \
                "$(code_inline gtk-query-immodules)"
                __need_blank_line=0
                write_eval "$(_ 'Version Line:')"
                write_quote_str "${version_line}"
        else
            write_eval "$(_ 'Found ${2} for unknown gtk version at ${1}.')" \
                "$(code_inline "${query_immodule}")" \
                "$(code_inline gtk-query-immodules)"
            real_version=${version}
        fi
        if fcitx_gtk=$(grep fcitx5 <<< "${query_output}"); then
            module_found=1
            __need_blank_line=0
            write_eval "$(_ 'Found ${1} im modules for gtk ${2}.')" \
                       fcitx5 "$(code_inline ${real_version})"
            write_quote_str "${fcitx_gtk}"
            reg_gtk_query_output "${version}" "${fcitx_gtk}"
        else
            write_error_eval \
                "$(_ 'Failed to find ${1} in the output of ${2}')" \
                fcitx5 "$(code_inline "${query_immodule}")"
        fi
    done
    ((query_found)) || {
        write_error_eval \
            "$(_ 'Cannot find ${2} for gtk ${1}')" \
            "${version}" \
            "$(code_inline gtk-query-immodules)"
    }
    ((module_found)) || {
        write_error_eval \
            "$(_ 'Cannot find ${1} im module for gtk ${2}.')" \
            fcitx5 "${version}"
    }
}

find_gtk_immodules_cache() {
    local version="$1"
    init_gtk_dirs "${version}"
    [ "${#gtk_dirs[@]}" = 0 ] && return
    local IFS=$'\n'
    local __gtk_immodule_cache
    find_file __gtk_immodule_cache -H \
        "${gtk_dirs[@]}" /etc/gtk-${version}* -type f \
        '(' -name '*gtk.immodules*' -o -name '*immodules.cache*' ')'
    unique_file_array "gtk_immodules_cache_${version}" "$2" \
        "${__gtk_immodule_cache[@]}"
}

check_gtk_immodule_cache() {
    local version="$1"
    local IFS=$'\n'
    local cache_found=0
    local module_found=0
    local version_correct=0
    write_order_list "gtk ${version}:"
    local gtk_immodules_cache
    find_gtk_immodules_cache "${version}" gtk_immodules_cache

    for cache in "${gtk_immodules_cache[@]}"; do
        cache_content=$(cat "${cache}")
        real_version=''
        version_line=''
        version_correct=0
        while read line; do
            regex='[Cc]reated.*gtk-query-immodules.*gtk\+-*([0-9][^ ]+)$'
            [[ $line =~ $regex ]] && {
                real_version="${BASH_REMATCH[1]}"
                version_line="${line}"
                break
            }
        done <<< "${cache_content}"
        if [[ -n $version_line ]]; then
            regex="^${version}\."
            if [[ $real_version =~ $regex ]]; then
                cache_found=1
                version_correct=1
                write_command=write_eval
            else
                write_command=write_error_eval
            fi
            "$write_command" \
                "$(_ 'Found immodules cache for gtk ${1} at ${2}.')" \
                "$(code_inline ${real_version})" \
                "$(code_inline "${cache}")"
                __need_blank_line=0
                write_eval "$(_ 'Version Line:')"
                write_quote_str "${version_line}"
        else
            write_eval \
                "$(_ 'Found immodule cache for unknown gtk version at ${1}.')" \
                "$(code_inline "${cache}")"
            real_version=${version}
        fi
        if fcitx_gtk=$(grep fcitx5 <<< "${cache_content}"); then
            ((version_correct)) && module_found=1
            __need_blank_line=0
            write_eval "$(_ 'Found ${1} im modules for gtk ${2}.')" \
                       fcitx5 "$(code_inline ${real_version})"
            write_quote_str "${fcitx_gtk}"
            reg_gtk_query_output "${version}" "${fcitx_gtk}"
        else
            write_error_eval \
                "$(_ 'Failed to find ${1} in immodule cache at ${2}')" \
                fcitx5 "$(code_inline "${cache}")"
        fi
    done
    ((cache_found)) || {
        write_error_eval \
            "$(_ 'Cannot find immodules cache for gtk ${1}')" \
            "${version}"
    }
    ((module_found)) || {
        write_error_eval \
            "$(_ 'Cannot find ${1} im module for gtk ${2} in cache.')" \
            fcitx5 "${version}"
    }
}

find_gtk_immodules_cache_gio() {
    local version="$1"
    init_gtk_dirs "${version}"
    [ "${#gtk_dirs[@]}" = 0 ] && return
    local IFS=$'\n'
    local __gtk_immodule_cache
    find_file __gtk_immodule_cache -H \
        "${gtk_dirs[@]}" /etc/gtk-${version}* -type f -path '*/immodules/giomodule.cache'
    unique_file_array "gtk_immodules_cache_${version}" "$2" \
        "${__gtk_immodule_cache[@]}"
}

check_gtk() {
    write_title 2 "Gtk:"
    _check_toolkit_env gtk GTK_IM_MODULE
    write_order_list "$(code_inline gtk-query-immodules):"
    increase_cur_level 1
    check_gtk_query_immodule 2
    check_gtk_query_immodule 3
    increase_cur_level -1
    write_order_list "$(_ 'Gtk IM module cache:')"
    increase_cur_level 1
    check_gtk_immodule_cache 2
    check_gtk_immodule_cache 3
    increase_cur_level -1
    write_order_list "$(_ 'Gtk IM module files:')"
    increase_cur_level 1
    check_gtk_immodule_file 2
    check_gtk_immodule_file 3
    check_gtk_immodule_file 4
    increase_cur_level -1
}


#############################
# fcitx modules
#############################

check_modules() {
    write_title 2 "$(_ 'Fcitx Addons:')"
    write_order_list "$(_ 'Addon Config Dir:')"
    local addon_conf_dir='@FCITX_INSTALL_PKGDATADIR@/addon'
    local enabled_addon=()
    local disabled_addon=()
    local enabled_ui_name=()
    local enabled_ui=()
    local addon_file
    local addon_version
    local name
    local version
    local id
    local enable
    local _enable
    local disabled_addon_config
    local addon_name
    declare -A addon_file
    declare -A disabled_addon_config
    declare -A addon_version
    if [ ! -d "${addon_conf_dir}" ]; then
      write_error_eval "$(_ 'Cannot find ${1} addon config directory.')" fcitx5
      return
    fi
    write_eval "$(_ 'Found ${1} addon config directory: ${2}.')" \
               fcitx5 "$(code_inline "${addon_conf_dir}")"
    if [[ -f "${fx_conf_home}/config" ]]; then
        get_config_section disabled_addon_config \
                           "${fx_conf_home}/config" Behavior/DisabledAddons
    fi
    write_order_list "$(_ 'Addon List:')"
    for file in "${addon_conf_dir}"/*.conf; do
        id=${file##*/}
        id=${id%.conf}
        if ! name=$(get_from_config_file "${file}" Name); then
            write_error_eval \
                "$(_ 'Invalid addon config file ${1}.')" \
                "$(code_inline "${file}")"
            continue
        fi
        if ! version=$(get_from_config_file "${file}" Version); then
            version=
        fi
        addon_version["${name}"]="${version}"
        enable=
        # This is O(M*N) but I don't care...
        for addon_name in "${disabled_addon_config[@]}"; do
            if [[ $addon_name = $id ]]; then
                enable=False
                break
            fi
        done
        if [ "$(echo "${enable}" | sed -e 's/.*/\L&/g')" = false ]; then
            array_push disabled_addon "${name}"
        else
            array_push enabled_addon "${name}"
            if [[ $(get_from_config_file "${file}" Category) = UI ]]; then
                array_push enabled_ui_name "${name}"
                array_push enabled_ui "${id}"
            fi
        fi
        type=$(get_from_config_file "${file}" Type)
        if [[ -z $type ]] || [[ $type = SharedLibrary ]]; then
            addon_file_name="$(get_from_config_file "${file}" Library)"
            addon_file["${name}"]="${addon_file_name/export:/}"
        fi
    done
    increase_cur_level 1
    write_order_list_eval "$(_ 'Found ${1} enabled addons:')" \
        "${#enabled_addon[@]}"
    [ "${#enabled_addon[@]}" = 0 ] || {
        local addon_list=()
        for addon_name in "${enabled_addon[@]}"; do
            array_push addon_list "${addon_name} ${addon_version[${addon_name}]}"
        done
        write_quote_cmd print_array "${addon_list[@]}"
    }
    write_order_list_eval "$(_ 'Found ${1} disabled addons:')" \
        "${#disabled_addon[@]}"
    [ "${#disabled_addon[@]}" = 0 ] || {
        local addon_list=()
        for addon_name in "${disabled_addon[@]}"; do
            array_push addon_list "${addon_name} ${addon_version[${addon_name}]}"
        done
        write_quote_cmd print_array "${addon_list[@]}"
    }
    increase_cur_level -1
    write_order_list_eval "$(_ 'Addon Libraries:')"
    local all_module_ok=1
    local module_files
    local _module_file
    for addon_name in "${!addon_file[@]}"; do
        find_fcitx_lib module_files "${addon_file[${addon_name}]}"
        if [[ ${#module_files[@]} = 0 ]]; then
            write_error_eval \
                "$(_ 'Cannot find file ${1} of addon ${2}.')" \
                "$(code_inline "${addon_file[${addon_name}]}")" \
                "$(code_inline ${addon_name})"
            all_module_ok=0
            continue
        fi
        for _module_file in "${module_files[@]}"; do
            not_found="$(run_ldd "${_module_file}")"
            [[ -z ${not_found} ]] && continue
            write_error_eval \
                "$(_ 'Cannot find following required libraries for ${1} of addon ${2}.')" \
                "$(code_inline "${addon_file[${addon_name}]}")" \
                "$(code_inline ${addon_name})"
            write_quote_str "${not_found}"
            all_module_ok=0
        done
    done
    ((all_module_ok)) && \
        write_eval "$(_ 'All libraries for all addons are found.')"
    write_order_list_eval "$(_ 'User Interface:')"
    if ! ((${#enabled_ui[@]})); then
        write_error_eval "$(_ 'Cannot find enabled ${1} user interface!')" fcitx5
    else
        write_eval "$(_ 'Found ${1} enabled user interface addons:')" \
            "${#enabled_ui[@]}"
        write_quote_cmd print_array "${enabled_ui_name[@]}"
        has_non_kimpanel=0
        has_kimpanel_dbus=0
        for ui in "${enabled_ui[@]}"; do
            if [[ $ui =~ kimpanel ]]; then
                pid=$(dbus_get_pid org.kde.impanel) || continue
                has_kimpanel_dbus=1
                write_eval "$(_ 'Kimpanel process:')"
                write_quote_cmd print_process_info "${pid}"
            else
                has_non_kimpanel=1
            fi
        done
        ((has_non_kimpanel)) || ((has_kimpanel_dbus)) || \
            write_error_eval \
            "$(_ 'Cannot find kimpanel dbus interface or enabled non-kimpanel user interface.')"
    fi
    increase_cur_level -1
}

check_input_methods() {
    write_title 2 "$(_ 'Input Methods:')"
    write_order_list "$(code_inline "${fx_conf_home}/profile"):"
    if [ -f "${fx_conf_home}/profile" ]; then
        write_quote_cmd cat "${fx_conf_home}/profile"
    else
        write_paragraph \
            "$(print_not_found "${fx_conf_home}/profile")"
    fi
}


#############################
# log
#############################

check_log() {
    local logdir
    write_order_list "$(code_inline 'date'):"
    if type date &> /dev/null; then
        write_quote_cmd date
    else
        write_error "$(print_not_found 'date')"
    fi
    logdir="${fx_conf_home}"
    write_order_list "$(code_inline "${logdir}/crash.log"):"
    if [ -f "${logdir}/crash.log" ]; then
        write_quote_cmd cat "${logdir}/crash.log"
    else
        write_paragraph \
            "$(print_not_found "${logdir}/crash.log")"
    fi
}


#############################
# cmd line
#############################

_check_frontend=1
_check_modules=1
_check_log=1

_use_color=auto

while true; do
    (($#)) || break
    arg=$1
    shift
    case "${arg}" in
        --color=@(never|false))
        _use_color=false
        ;;
        --color=auto)
            _use_color=auto
            ;;
        --color@(|=*))
        _use_color=true
        ;;
    esac
done

[ -z "$1" ] || exec > "$1"


#############################
# init output
#############################

check_istty


#############################
# run
#############################

check_system
check_env
check_fcitx
check_config_ui

((_check_frontend)) && {
    write_title 1 "$(_ 'Frontends setup:')"
    check_xim
    check_qt
    check_gtk
}

((_check_modules)) && {
    write_title 1 "$(_ 'Configuration:')"
    check_modules
    check_input_methods
}

((_check_log)) && {
    write_title 1 "$(_ 'Log:')"
    check_log
}

set_cur_level -1
write_error "$(_ 'Warning: the output of fcitx5-diagnose contains sensitive information, including the distribution name, kernel version, name of currently running programs, etc.')"
write_error "$(_ 'Though such information can be helpful to developers for diagnostic purpose, please double check and remove as necessary before posting it online publicly.')"
