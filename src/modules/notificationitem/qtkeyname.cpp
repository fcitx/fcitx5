/*
 * SPDX-FileCopyrightText: 2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#include "qtkeyname.h"
#include <iterator>
#include <string_view>
#include <unordered_map>
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/misc.h"

namespace fcitx {
namespace {

constexpr struct {
    KeySym key;
    const char name[25];
} keyname[] = {
    {FcitxKey_space, "Space"},
    {FcitxKey_Escape, "Esc"},
    {FcitxKey_Tab, "Tab"},
    {FcitxKey_BackTab, "Backtab"},
    {FcitxKey_BackSpace, "Backspace"},
    {FcitxKey_Return, "Return"},
    {FcitxKey_Insert, "Ins"},
    {FcitxKey_Delete, "Del"},
    {FcitxKey_Pause, "Pause"},
    {FcitxKey_Print, "Print"},
    {FcitxKey_Sys_Req, "SysReq"},
    {FcitxKey_Home, "Home"},
    {FcitxKey_End, "End"},
    {FcitxKey_Left, "Left"},
    {FcitxKey_Up, "Up"},
    {FcitxKey_Right, "Right"},
    {FcitxKey_Down, "Down"},
    {FcitxKey_Page_Up, "PgUp"},
    {FcitxKey_Page_Down, "PgDown"},
    {FcitxKey_Caps_Lock, "CapsLock"},
    {FcitxKey_Num_Lock, "NumLock"},
    {FcitxKey_Scroll_Lock, "ScrollLock"},
    {FcitxKey_Menu, "Menu"},
    {FcitxKey_Help, "Help"},
    // Special keys
    // Includes multimedia, launcher, lan keys ( bluetooth, wireless
    // window navigation
    {FcitxKey_Back, "Back"},
    {FcitxKey_Forward, "Forward"},
    {FcitxKey_Stop, "Stop"},
    {FcitxKey_Refresh, "Refresh"},
    {FcitxKey_AudioLowerVolume, "Volume Down"},
    {FcitxKey_AudioMute, "Volume Mute"},
    {FcitxKey_AudioRaiseVolume, "Volume Up"},
    {FcitxKey_AudioBassBoost, "Bass Boost"},
    // {FcitxKey_AudioBassUp, "Bass Up"},
    // {FcitxKey_AudioBassDown, "Bass Down"},
    // {FcitxKey_TrebleUp, "Treble Up"},
    // {FcitxKey_TrebleDown, "Treble Down"},
    {FcitxKey_AudioPlay, "Media Play"},
    {FcitxKey_AudioStop, "Media Stop"},
    {FcitxKey_AudioPrev, "Media Previous"},
    {FcitxKey_AudioNext, "Media Next"},
    {FcitxKey_AudioRecord, "Media Record"},
    //: Media player pause button
    {FcitxKey_AudioPause, "Media Pause"},
    //: Media player button to toggle between playing and paused
    // {FcitxKey_AudioTogglePlayPause, "Toggle Media Play/Pause"},
    {FcitxKey_HomePage, "Home Page"},
    {FcitxKey_Favorites, "Favorites"},
    {FcitxKey_Search, "Search"},
    {FcitxKey_Standby, "Standby"},
    {FcitxKey_OpenURL, "Open URL"},
    {FcitxKey_Mail, "Launch Mail"},
    {FcitxKey_MediaPlayer, "Launch Media"},
    {FcitxKey_Launch0, "Launch (0)"},
    {FcitxKey_Launch1, "Launch (1)"},
    {FcitxKey_Launch2, "Launch (2)"},
    {FcitxKey_Launch3, "Launch (3)"},
    {FcitxKey_Launch4, "Launch (4)"},
    {FcitxKey_Launch5, "Launch (5)"},
    {FcitxKey_Launch6, "Launch (6)"},
    {FcitxKey_Launch7, "Launch (7)"},
    {FcitxKey_Launch8, "Launch (8)"},
    {FcitxKey_Launch9, "Launch (9)"},
    {FcitxKey_LaunchA, "Launch (A)"},
    {FcitxKey_LaunchB, "Launch (B)"},
    {FcitxKey_LaunchC, "Launch (C)"},
    {FcitxKey_LaunchD, "Launch (D)"},
    {FcitxKey_LaunchE, "Launch (E)"},
    {FcitxKey_LaunchF, "Launch (F)"},
    // {FcitxKey_LaunchG, "Launch (G)"},
    // {FcitxKey_LaunchH, "Launch (H)"},
    {FcitxKey_MonBrightnessUp, "Monitor Brightness Up"},
    {FcitxKey_MonBrightnessDown, "Monitor Brightness Down"},
    {FcitxKey_KbdLightOnOff, "Keyboard Light On/Off"},
    {FcitxKey_KbdBrightnessUp, "Keyboard Brightness Up"},
    {FcitxKey_KbdBrightnessDown, "Keyboard Brightness Down"},
    {FcitxKey_PowerOff, "Power Off"},
    {FcitxKey_WakeUp, "Wake Up"},
    {FcitxKey_Eject, "Eject"},
    {FcitxKey_ScreenSaver, "Screensaver"},
    {FcitxKey_WWW, "WWW"},
    {FcitxKey_Sleep, "Sleep"},
    {FcitxKey_LightBulb, "LightBulb"},
    {FcitxKey_Shop, "Shop"},
    {FcitxKey_History, "History"},
    {FcitxKey_AddFavorite, "Add Favorite"},
    {FcitxKey_HotLinks, "Hot Links"},
    {FcitxKey_BrightnessAdjust, "Adjust Brightness"},
    {FcitxKey_Finance, "Finance"},
    {FcitxKey_Community, "Community"},
    {FcitxKey_AudioRewind, "Media Rewind"},
    {FcitxKey_BackForward, "Back Forward"},
    {FcitxKey_ApplicationLeft, "Application Left"},
    {FcitxKey_ApplicationRight, "Application Right"},
    {FcitxKey_Book, "Book"},
    {FcitxKey_CD, "CD"},
    {FcitxKey_Calculator, "Calculator"},
    {FcitxKey_Calendar, "Calendar"},
    {FcitxKey_Clear, "Clear"},
    {FcitxKey_ClearGrab, "Clear Grab"},
    {FcitxKey_Close, "Close"},
    {FcitxKey_ContrastAdjust, "Adjust contrast"},
    {FcitxKey_Copy, "Copy"},
    {FcitxKey_Cut, "Cut"},
    {FcitxKey_Display, "Display"},
    {FcitxKey_DOS, "DOS"},
    {FcitxKey_Documents, "Documents"},
    {FcitxKey_Excel, "Spreadsheet"},
    {FcitxKey_Explorer, "Browser"},
    {FcitxKey_Game, "Game"},
    {FcitxKey_Go, "Go"},
    {FcitxKey_iTouch, "iTouch"},
    {FcitxKey_LogOff, "Logoff"},
    {FcitxKey_Market, "Market"},
    {FcitxKey_Meeting, "Meeting"},
    {FcitxKey_Memo, "Memo"},
    {FcitxKey_MenuKB, "Keyboard Menu"},
    {FcitxKey_MenuPB, "Menu PB"},
    {FcitxKey_MySites, "My Sites"},
    {FcitxKey_News, "News"},
    {FcitxKey_OfficeHome, "Home Office"},
    {FcitxKey_Option, "Option"},
    {FcitxKey_Paste, "Paste"},
    {FcitxKey_Phone, "Phone"},
    {FcitxKey_Reply, "Reply"},
    {FcitxKey_Reload, "Reload"},
    {FcitxKey_RotateWindows, "Rotate Windows"},
    {FcitxKey_RotationPB, "Rotation PB"},
    {FcitxKey_RotationKB, "Rotation KB"},
    {FcitxKey_Save, "Save"},
    {FcitxKey_Send, "Send"},
    {FcitxKey_Spell, "Spellchecker"},
    {FcitxKey_SplitScreen, "Split Screen"},
    {FcitxKey_Support, "Support"},
    {FcitxKey_TaskPane, "Task Panel"},
    {FcitxKey_Terminal, "Terminal"},
    {FcitxKey_ToDoList, "To-do list"},
    {FcitxKey_Tools, "Tools"},
    {FcitxKey_Travel, "Travel"},
    {FcitxKey_Video, "Video"},
    {FcitxKey_Word, "Word Processor"},
    {FcitxKey_Xfer, "XFer"},
    {FcitxKey_ZoomIn, "Zoom In"},
    {FcitxKey_ZoomOut, "Zoom Out"},
    {FcitxKey_Away, "Away"},
    {FcitxKey_Messenger, "Messenger"},
    {FcitxKey_WebCam, "WebCam"},
    {FcitxKey_MailForward, "Mail Forward"},
    {FcitxKey_Pictures, "Pictures"},
    {FcitxKey_Music, "Music"},
    {FcitxKey_Battery, "Battery"},
    {FcitxKey_Bluetooth, "Bluetooth"},
    {FcitxKey_WLAN, "Wireless"},
    {FcitxKey_AudioForward, "Media Fast Forward"},
    {FcitxKey_AudioRepeat, "Audio Repeat"},
    {FcitxKey_AudioRandomPlay, "Audio Random Play"},
    {FcitxKey_Subtitle, "Subtitle"},
    {FcitxKey_AudioCycleTrack, "Audio Cycle Track"},
    {FcitxKey_Time, "Time"},
    {FcitxKey_Hibernate, "Hibernate"},
    {FcitxKey_View, "View"},
    {FcitxKey_TopMenu, "Top Menu"},
    {FcitxKey_PowerDown, "Power Down"},
    {FcitxKey_Suspend, "Suspend"},
    {FcitxKey_AudioMicMute, "Microphone Mute"},
    {FcitxKey_Red, "Red"},
    {FcitxKey_Green, "Green"},
    {FcitxKey_Yellow, "Yellow"},
    {FcitxKey_Blue, "Blue"},
    {FcitxKey_ChannelUp, "Channel Up"},
    {FcitxKey_ChannelDown, "Channel Down"},
    // {FcitxKey_Guide, "Guide"},
    {FcitxKey_Info, "Info"},
    // {FcitxKey_Settings, "Settings"},
    // {FcitxKey_AudioMicVolumeUp, "Microphone Volume Up"},
    // {FcitxKey_AudioMicVolumeDown, "Microphone Volume Down"},
    {FcitxKey_Keyboard, "Keyboard"},
    {FcitxKey_New, "New"},
    {FcitxKey_Open, "Open"},
    {FcitxKey_Find, "Find"},
    {FcitxKey_Undo, "Undo"},
    {FcitxKey_Redo, "Redo"},
    // --------------------------------------------------------------
    // More consistent namings
    {FcitxKey_Print, "Print Screen"},
    {FcitxKey_Page_Up, "Page Up"},
    {FcitxKey_Page_Down, "Page Down"},
    {FcitxKey_Caps_Lock, "Caps Lock"},
    {FcitxKey_Num_Lock, "Num Lock"},
    {FcitxKey_Num_Lock, "Number Lock"},
    {FcitxKey_Scroll_Lock, "Scroll Lock"},
    {FcitxKey_Insert, "Insert"},
    {FcitxKey_Delete, "Delete"},
    {FcitxKey_Escape, "Escape"},
    {FcitxKey_Sys_Req, "System Request"},
    // --------------------------------------------------------------
    // Keypad navigation keys
    {FcitxKey_Select, "Select"},
    // {FcitxKey_Yes, "Yes"},
    // {FcitxKey_No, "No"},
    // --------------------------------------------------------------
    // Device keys
    // {FcitxKey_Context1, "Context1"},
    // {FcitxKey_Context2, "Context2"},
    // {FcitxKey_Context3, "Context3"},
    // {FcitxKey_Context4, "Context4"},
    //: Button to start a call (note: a separate button is used to end the call
    {FcitxKey_PickupPhone, "Call"},
    //: Button to end a call (note: a separate button is used to start the call
    {FcitxKey_HangupPhone, "Hangup"},
    //: Button that will hang up if we're in call, or make a call if we're not.
    // {FcitxKey_ToggleCallHangup, "Toggle Call/Hangup"},
    // {FcitxKey_Flip, "Flip"},
    //: Button to trigger voice dialing
    // {FcitxKey_VoiceDial, "Voice Dial"},
    //: Button to redial the last number called
    // {FcitxKey_LastNumberRedial, "Last Number Redial"},
    //: Button to trigger the camera shutter (take a picture)
    // {FcitxKey_Camera, "Camera Shutter"},
    //: Button to focus the camera
    {FcitxKey_CameraFocus, "Camera Focus"},
    // --------------------------------------------------------------
    // Japanese keyboard support
    {FcitxKey_Kanji, "Kanji"},
    {FcitxKey_Muhenkan, "Muhenkan"},
    {FcitxKey_Henkan, "Henkan"},
    {FcitxKey_Romaji, "Romaji"},
    {FcitxKey_Hiragana, "Hiragana"},
    {FcitxKey_Katakana, "Katakana"},
    {FcitxKey_Hiragana_Katakana, "Hiragana Katakana"},
    {FcitxKey_Zenkaku, "Zenkaku"},
    {FcitxKey_Hankaku, "Hankaku"},
    {FcitxKey_Zenkaku_Hankaku, "Zenkaku Hankaku"},
    {FcitxKey_Touroku, "Touroku"},
    {FcitxKey_Massyo, "Massyo"},
    {FcitxKey_Kana_Lock, "Kana Lock"},
    {FcitxKey_Kana_Shift, "Kana Shift"},
    {FcitxKey_Eisu_Shift, "Eisu Shift"},
    {FcitxKey_Eisu_toggle, "Eisu toggle"},
    {FcitxKey_Codeinput, "Code input"},
    {FcitxKey_MultipleCandidate, "Multiple Candidate"},
    {FcitxKey_PreviousCandidate, "Previous Candidate"},
    // --------------------------------------------------------------
    // Korean keyboard support
    {FcitxKey_Hangul, "Hangul"},
    {FcitxKey_Hangul_Start, "Hangul Start"},
    {FcitxKey_Hangul_End, "Hangul End"},
    {FcitxKey_Hangul_Hanja, "Hangul Hanja"},
    {FcitxKey_Hangul_Jamo, "Hangul Jamo"},
    {FcitxKey_Hangul_Romaja, "Hangul Romaja"},
    {FcitxKey_Hangul_Jeonja, "Hangul Jeonja"},
    {FcitxKey_Hangul_Banja, "Hangul Banja"},
    {FcitxKey_Hangul_PreHanja, "Hangul PreHanja"},
    {FcitxKey_Hangul_PostHanja, "Hangul PostHanja"},
    {FcitxKey_Hangul_Special, "Hangul Special"},
    // --------------------------------------------------------------
    // Miscellaneous keys
    {FcitxKey_Cancel, "Cancel"},
    // {FcitxKey_Printer, "Printer"},
    {FcitxKey_Execute, "Execute"},
    // {FcitxKey_Play, "Play"},
    // {FcitxKey_Zoom, "Zoom"},
    // {FcitxKey_Exit, "Exit"},
    {FcitxKey_TouchpadToggle, "Touchpad Toggle"},
    {FcitxKey_TouchpadOn, "Touchpad On"},
    {FcitxKey_TouchpadOff, "Touchpad Off"},
};

const std::unordered_map<KeySym, std::string_view, EnumHash> &qtKeyNameMap() {
    static const auto map = [] {
        std::unordered_map<KeySym, std::string_view, EnumHash> result;
        result.reserve(std::size(keyname));
        for (const auto &entry : keyname) {
            result.try_emplace(entry.key, entry.name);
        }
        return result;
    }();
    return map;
}

} // namespace

std::string_view qtKeyName(KeySym key) {
    const auto &map = qtKeyNameMap();
    if (const auto iter = map.find(key); iter != map.end()) {
        return iter->second;
    }
    return {};
}

} // namespace fcitx
