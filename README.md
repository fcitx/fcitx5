Next generation of fcitx
==========================
Fcitx 5 is a generic input method framework released under LGPL-2.1+.

# Resources

[![Jenkins Build Status](https://img.shields.io/jenkins/s/https/jenkins.fcitx-im.org/job/fcitx5.svg)](https://jenkins.fcitx-im.org/job/fcitx5/)
[![Coverity Scan Status](https://img.shields.io/coverity/scan/9063.svg)](https://scan.coverity.com/projects/fcitx-fcitx5)
[![Documentation](https://codedocs.xyz/fcitx/fcitx5.svg)](https://codedocs.xyz/fcitx/fcitx5/)

* Wiki: [https://fcitx-im.org/](https://fcitx-im.org/)
  - Registration require explicit approval due to the spam, please send an email to the mail list if you do not get approved.
* Discussion:
  - Email: Send email to [fcitx@googlegroups.com](https://groups.google.com/g/fcitx)
  - Github Discussions: [https://github.com/fcitx/fcitx5/discussions](https://github.com/fcitx/fcitx5/discussions)
* Chat Group:
  - Following methods are all bridged together, you can pick any that works.
  - Telegram: https://fcitx-im.org/telegram/captcha.html
  - IRC: [#fcitx [at] libera.chat](https://web.libera.chat/?channels=#fcitx)
  - Matrix: fcitx on matrix.org
* Bug report: [https://github.com/fcitx/fcitx5/issues](https://github.com/fcitx/fcitx5/issues)
  - You can always report any fcitx 5 issue here, it might be transfer to other repos later.
* Translation: [https://explore.transifex.com/fcitx/](https://explore.transifex.com/fcitx/)
  - Do not send pull request for translation updates.
  - The translation will be automatically pushed to git repository nightly, but not vice versa.
 
# Supported platform on Linux and BSD
X11/Wayland

For using input method under TTY, please check [fbterm](https://github.com/fcitx/fcitx5-fbterm/) or [tmux](https://github.com/wengxt/fcitx5-tmux), not all features are supported.
 
# Quick start for Linux
[Install Fcitx 5](https://fcitx-im.org/wiki/Install_Fcitx_5)

Looking for Mac or Android?
[Mac](https://github.com/fcitx-contrib/fcitx5-macos/)
[Android](https://github.com/fcitx5-android/fcitx5-android)
 
The main package (this repository) only contains keyboard layout engine.

Coressponding input method engines need to be installed to support other languages (e.g. Chinese/Japanese/Korean).

You may find the list of input method engines at [here](https://fcitx-im.org/wiki/Input_method_engines).

# For developers
[Compiling fcitx5](https://fcitx-im.org/wiki/Compiling_fcitx5)

To write a input method (or addon) from scratch, check [Develop a simple input method](https://fcitx-im.org/wiki/Develop_an_simple_input_method)

# Packaging status

[![Packaging status](https://repology.org/badge/vertical-allrepos/fcitx5.svg)](https://repology.org/project/fcitx5/versions)
