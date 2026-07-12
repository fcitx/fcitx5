/*
 * SPDX-FileCopyrightText: 2026-2026 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#ifndef _FCITX_TEMPMODEMANAGER_H_
#define _FCITX_TEMPMODEMANAGER_H_

#include <memory>
#include <fcitx-utils/handlertable.h>
#include <fcitx-utils/macros.h>
#include <fcitx/fcitxcore_export.h>
#include <fcitx/instance.h>
#include <fcitx/tempmode.h>

/// \addtogroup FcitxCore
/// \{
/// \file
/// \brief Registration and dispatch for temporary input modes.

namespace fcitx {

class TempModeManagerPrivate;

/**
 * Manage registered TempMode objects for an Instance.
 *
 * The manager watches key events, activates temp modes on their trigger keys,
 * and routes subsequent key events to the active mode before normal input
 * method handling.
 */
class FCITXCORE_EXPORT TempModeManager {
public:
    /**
     * Create a temp mode manager for an instance.
     *
     * @param instance instance that owns the event pipeline
     */
    TempModeManager(Instance *instance);
    ~TempModeManager();

    /**
     * Register a temp mode with this manager.
     *
     * Registering the same temp mode multiple times has no effect.
     *
     * @param tempMode temp mode to register
     */
    void registerTempMode(TempMode &tempMode);

private:
    std::unique_ptr<TempModeManagerPrivate> d_ptr;
    FCITX_DECLARE_PRIVATE(TempModeManager);
};

} // namespace fcitx

/// \}

#endif // _FCITX_TEMPMODEMANAGER_H_
