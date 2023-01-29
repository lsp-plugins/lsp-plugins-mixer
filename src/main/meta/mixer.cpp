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

#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/shared/meta/developers.h>
#include <private/meta/mixer.h>

#define LSP_PLUGINS_MIXER_VERSION_MAJOR       1
#define LSP_PLUGINS_MIXER_VERSION_MINOR       0
#define LSP_PLUGINS_MIXER_VERSION_MICRO       0

#define LSP_PLUGINS_MIXER_VERSION  \
    LSP_MODULE_VERSION( \
        LSP_PLUGINS_MIXER_VERSION_MAJOR, \
        LSP_PLUGINS_MIXER_VERSION_MINOR, \
        LSP_PLUGINS_MIXER_VERSION_MICRO  \
    )

namespace lsp
{
    namespace meta
    {
        //-------------------------------------------------------------------------
        // Plugin metadata
        #define MIX_MONO_CHANNEL(id, label) \
            AUDIO_INPUT("in" id, "Audio input" label), \
            SWITCH("cs" id, "Channel solo" label, 0.0f), \
            SWITCH("cm" id, "Channel mute" label, 0.0f), \
            SWITCH("ci" id, "Channel phase invert" label, 0.0f), \
            LOG_CONTROL("cg" id, "Channel gain" label, U_GAIN_AMP, meta::mixer::CHANNEL_GAIN), \
            METER_GAIN("cl" id, "Channel signal level", GAIN_AMP_P_48_DB)

        #define MIX_STEREO_CHANNEL(id, label) \
            AUDIO_INPUT("in" id "l", "Audio input left" label), \
            AUDIO_INPUT("in" id "r", "Audio input right" label), \
            SWITCH("cs" id, "Channel solo" label, 0.0f), \
            SWITCH("cm" id, "Channel mute" label, 0.0f), \
            SWITCH("ci" id, "Channel phase invert" label, 0.0f), \
            PAN_CTL("cp" id "l", "Channel pan left" label, -100.0f), \
            PAN_CTL("cp" id "r", "Channel pan right" label, 100.0f), \
            PAN_CTL("cb" id, "Channel output balance" label, 0.0f), \
            LOG_CONTROL("cg" id, "Channel gain" label, U_GAIN_AMP, meta::mixer::CHANNEL_GAIN), \
            METER_GAIN("cl" id "l", "Channel signal level left", GAIN_AMP_P_48_DB), \
            METER_GAIN("cl" id "r", "Channel signal level right", GAIN_AMP_P_48_DB)

        #define MIX_MONO_GLOBAL \
            DRY_GAIN(0.0f), \
            WET_GAIN(1.0f), \
            OUT_GAIN, \
            METER_GAIN("ilm", "Input level meter", GAIN_AMP_P_48_DB), \
            METER_GAIN("olm", "Output level meter", GAIN_AMP_P_48_DB)

        #define MIX_STEREO_GLOBAL \
            SWITCH("mono", "Mono output", 0.0f), \
            DRY_GAIN(0.0f), \
            WET_GAIN(1.0f), \
            OUT_GAIN, \
            METER_GAIN("ilm_l", "Input level meter left", GAIN_AMP_P_48_DB), \
            METER_GAIN("ilm_r", "Input level meter right", GAIN_AMP_P_48_DB), \
            METER_GAIN("olm_l", "Output level meter left", GAIN_AMP_P_48_DB), \
            METER_GAIN("olm_r", "Output level meter right", GAIN_AMP_P_48_DB)

        static const port_t mixer_x4_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            BYPASS,
            MIX_MONO_GLOBAL,

            MIX_MONO_CHANNEL("_1", "1"),
            MIX_MONO_CHANNEL("_2", "2"),
            MIX_MONO_CHANNEL("_3", "3"),
            MIX_MONO_CHANNEL("_4", "4"),

            PORTS_END
        };

        static const port_t mixer_x8_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            BYPASS,
            MIX_MONO_GLOBAL,

            MIX_MONO_CHANNEL("_1", "1"),
            MIX_MONO_CHANNEL("_2", "2"),
            MIX_MONO_CHANNEL("_3", "3"),
            MIX_MONO_CHANNEL("_4", "4"),
            MIX_MONO_CHANNEL("_5", "5"),
            MIX_MONO_CHANNEL("_6", "6"),
            MIX_MONO_CHANNEL("_7", "7"),
            MIX_MONO_CHANNEL("_8", "8"),

            PORTS_END
        };

        static const port_t mixer_x16_mono_ports[] =
        {
            PORTS_MONO_PLUGIN,
            BYPASS,
            MIX_MONO_GLOBAL,

            MIX_MONO_CHANNEL("_1", "1"),
            MIX_MONO_CHANNEL("_2", "2"),
            MIX_MONO_CHANNEL("_3", "3"),
            MIX_MONO_CHANNEL("_4", "4"),
            MIX_MONO_CHANNEL("_5", "5"),
            MIX_MONO_CHANNEL("_6", "6"),
            MIX_MONO_CHANNEL("_7", "7"),
            MIX_MONO_CHANNEL("_8", "8"),
            MIX_MONO_CHANNEL("_9", "9"),
            MIX_MONO_CHANNEL("_10", "10"),
            MIX_MONO_CHANNEL("_11", "11"),
            MIX_MONO_CHANNEL("_12", "12"),
            MIX_MONO_CHANNEL("_13", "13"),
            MIX_MONO_CHANNEL("_14", "14"),
            MIX_MONO_CHANNEL("_15", "15"),
            MIX_MONO_CHANNEL("_16", "16"),

            PORTS_END
        };

        static const port_t mixer_x4_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            BYPASS,
            MIX_STEREO_GLOBAL,

            MIX_STEREO_CHANNEL("_1", "1"),
            MIX_STEREO_CHANNEL("_2", "2"),
            MIX_STEREO_CHANNEL("_3", "3"),
            MIX_STEREO_CHANNEL("_4", "4"),

            PORTS_END
        };

        static const port_t mixer_x8_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            BYPASS,
            MIX_STEREO_GLOBAL,

            MIX_STEREO_CHANNEL("_1", "1"),
            MIX_STEREO_CHANNEL("_2", "2"),
            MIX_STEREO_CHANNEL("_3", "3"),
            MIX_STEREO_CHANNEL("_4", "4"),
            MIX_STEREO_CHANNEL("_5", "5"),
            MIX_STEREO_CHANNEL("_6", "6"),
            MIX_STEREO_CHANNEL("_7", "7"),
            MIX_STEREO_CHANNEL("_8", "8"),

            PORTS_END
        };

        static const port_t mixer_x16_stereo_ports[] =
        {
            PORTS_STEREO_PLUGIN,
            BYPASS,
            MIX_STEREO_GLOBAL,

            MIX_STEREO_CHANNEL("_1", "1"),
            MIX_STEREO_CHANNEL("_2", "2"),
            MIX_STEREO_CHANNEL("_3", "3"),
            MIX_STEREO_CHANNEL("_4", "4"),
            MIX_STEREO_CHANNEL("_5", "5"),
            MIX_STEREO_CHANNEL("_6", "6"),
            MIX_STEREO_CHANNEL("_7", "7"),
            MIX_STEREO_CHANNEL("_8", "8"),
            MIX_STEREO_CHANNEL("_9", "9"),
            MIX_STEREO_CHANNEL("_10", "10"),
            MIX_STEREO_CHANNEL("_11", "11"),
            MIX_STEREO_CHANNEL("_12", "12"),
            MIX_STEREO_CHANNEL("_13", "13"),
            MIX_STEREO_CHANNEL("_14", "14"),
            MIX_STEREO_CHANNEL("_15", "15"),
            MIX_STEREO_CHANNEL("_16", "16"),

            PORTS_END
        };

        static const int plugin_classes[]       = { C_MIXER, -1 };
        static const int clap_features_mono[]   = { CF_AUDIO_EFFECT, CF_UTILITY, CF_MIXING, -1 };
        static const int clap_features_stereo[] = { CF_AUDIO_EFFECT, CF_UTILITY, CF_MIXING, -1 };

        const meta::bundle_t mixer_bundle =
        {
            "mixer",
            "Mixer",
            B_UTILITIES,
            "", // TODO: provide ID of the video on YouTube
            "" // TODO: write plugin description, should be the same to the english version in 'bundles.json'
        };

        const plugin_t mixer_x4_mono =
        {
            "Mischer x4 Mono",
            "Mixer x4 Mono",
            "M4M",
            &developers::v_sadovnikov,
            "mixer_x4_mono",
            LSP_LV2_URI("mixer_x4_mono"),
            LSP_LV2UI_URI("mixer_x4_mono"),
            "m04m",
            LSP_LADSPA_MIXER_BASE + 0,
            LSP_LADSPA_URI("mixer_x4_mono"),
            LSP_CLAP_URI("mixer_x4_mono"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_DUMP_STATE,
            mixer_x4_mono_ports,
            "util/mixer/mono.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

        const plugin_t mixer_x8_mono =
        {
            "Mischer x8 Mono",
            "Mixer x8 Mono",
            "M8M",
            &developers::v_sadovnikov,
            "mixer_x8_mono",
            LSP_LV2_URI("mixer_x8_mono"),
            LSP_LV2UI_URI("mixer_x8_mono"),
            "m08m",
            LSP_LADSPA_MIXER_BASE + 0,
            LSP_LADSPA_URI("mixer_x8_mono"),
            LSP_CLAP_URI("mixer_x8_mono"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_DUMP_STATE,
            mixer_x8_mono_ports,
            "util/mixer/mono.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

        const plugin_t mixer_x16_mono =
        {
            "Mischer x16 Mono",
            "Mixer x16 Mono",
            "M16M",
            &developers::v_sadovnikov,
            "mixer_x16_mono",
            LSP_LV2_URI("mixer_x16_mono"),
            LSP_LV2UI_URI("mixer_x16_mono"),
            "m16m",
            LSP_LADSPA_MIXER_BASE + 1,
            LSP_LADSPA_URI("mixer_x16_mono"),
            LSP_CLAP_URI("mixer_x16_mono"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_mono,
            E_DUMP_STATE,
            mixer_x16_mono_ports,
            "util/mixer/mono.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

        const plugin_t mixer_x4_stereo =
        {
            "Mischer x4 Stereo",
            "Mixer x4 Stereo",
            "M4S",
            &developers::v_sadovnikov,
            "mixer_x4_stereo",
            LSP_LV2_URI("mixer_x4_stereo"),
            LSP_LV2UI_URI("mixer_x4_stereo"),
            "m04s",
            LSP_LADSPA_MIXER_BASE + 2,
            LSP_LADSPA_URI("mixer_x4_stereo"),
            LSP_CLAP_URI("mixer_x4_stereo"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_DUMP_STATE,
            mixer_x4_stereo_ports,
            "util/mixer/stereo.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

        const plugin_t mixer_x8_stereo =
        {
            "Mischer x8 Stereo",
            "Mixer x8 Stereo",
            "M8S",
            &developers::v_sadovnikov,
            "mixer_x8_stereo",
            LSP_LV2_URI("mixer_x8_stereo"),
            LSP_LV2UI_URI("mixer_x8_stereo"),
            "m08s",
            LSP_LADSPA_MIXER_BASE + 2,
            LSP_LADSPA_URI("mixer_x8_stereo"),
            LSP_CLAP_URI("mixer_x8_stereo"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_DUMP_STATE,
            mixer_x8_stereo_ports,
            "util/mixer/stereo.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

        const plugin_t mixer_x16_stereo =
        {
            "Mischer x16 Stereo",
            "Mixer x16 Stereo",
            "M16s",
            &developers::v_sadovnikov,
            "mixer_x16_stereo",
            LSP_LV2_URI("mixer_x16_stereo"),
            LSP_LV2UI_URI("mixer_x16_stereo"),
            "m16s",
            LSP_LADSPA_MIXER_BASE + 3,
            LSP_LADSPA_URI("mixer_x16_stereo"),
            LSP_CLAP_URI("mixer_x16_stereo"),
            LSP_PLUGINS_MIXER_VERSION,
            plugin_classes,
            clap_features_stereo,
            E_DUMP_STATE,
            mixer_x16_stereo_ports,
            "util/mixer/stereo.xml",
            NULL,
            mono_plugin_port_groups,
            &mixer_bundle
        };

    } /* namespace meta */
} /* namespace lsp */



