/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "applicationactioncontroller.h"

#include "actions/actiontypes.h"
#include "log.h"

using namespace mu::appshell;
using namespace muse;
using namespace muse::actions;

void ApplicationActionController::preInit()
{
}

void ApplicationActionController::init()
{
    // Forward the global action codes to their notation-scoped equivalents.
    // On desktop the real ApplicationActionController does this; on web our
    // stub used to leave them unregistered, so Esc / arrows / Backspace / etc.
    // were dispatched into the void.
    auto forward = [this](const ActionCode& from, const ActionCode& to) {
        dispatcher()->reg(this, from, [this, to]() {
            dispatcher()->dispatch(to);
        });
    };

    forward("action://copy",   "action://notation/copy");
    forward("action://cut",    "action://notation/cut");
    forward("action://paste",  "action://notation/paste");
    forward("action://undo",   "action://notation/undo");
    forward("action://redo",   "action://notation/redo");
    forward("action://delete", "action://notation/delete");
    forward("action://cancel", "action://notation/cancel");
}
