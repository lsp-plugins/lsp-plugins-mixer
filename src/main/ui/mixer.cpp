/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugins-mixer
 * Created on: 25 нояб. 2020 г.
 *
 * lsp-plugins-mixer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugins-mixer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugins-mixer. If not, see <https://www.gnu.org/licenses/>.
 */

#include <lsp-plug.in/plug-fw/ui.h>
#include <private/plugins/mixer.h>

namespace lsp
{
    namespace plugui
    {
        //---------------------------------------------------------------------
        // Plugin UI factory
        static const meta::plugin_t *plugin_uis[] =
        {
            &meta::mixer_x8_mono,
            &meta::mixer_x16_mono,
            &meta::mixer_x8_stereo,
            &meta::mixer_x16_stereo
        };

        static ui::Factory factory(plugin_uis, 4);

    } /* namespace plugui */
} /* namespace lsp */

