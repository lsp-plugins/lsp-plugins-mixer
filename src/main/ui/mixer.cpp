/*
 * Copyright (C) 2024 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2024 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/plug-fw/ui.h>
#include <lsp-plug.in/stdlib/stdio.h>
#include <lsp-plug.in/stdlib/stdlib.h>

#include <private/plugins/mixer.h>
#include <private/ui/mixer.h>


namespace lsp
{
    namespace plugui
    {
        //---------------------------------------------------------------------
        // Plugin UI factory
        static const meta::plugin_t *plugin_uis[] =
        {
            &meta::mixer_x4_mono,
            &meta::mixer_x8_mono,
            &meta::mixer_x16_mono,
            &meta::mixer_x4_stereo,
            &meta::mixer_x8_stereo,
            &meta::mixer_x16_stereo
        };

        static ui::Module *ui_factory(const meta::plugin_t *meta)
        {
            return new mixer(meta);
        }

        static ui::Factory factory(ui_factory, plugin_uis, 6);


        //---------------------------------------------------------------------
        // Plugin UI
        mixer::mixer(const meta::plugin_t *meta)
            : ui::Module(meta)
        {
        }

        mixer::~mixer()
        {
        }

        status_t mixer::post_init()
        {
            status_t res = ui::Module::post_init();
            if (res != STATUS_OK)
                return res;

            // Generate channels
            for (size_t i=0; ; ++i)
            {
                ui::IPort *ch = find_port("cg", i+1);
                if (ch == NULL)
                    break;

                // Add one more channel
                channel_t *c = vChannels.add();
                if (c == NULL)
                    return STATUS_NO_MEM;

                c->nIndex       = i+1;
                c->bNameChanged = false;

                c->wName        = find_widget<tk::Edit>("channel_name", c->nIndex);

                if (c->wName != NULL)
                {
                    c->wName->text()->set("lists.mixer.channel");
                    c->wName->text()->params()->set_int("id", int(c->nIndex));

                    c->wName->slots()->bind(tk::SLOT_CHANGE, slot_channel_name_updated, c);
                }
            }

            return STATUS_OK;
        }

        ui::IPort *mixer::find_port(const char *prefix, size_t id)
        {
            LSPString uid;
            uid.fmt_ascii("%s_%d", prefix, int(id));
            return pWrapper->port(&uid);
        }

        template <class T>
        T *mixer::find_widget(const char *prefix, size_t id)
        {
            LSPString uid;
            uid.fmt_ascii("%s_%d", prefix, int(id));
            return pWrapper->controller()->widgets()->get<T>(&uid);
        }

        void mixer::notify(ui::IPort *port, size_t flags)
        {
        }

        void mixer::sync_channel_names(core::KVTStorage *kvt)
        {
            LSPString value;

            for (size_t i=0, n=vChannels.size(); i<n; ++i)
            {
                channel_t *c = vChannels.uget(i);
                if ((c->wName == NULL) || (!c->bNameChanged))
                    continue;

                // Obtain the new instrument name
                if (c->wName->text()->format(&value) != STATUS_OK)
                    continue;

                // Submit new value to KVT
                set_channel_name(kvt, c->nIndex, value.get_utf8());
            }
        }

        void mixer::set_channel_name(core::KVTStorage *kvt, int id, const char *name)
        {
            char kvt_name[0x80];
            core::kvt_param_t kparam;

            // Submit new value to KVT
            snprintf(kvt_name, sizeof(kvt_name), "/channel/%d/name", int(id));
            kparam.type     = core::KVT_STRING;
            kparam.str      = name;
            lsp_trace("%s = %s", kvt_name, kparam.str);
            kvt->put(kvt_name, &kparam, core::KVT_RX);
            wrapper()->kvt_notify_write(kvt, kvt_name, &kparam);
        }

        void mixer::idle()
        {
            // Scan the list of instrument names for changes
            size_t changes = 0;
            for (size_t i=0, n=vChannels.size(); i<n; ++i)
            {
                channel_t *c = vChannels.uget(i);
                if ((c->wName != NULL) && (c->bNameChanged))
                    ++changes;
            }

            // Apply instrument names to KVT
            if (changes > 0)
            {
                core::KVTStorage *kvt = wrapper()->kvt_lock();
                if (kvt != NULL)
                {
                    sync_channel_names(kvt);
                    wrapper()->kvt_release();
                }
            }
        }

        void mixer::kvt_changed(core::KVTStorage *kvt, const char *id, const core::kvt_param_t *value)
        {
            ui::Module::kvt_changed(kvt, id, value);

            if ((value->type == core::KVT_STRING) && (::strstr(id, "/channel/") == id))
            {
                id += ::strlen("/channel/");

                char *endptr = NULL;
                errno = 0;
                long index = ::strtol(id, &endptr, 10);

                // Valid object number?
                if ((errno == 0) && (!::strcmp(endptr, "/name")) && (index > 0))
                {
                    for (size_t i=0, n=vChannels.size(); i<n; ++i)
                    {
                        channel_t *c = vChannels.uget(i);
                        if ((c->wName == NULL) || (c->nIndex != size_t(index)))
                            continue;

                        c->wName->text()->set_raw(value->str);
                        c->bNameChanged = false;
                    }
                }
            }
        }

        status_t mixer::reset_settings()
        {
            core::KVTStorage *kvt = wrapper()->kvt_lock();
            if (kvt != NULL)
            {
                // Reset all names for all instruments
                for (size_t i=0, n=vChannels.size(); i<n; ++i)
                {
                    channel_t *c = vChannels.uget(i);
                    if (c->wName == NULL)
                        continue;

                    c->wName->text()->set("lists.mixer.channel");
                    c->wName->text()->params()->set_int("id", int(c->nIndex));
                    c->bNameChanged  = true;
                }

                sync_channel_names(kvt);
                wrapper()->kvt_release();
            }

            return ui::Module::reset_settings();
        }

        status_t mixer::slot_channel_name_updated(tk::Widget *sender, void *ptr, void *data)
        {
            channel_t *c    = static_cast<channel_t *>(ptr);
            c->bNameChanged = true;

            return STATUS_OK;
        }

    } /* namespace plugui */
} /* namespace lsp */

