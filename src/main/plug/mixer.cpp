/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/debug.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/dsp-units/units.h>
#include <lsp-plug.in/plug-fw/meta/func.h>
#include <lsp-plug.in/plug-fw/meta/ports.h>
#include <lsp-plug.in/stdlib/string.h>

#include <private/plugins/mixer.h>

/* The size of temporary buffer for audio processing */
#define BUFFER_SIZE         0x1000U

namespace lsp
{
    static plug::IPort *TRACE_PORT(plug::IPort *p)
    {
        lsp_trace("  port id=%s", (p)->metadata()->id);
        return p;
    }

    namespace plugins
    {
        //---------------------------------------------------------------------
        // Plugin factory
        static const meta::plugin_t *plugins[] =
        {
            &meta::mixer_x4_mono,
            &meta::mixer_x8_mono,
            &meta::mixer_x16_mono,
            &meta::mixer_x4_stereo,
            &meta::mixer_x8_stereo,
            &meta::mixer_x16_stereo
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            return new mixer(meta);
        }

        static plug::Factory factory(plugin_factory, plugins, 6);

        //---------------------------------------------------------------------
        // Implementation
        mixer::mixer(const meta::plugin_t *meta):
            Module(meta)
        {
            // Compute the number of audio channels by the number of inputs
            size_t p_channels   = 0;
            size_t m_channels   = 0;

            for (const meta::port_t *p = meta->ports; p->id != NULL; ++p)
            {
                if (!meta::is_audio_in_port(p))
                    continue;

                if (strcmp(p->id, meta::PORT_NAME_INPUT_L) == 0)
                    ++p_channels;
                else if (strcmp(p->id, meta::PORT_NAME_INPUT_R) == 0)
                    ++p_channels;
                else if (strcmp(p->id, meta::PORT_NAME_INPUT) == 0)
                    ++p_channels;
                else
                    ++m_channels;
            }

            vPChannels      = NULL;
            vMChannels      = NULL;
            nPChannels      = p_channels;
            nMChannels      = m_channels;
            bMonoOut        = false;
            vWet[0]         = NULL;
            vWet[1]         = NULL;
            vTemp[0]        = NULL;
            vTemp[1]        = NULL;

            pBypass         = NULL;
            pMonoOut        = NULL;
            pBalance        = NULL;

            pData           = NULL;
        }

        mixer::~mixer()
        {
            do_destroy();
        }

        void mixer::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            // Call parent class for initialization
            Module::init(wrapper, ports);

            // Estimate the number of bytes to allocate
            size_t szof_pchannels   = align_size(sizeof(primary_channel_t) * nPChannels, DEFAULT_ALIGN);
            size_t szof_mchannels   = align_size(sizeof(mix_channel_t) * nMChannels, DEFAULT_ALIGN);
            size_t szof_wet         = align_size(BUFFER_SIZE * sizeof(float), DEFAULT_ALIGN);
            size_t szof_temp        = align_size(BUFFER_SIZE * sizeof(float), DEFAULT_ALIGN);
            size_t alloc            = szof_pchannels + szof_mchannels + (szof_temp + szof_wet) * nPChannels;

            // Allocate memory-aligned data
            uint8_t *ptr            = alloc_aligned<uint8_t>(pData, alloc, DEFAULT_ALIGN);
            if (ptr == NULL)
                return;

            // Initialize pointers
            vPChannels              = reinterpret_cast<primary_channel_t *>(ptr);
            ptr                    += szof_pchannels;
            vMChannels              = reinterpret_cast<mix_channel_t *>(ptr);
            ptr                    += szof_mchannels;

            for (size_t i=0; i<nPChannels; ++i)
            {
                vWet[i]                 = reinterpret_cast<float *>(ptr);
                ptr                    += szof_wet;
                vTemp[i]                = reinterpret_cast<float *>(ptr);
                ptr                    += szof_temp;
            }

            // Initialize channels
            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *c = &vPChannels[i];

                c->sBypass.construct();

                c->vIn          = NULL;
                c->vOut         = NULL;
                c->fOldDry      = GAIN_AMP_0_DB;
                c->fDry         = GAIN_AMP_0_DB;
                c->fOldWet      = GAIN_AMP_0_DB;
                c->fWet         = GAIN_AMP_0_DB;
                c->fOldGain[0]  = GAIN_AMP_0_DB;
                c->fOldGain[1]  = GAIN_AMP_0_DB;
                c->fGain[0]     = GAIN_AMP_0_DB;
                c->fGain[1]     = GAIN_AMP_0_DB;

                c->pIn          = NULL;
                c->pOut         = NULL;
                c->pDry         = NULL;
                c->pWet         = NULL;
                c->pOutGain     = NULL;
                c->pInLevel     = NULL;
                c->pOutLevel    = NULL;
            }

            for (size_t i=0; i<nMChannels; ++i)
            {
                mix_channel_t *c = &vMChannels[i];

                c->vIn          = NULL;
                c->fOldGain[0]  = GAIN_AMP_M_INF_DB;
                c->fOldGain[1]  = GAIN_AMP_M_INF_DB;
                c->fGain[0]     = GAIN_AMP_M_INF_DB;
                c->fGain[1]     = GAIN_AMP_M_INF_DB;
                c->fOldPostGain = GAIN_AMP_0_DB;
                c->fPostGain    = GAIN_AMP_0_DB;
                c->bSolo        = false;

                c->pIn          = NULL;
                c->pSolo        = NULL;
                c->pMute        = NULL;
                c->pPhase       = NULL;
                c->pPan         = NULL;
                c->pBalance     = NULL;
                c->pOutGain     = NULL;
                c->pOutLevel    = NULL;
            }

            for (size_t i=0; i<nPChannels; ++i)
                dsp::fill_zero(vTemp[i], BUFFER_SIZE);

            // Bind ports
            lsp_trace("Binding ports");
            size_t port_id      = 0;

            // Bind primary inputs and outpus
            for (size_t i=0; i<nPChannels; ++i)
                vPChannels[i].pIn       = TRACE_PORT(ports[port_id++]);
            for (size_t i=0; i<nPChannels; ++i)
                vPChannels[i].pOut      = TRACE_PORT(ports[port_id++]);

            pBypass                 = TRACE_PORT(ports[port_id++]);

            // Bind mono output for stereo mixer
            if (nPChannels > 1)
            {
                pMonoOut                = TRACE_PORT(ports[port_id++]);
                pBalance                = TRACE_PORT(ports[port_id++]);
            }

            plug::IPort *dry        = TRACE_PORT(ports[port_id++]);
            plug::IPort *wet        = TRACE_PORT(ports[port_id++]);
            plug::IPort *out_gain   = TRACE_PORT(ports[port_id++]);

            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *c = &vPChannels[i];
                c->pDry                 = dry;
                c->pWet                 = wet;
                c->pOutGain             = out_gain;
            }

            // Bind global level meters
            for (size_t i=0; i<nPChannels; ++i)
                vPChannels[i].pInLevel      = TRACE_PORT(ports[port_id++]);
            for (size_t i=0; i<nPChannels; ++i)
                vPChannels[i].pOutLevel     = TRACE_PORT(ports[port_id++]);


            // Bind ports for audio processing channels
            if (nPChannels > 1)
            {
                for (size_t i=0; i<nMChannels; i += 2)
                {
                    mix_channel_t *l        = &vMChannels[i];
                    mix_channel_t *r        = &vMChannels[i+1];

                    l->pIn                  = TRACE_PORT(ports[port_id++]);
                    r->pIn                  = TRACE_PORT(ports[port_id++]);

                    l->pSolo                = TRACE_PORT(ports[port_id++]);
                    l->pMute                = TRACE_PORT(ports[port_id++]);
                    l->pPhase               = TRACE_PORT(ports[port_id++]);
                    r->pSolo                = l->pSolo;
                    r->pMute                = l->pMute;
                    r->pPhase               = l->pPhase;

                    l->pPan                 = TRACE_PORT(ports[port_id++]);
                    r->pPan                 = TRACE_PORT(ports[port_id++]);

                    l->pBalance             = TRACE_PORT(ports[port_id++]);
                    l->pOutGain             = TRACE_PORT(ports[port_id++]);
                    r->pBalance             = l->pBalance;
                    r->pOutGain             = l->pOutGain;

                    l->pOutLevel            = TRACE_PORT(ports[port_id++]);
                    r->pOutLevel            = TRACE_PORT(ports[port_id++]);
                }
            }
            else
            {
                for (size_t i=0; i<nMChannels; ++i)
                {
                    mix_channel_t *c        = &vMChannels[i];

                    c->pIn                  = TRACE_PORT(ports[port_id++]);
                    c->pSolo                = TRACE_PORT(ports[port_id++]);
                    c->pMute                = TRACE_PORT(ports[port_id++]);
                    c->pPhase               = TRACE_PORT(ports[port_id++]);
                    c->pOutGain             = TRACE_PORT(ports[port_id++]);
                    c->pOutLevel            = TRACE_PORT(ports[port_id++]);
                }
            }
        }

        void mixer::destroy()
        {
            Module::destroy();
            do_destroy();
        }

        void mixer::do_destroy()
        {
            vPChannels      = NULL;
            vMChannels      = NULL;
            vWet[0]         = NULL;
            vWet[1]         = NULL;
            vTemp[0]        = NULL;
            vTemp[1]        = NULL;

            // Free previously allocated data chunk
            if (pData != NULL)
            {
                free_aligned(pData);
                pData       = NULL;
            }
        }

        void mixer::update_sample_rate(long sr)
        {
            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *c    = &vPChannels[i];
                c->sBypass.init(sr);
            }
        }

        void mixer::update_settings()
        {
            bool bypass     = pBypass->value() >= 0.5f;

            // Update settings for primary channels
            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *c    = &vPChannels[i];
                c->sBypass.set_bypass(bypass);

                float out_gain          = c->pOutGain->value();
                c->fOldDry              = c->fDry;
                c->fDry                 = c->pDry->value() * out_gain;
                c->fOldWet              = c->fWet;
                c->fWet                 = c->pWet->value() * out_gain;
            }

            // Apply balance
            if (nPChannels > 1)
            {
                primary_channel_t *l    = &vPChannels[0];
                primary_channel_t *r    = &vPChannels[1];

                float balance           = pBalance->value() * 0.01f;
                float bal_l             = 1.0f - balance;
                float bal_r             = 1.0f + balance;
                float pan               = (pMonoOut->value() >= 0.5f) ? 0.5f : 1.0f;

                l->fOldGain[0]          = l->fGain[0];
                l->fOldGain[1]          = l->fGain[1];
                r->fOldGain[0]          = r->fGain[0];
                r->fOldGain[1]          = r->fGain[1];

                l->fGain[0]             = (pan) * bal_l;
                l->fGain[1]             = (1.0f - pan) * bal_r;
                r->fGain[0]             = (1.0f - pan) * bal_l;
                r->fGain[1]             = (pan) * bal_r;
            }
            else
            {
                primary_channel_t *c    = &vPChannels[0];

                c->fOldGain[0]          = c->fGain[0];
                c->fOldGain[1]          = c->fGain[1];

                c->fGain[0]             = GAIN_AMP_0_DB;
                c->fGain[1]             = GAIN_AMP_0_DB;
            }

            // Check soloing option
            bool has_solo   = false;
            for (size_t i=0; i<nMChannels; ++i)
            {
                mix_channel_t *c        = &vMChannels[i];
                c->bSolo                = c->pSolo->value() >= 0.5f;
                if (c->bSolo)
                    has_solo                = true;
            }

            // Update channel configuration
            for (size_t i=0; i<nMChannels; ++i)
            {
                mix_channel_t *c        = &vMChannels[i];

                bool mute               = (c->pMute->value() >= 0.5f) || ((has_solo) && (!c->bSolo));
                float gain              = c->pOutGain->value();
                float post_gain         = (mute) ? 0.0f : 1.0f;
                if (c->pPhase->value() >= 0.5f)
                    post_gain               = -post_gain;

                c->fOldGain[0]          = c->fGain[0];
                c->fOldGain[1]          = c->fGain[1];
                c->fOldPostGain         = c->fPostGain;

                c->fGain[0]             = gain;
                c->fGain[1]             = gain;
                c->fPostGain            = post_gain;
            }

            // Additional stereo control for stereo mixer
            if (nPChannels > 1)
            {
                for (size_t i=0; i<nMChannels; i += 2)
                {
                    mix_channel_t *l        = &vMChannels[i];
                    mix_channel_t *r        = &vMChannels[i+1];

                    float pan_l             = l->pPan->value() * 0.005f;
                    float pan_r             = r->pPan->value() * 0.005f;
                    float balance           = l->pBalance->value() * 0.01f;
                    float bal_l             = 1.0f - balance;
                    float bal_r             = 1.0f + balance;

                    l->fGain[0]            *= (0.5f - pan_l) * bal_l;
                    l->fGain[1]            *= (0.5f + pan_l) * bal_r;
                    r->fGain[0]            *= (0.5f - pan_r) * bal_l;
                    r->fGain[1]            *= (0.5f + pan_r) * bal_r;
                }
            }
        }

        void mixer::process(size_t samples)
        {
            // Obtain audio buffers
            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *c    = &vPChannels[i];
                c->vIn                  = c->pIn->buffer<float>();
                c->vOut                 = c->pOut->buffer<float>();
            }
            for (size_t i=0; i<nMChannels; ++i)
                vMChannels[i].vIn       = vMChannels[i].pIn->buffer<float>();

            // Main processing
            while (samples > 0)
            {
                size_t to_process           = lsp_min(samples, BUFFER_SIZE);

                // Do the mixing stuff
                if (nPChannels > 1)
                {
                    // Stereo
                    // Clear wet buffer
                    dsp::fill_zero(vWet[0], samples);
                    dsp::fill_zero(vWet[1], samples);

                    // Apply mixing stuff
                    for (size_t i=0; i<nMChannels; i += 2)
                    {
                        mix_channel_t *l        = &vMChannels[i];
                        mix_channel_t *r        = &vMChannels[i+1];

                        // Perform audio mixing of input stereo signal
                        dsp::lramp2(vTemp[0], l->vIn, l->fOldGain[0], l->fGain[0], to_process);
                        dsp::lramp2(vTemp[1], l->vIn, l->fOldGain[1], l->fGain[1], to_process);
                        dsp::lramp_add2(vTemp[0], r->vIn, r->fOldGain[0], r->fGain[0], to_process);
                        dsp::lramp_add2(vTemp[1], r->vIn, r->fOldGain[1], r->fGain[1], to_process);

                        // Perform output level metering
                        float out_l             = dsp::abs_max(vTemp[0], to_process);
                        float out_r             = dsp::abs_max(vTemp[1], to_process);
                        l->pOutLevel->set_value(out_l);
                        r->pOutLevel->set_value(out_r);

                        // Apply mixed channels to the wet signal
                        dsp::lramp_add2(vWet[0], vTemp[0], l->fOldPostGain, l->fPostGain, to_process);
                        dsp::lramp_add2(vWet[1], vTemp[1], r->fOldPostGain, r->fPostGain, to_process);

                        // Renew old parameters
                        l->fOldGain[0]          = l->fGain[0];
                        l->fOldGain[1]          = l->fGain[1];
                        l->fOldPostGain         = l->fPostGain;
                        r->fOldGain[0]          = r->fGain[0];
                        r->fOldGain[1]          = r->fGain[1];
                        r->fOldPostGain         = r->fPostGain;
                    }

                    // Mix dry/wet
                    primary_channel_t *pl   = &vPChannels[0];
                    primary_channel_t *pr   = &vPChannels[1];

                    dsp::lramp2(vTemp[0], vWet[0], pl->fOldWet, pl->fWet, to_process);
                    dsp::lramp2(vTemp[1], vWet[1], pr->fOldWet, pl->fWet, to_process);
                    dsp::lramp_add2(vTemp[0], pl->vIn, pl->fOldDry, pl->fDry, to_process);
                    dsp::lramp_add2(vTemp[1], pr->vIn, pr->fOldDry, pl->fDry, to_process);

                    // Apply balance and mono
                    dsp::lramp2(vWet[0], vTemp[0], pl->fOldGain[0], pl->fGain[0], to_process);
                    dsp::lramp2(vWet[1], vTemp[0], pl->fOldGain[1], pl->fGain[1], to_process);
                    dsp::lramp_add2(vWet[0], vTemp[1], pr->fOldGain[0], pr->fGain[0], to_process);
                    dsp::lramp_add2(vWet[1], vTemp[1], pr->fOldGain[1], pr->fGain[1], to_process);

                    // Renew old parameters
                    pl->fOldWet     = pl->fWet;
                    pl->fOldDry     = pl->fDry;
                    pl->fOldGain[0] = pl->fGain[0];
                    pl->fOldGain[1] = pl->fGain[1];
                    pr->fOldWet     = pr->fWet;
                    pr->fOldDry     = pr->fDry;
                    pr->fOldGain[0] = pr->fGain[0];
                    pr->fOldGain[1] = pr->fGain[1];
                }
                else
                {
                    // Mono
                    dsp::fill_zero(vWet[0], samples);

                    // Apply mixing stuff
                    for (size_t i=0; i<nMChannels; ++i)
                    {
                        mix_channel_t *c        = &vMChannels[i];

                        // Perform audio mixing of input stereo signal
                        dsp::mul_k3(vTemp[0], c->vIn, c->fGain[0], to_process);

                        // Perform output level metering
                        float out               = dsp::abs_max(vTemp[0], to_process);
                        c->pOutLevel->set_value(out);

                        // Apply mixed channels to the wet signal
                        dsp::fmadd_k3(vWet[0], vTemp[0], c->fPostGain, to_process);

                        // Renew old parameters
                        c->fOldGain[0]          = c->fGain[0];
                        c->fOldGain[1]          = c->fGain[1];
                        c->fOldPostGain         = c->fPostGain;
                    }

                    // Mix dry/wet
                    primary_channel_t *pc   = &vPChannels[0];
                    dsp::lramp2(vWet[0], vWet[0], pc->fOldWet, pc->fWet, to_process);
                    dsp::lramp_add2(vWet[0], pc->vIn, pc->fDry, pc->fOldDry, to_process);

                    // Renew old parameters
                    pc->fOldWet     = pc->fWet;
                    pc->fOldDry     = pc->fDry;
                    pc->fOldGain[0] = pc->fGain[0];
                    pc->fOldGain[1] = pc->fGain[1];
                }

                // Apply bypass and metering
                for (size_t i=0; i<nPChannels; ++i)
                {
                    primary_channel_t *c    = &vPChannels[i];

                    c->sBypass.process(c->vOut, c->vIn, vWet[i], to_process);

                    float in_lvl            = dsp::abs_max(c->vIn, to_process);
                    float out_lvl           = dsp::abs_max(vWet[i], to_process);

                    c->pInLevel->set_value(in_lvl);
                    c->pOutLevel->set_value(out_lvl);
                }

                // Update counters and pointers
                samples                    -= to_process;
                for (size_t i=0; i<nPChannels; ++i)
                {
                    primary_channel_t *c    = &vPChannels[i];
                    c->vIn                 += to_process;
                    c->vOut                += to_process;
                }
                for (size_t i=0; i<nMChannels; ++i)
                    vMChannels[i].vIn      += to_process;
            }
        }

        void mixer::dump(dspu::IStateDumper *v) const
        {
            v->begin_array("vPChannels", vPChannels, nPChannels);
            for (size_t i=0; i<nPChannels; ++i)
            {
                primary_channel_t *p = &vPChannels[i];

                v->write_object("sBypass", &p->sBypass);
                v->write("vIn", p->vIn);
                v->write("vOut", p->vOut);
                v->write("fOldDry", p->fOldDry);
                v->write("fDry", p->fDry);
                v->write("fOldWet", p->fOldWet);
                v->write("fWet", p->fWet);
                v->writev("fOldGain", p->fOldGain, 2);
                v->writev("fGain", p->fGain, 2);

                v->write("pIn", p->pIn);
                v->write("pOut", p->pOut);
                v->write("pDry", p->pDry);
                v->write("pWet", p->pWet);
                v->write("pOutGain", p->pOutGain);
                v->write("pInLevel", p->pInLevel);
                v->write("pOutLevel", p->pOutLevel);
            }
            v->end_array();

            v->begin_array("vMChannels", vMChannels, nMChannels);
            for (size_t i=0; i<nMChannels; ++i)
            {
                mix_channel_t *c = &vMChannels[i];

                v->write("vIn", c->vIn);
                v->writev("fOldGain", c->fOldGain, 2);
                v->writev("fGain", c->fGain, 2);
                v->write("fOldPostGain", c->fOldPostGain);
                v->write("fPostGain", c->fPostGain);
                v->write("bSolo", c->bSolo);

                v->write("pIn", c->pIn);
                v->write("pSolo", c->pSolo);
                v->write("pMute", c->pMute);
                v->write("pPhase", c->pPhase);
                v->write("pPan", c->pPan);
                v->write("pBalance", c->pBalance);
                v->write("pOutGain", c->pOutGain);
                v->write("pOutLevel", c->pOutLevel);
            }
            v->end_array();

            v->write("nPChannels", nPChannels);
            v->write("nMChannels", nMChannels);
            v->write("bMonoOut", bMonoOut);
            v->begin_array("vWet", vWet, 2);
            {
                v->write(vWet[0]);
                v->write(vWet[1]);
            }
            v->end_array();
            v->begin_array("vTemp", vTemp, 2);
            {
                v->write(vTemp[0]);
                v->write(vTemp[1]);
            }
            v->end_array();

            v->write("pBypass", pBypass);
            v->write("pMonoOut", pMonoOut);
            v->write("pBalance", pBalance);

            v->write("pData", pData);
        }

    } /* namespace plugins */
} /* namespace lsp */


