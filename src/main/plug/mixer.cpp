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
            &meta::mixer_x8_mono,
            &meta::mixer_x16_mono,
            &meta::mixer_x8_stereo,
            &meta::mixer_x16_stereo
        };

        static plug::Module *plugin_factory(const meta::plugin_t *meta)
        {
            return new mixer(meta);
        }

        static plug::Factory factory(plugin_factory, plugins, 4);

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
            vTemp[0]        = NULL;
            vTemp[1]        = NULL;

            pMonoOut        = NULL;

            pData           = NULL;
        }

        mixer::~mixer()
        {
            destroy();
        }

        void mixer::init(plug::IWrapper *wrapper, plug::IPort **ports)
        {
            // Call parent class for initialization
            Module::init(wrapper, ports);

            // Estimate the number of bytes to allocate
            size_t szof_pchannels   = align_size(sizeof(primary_channel_t) * nPChannels, DEFAULT_ALIGN);
            size_t szof_mchannels   = align_size(sizeof(mix_channel_t) * nMChannels, DEFAULT_ALIGN);
            size_t szof_temp        = align_size(BUFFER_SIZE * sizeof(float), DEFAULT_ALIGN);
            size_t alloc            = szof_pchannels + szof_mchannels + szof_temp * nPChannels;

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
                c->fDry         = GAIN_AMP_0_DB;
                c->fWet         = GAIN_AMP_0_DB;

                c->pBypass      = NULL;
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
                c->fGain[0]     = 0.0f;
                c->fGain[1]     = 0.0f;
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

            plug::IPort *bypass     = TRACE_PORT(ports[port_id++]);
            for (size_t i=0; i<nPChannels; ++i)
                vPChannels[i].pBypass   = bypass;

            // Bind mono output for stereo mixer
            if (nPChannels > 1)
                pMonoOut                = TRACE_PORT(ports[port_id++]);

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

            vPChannels      = NULL;
            vMChannels      = NULL;
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
            // Update sample rate for the bypass processors
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c    = &vChannels[i];
                c->sLine.init(dspu::millis_to_samples(sr, meta::mixer::DELAY_OUT_MAX_TIME));
                c->sBypass.init(sr);
            }
        }

        void mixer::update_settings()
        {
            float out_gain          = pGainOut->value();
            bool bypass             = pBypass->value() >= 0.5f;

            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                // Store the parameters for each processor
                c->fDryGain             = c->pDry->value() * out_gain;
                c->fWetGain             = c->pWet->value() * out_gain;
                c->nDelay               = c->pDelay->value();

                // Update processors
                c->sLine.set_delay(c->nDelay);
                c->sBypass.set_bypass(bypass);
            }
        }

        void mixer::process(size_t samples)
        {
            // Process each channel independently
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                // Get input and output buffers
                const float *in         = c->pIn->buffer<float>();
                float *out              = c->pOut->buffer<float>();
                if ((in == NULL) || (out == NULL))
                    continue;

                // Input and output gain meters
                float in_gain           = 0.0f;
                float out_gain          = 0.0f;

                // Process the channel with BUFFER_SIZE chunks
                // Note: since input buffer pointer can be the same to output buffer pointer,
                // we need to store the processed signal data to temporary buffer before
                // it gets processed by the dspu::Bypass processor.
                for (size_t n=0; n<samples; )
                {
                    size_t count            = lsp_min(samples - n, BUFFER_SIZE);

                    // Pre-process signal (fill buffer)
                    c->sLine.process_ramping(vBuffer, in, c->fWetGain, c->nDelay, samples);

                    // Apply 'dry' control
                    if (c->fDryGain > 0.0f)
                        dsp::fmadd_k3(vBuffer, in, c->fDryGain, count);

                    // Compute the gain of input and output signal.
                    in_gain             = lsp_max(in_gain, dsp::abs_max(in, samples));
                    out_gain            = lsp_max(out_gain, dsp::abs_max(vBuffer, samples));

                    // Process the
                    //  - dry (unprocessed) signal stored in 'in'
                    //  - wet (processed) signal stored in 'vBuffer'
                    // Output the result to 'out' buffer
                    c->sBypass.process(out, in, vBuffer, count);

                    // Increment pointers
                    in          +=  count;
                    out         +=  count;
                    n           +=  count;
                }

                // Update meters
                c->pInLevel->set_value(in_gain);
                c->pOutLevel->set_value(out_gain);

                // Output the delay value in milliseconds
                float millis = dspu::samples_to_millis(fSampleRate, c->nDelay);
                c->pOutDelay->set_value(millis);
            }
        }

        void mixer::dump(dspu::IStateDumper *v) const
        {
            // It is very useful to dump plugin state for debug purposes
            v->write("nChannels", nChannels);
            v->begin_array("vChannels", vChannels, nChannels);
            for (size_t i=0; i<nChannels; ++i)
            {
                channel_t *c            = &vChannels[i];

                v->begin_object(c, sizeof(channel_t));
                {
                    v->write_object("sLine", &c->sLine);
                    v->write_object("sBypass", &c->sBypass);

                    v->write("nDelay", c->nDelay);
                    v->write("fDryGain", c->fDryGain);
                    v->write("fWetWain", c->fWetGain);

                    v->write("pIn", c->pIn);
                    v->write("pOut", c->pOut);
                    v->write("pDelay", c->pDelay);
                    v->write("pDry", c->pDry);
                    v->write("pWet", c->pWet);

                    v->write("pOutDelay", c->pOutDelay);
                    v->write("pInLevel", c->pInLevel);
                    v->write("pOutLevel", c->pOutLevel);
                }
                v->end_object();
            }
            v->end_array();

            v->write("vBuffer", vBuffer);

            v->write("pBypass", pBypass);
            v->write("pGainOut", pGainOut);

            v->write("pData", pData);
        }

    }
}


