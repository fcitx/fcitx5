#!/bin/sh
#--------------------------------------
# fcitx-configtool
#

export TEXTDOMAIN=fcitx5

if command -v kdialog > /dev/null 2>&1; then
    message() {
        kdialog --msgbox "$1"
    }
    error() {
        kdialog --error "$1"
    }
elif command -v zenity > /dev/null 2>&1; then
    message() {
        zenity --info --text="$1"
    }
    error() {
        zenity --error --text="$1"
    }
else
    message() {
        echo "$1"
    }
    error() {
        echo "$1" >&2
    }
fi

if command -v gettext > /dev/null 2>&1; then
    _() {
        gettext "$@"
    }
else
    _() {
        echo "$@"
    }
fi



# from xdg-open

detectDE() {
    # see https://bugs.freedesktop.org/show_bug.cgi?id=34164
    unset GREP_OPTIONS

    local desktop
    if [ -n "${XDG_CURRENT_DESKTOP}" ]; then
      for desktop in $(echo "${XDG_CURRENT_DESKTOP}" | tr ":" "\n"); do
        case "${desktop}" in
            GNOME)
            DE=gnome
            break
            ;;
            KDE)
            DE=kde
            break
            ;;
            LXDE)
            DE=lxde
            break
            ;;
            XFCE)
            DE=xfce
            break
        esac
      done
    fi

    if [ x"$DE" = x"" ]; then
      # classic fallbacks
      if [ x"$KDE_FULL_SESSION" = x"true" ]; then DE=kde;
      elif xprop -root KDE_FULL_SESSION 2> /dev/null | grep ' = \"true\"$' > /dev/null 2>&1; then DE=kde;
      elif [ x"$GNOME_DESKTOP_SESSION_ID" != x"" ]; then DE=gnome;
      elif [ x"$MATE_DESKTOP_SESSION_ID" != x"" ]; then DE=mate;
      elif dbus-send --print-reply --dest=org.freedesktop.DBus /org/freedesktop/DBus org.freedesktop.DBus.GetNameOwner string:org.gnome.SessionManager > /dev/null 2>&1 ; then DE=gnome;
      elif xprop -root _DT_SAVE_MODE 2> /dev/null | grep ' = \"xfce4\"$' >/dev/null 2>&1; then DE=xfce;
      elif xprop -root 2> /dev/null | grep -i '^xfce_desktop_window' >/dev/null 2>&1; then DE=xfce
      fi
    fi

    if [ x"$DE" = x"" ]; then
      # fallback to checking $DESKTOP_SESSION
      case "$DESKTOP_SESSION" in
         gnome)
           DE=gnome;
           ;;
         LXDE|Lubuntu)
           DE=lxde;
           ;;
         xfce|xfce4|'Xfce Session')
           DE=xfce;
           ;;
      esac
    fi

    if [ x"$DE" = x"" ]; then
      # fallback to uname output for other platforms
      case "$(uname 2>/dev/null)" in
        Darwin)
          DE=darwin;
          ;;
      esac
    fi

    if [ x"$DE" = x"gnome" ]; then
      # gnome-default-applications-properties is only available in GNOME 2.x
      # but not in GNOME 3.x
      command -v gnome-default-applications-properties > /dev/null 2>&1  || DE="gnome3"
    fi
}

run_kde() {
    if (systemsettings --list 2>/dev/null | grep ^kcm_fcitx5 > /dev/null 2>&1); then
        exec systemsettings kcm_fcitx5
    fi
    if (systemsettings5 --list 2>/dev/null | grep ^kcm_fcitx5 > /dev/null 2>&1); then
        exec systemsettings5 kcm_fcitx5
    fi
}

run_qt() {
    if command -v fcitx5-config-qt > /dev/null 2>&1; then
        exec fcitx5-config-qt
    fi
    return 1
}

run_xdg() {
    case "$DE" in
        kde)
            message "$(_ "You're currently running KDE, but KCModule for fcitx couldn't be found, the package name of this KCModule is usually kcm-fcitx or kde-config-fcitx. Now it will open config directory.")"
            ;;
        *)
            message "$(_ "You're currently running Fcitx with GUI, but fcitx5-config-qt couldn't be found. Now it will open config directory.")"
            ;;
    esac

    if command="$(command -v xdg-open 2>/dev/null)"; then
        exec "$command" "$HOME/.config/fcitx5"
    fi
}

_which_cmdline() {
    cmd="$(command -v "$1")" || return 1
    shift
    echo "$cmd $*"
}

detectDE

# even if we are not on KDE, we should still try kde, some wrongly
# configured kde desktop cannot be detected (usually missing xprop)
# and if kde one can work, so why not use it if gtk, gtk3 not work?
# xdg is never a preferred solution
case "$DE" in
    kde)
        order="kde qt xdg"
        ;;
    *)
        order="qt kde xdg"
        ;;
esac

for cmd in $order; do
    run_${cmd}
done

echo 'Cannot find a command to run.' >&2
