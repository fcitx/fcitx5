#!/bin/sh
#--------------------------------------
# fcitx-config
#

export TEXTDOMAIN=fcitx5

if which kdialog > /dev/null 2>&1; then
    message() {
        kdialog --msgbox "$1"
    }
    error() {
        kdialog --error "$1"
    }
elif which zenity > /dev/null 2>&1; then
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

if which gettext > /dev/null 2>&1; then
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

    if [ -n "${XDG_CURRENT_DESKTOP}" ]; then
      case "${XDG_CURRENT_DESKTOP}" in
         GNOME)
           DE=gnome;
           ;;
         KDE)
           DE=kde;
           ;;
         LXDE)
           DE=lxde;
           ;;
         XFCE)
           DE=xfce
      esac
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
      which gnome-default-applications-properties > /dev/null 2>&1  || DE="gnome3"
    fi
}

run_kde() {
    if (kcmshell5 --list 2>/dev/null | grep ^kcm_fcitx5 > /dev/null 2>&1); then
        if [ x"$1" != x ]; then
            exec kcmshell5 fcitx5 --args "$1"
        else
            exec kcmshell5 fcitx5
        fi
    fi
}

run_qt() {
    if which fcitx5-config-qt > /dev/null 2>&1; then
        exec fcitx5-config-qt "$1"
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

    if command="$(which xdg-open 2>/dev/null)"; then
        exec "$command" "$HOME/.config/fcitx5"
    fi
}

_which_cmdline() {
    cmd="$(which "$1")" || return 1
    shift
    echo "$cmd $*"
}

if [ ! -n "$DISPLAY" ] && [ ! -n "$WAYLAND_DISPLAY" ]; then
    echo 'Please run it under desktop.' >&2
    exit 0
fi

detectDE

# even if we are not on KDE, we should still try kde, some wrongly
# configured kde desktop cannot be detected (usually missing xprop)
# and if kde one can work, so why not use it if gtk, gtk3 not work?
# xdg/editor is never a preferred solution
case "$DE" in
    kde)
        order="kde qt xdg"
        ;;
    *)
        order="qt kde xdg"
        ;;
esac

for cmd in $order; do
    run_${cmd} "$1"
done

echo 'Cannot find a command to run.' >&2
