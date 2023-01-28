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

#ifndef PRIVATE_PLUGINS_MIXER_H_
#define PRIVATE_PLUGINS_MIXER_H_

#include <lsp-plug.in/dsp-units/util/Delay.h>
#include <lsp-plug.in/dsp-units/ctl/Bypass.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <private/meta/mixer.h>

namespace lsp
{
    namespace plugins
    {
        /**
         * Base class for the latency compensation delay
         */
        class mixer: public plug::Module
        {
            private:
                mixer & operator = (const mixer &);
                mixer (const mixer &);

            protected:
                typedef struct primary_channel_t
                {
                    dspu::Bypass        sBypass;        // Bypass switch

                    float              *vIn;            // Input buffer pointer
                    float              *vOut;           // Output buffer pointer
                    float               fDry;           // Dry signal amount
                    float               fWet;           // Wet signal amount

                    plug::IPort        *pBypass;        // Bypass switch
                    plug::IPort        *pIn;            // Input data port
                    plug::IPort        *pOut;           // Output data port
                    plug::IPort        *pDry;           // Dry signal amount
                    plug::IPort        *pWet;           // Wet signal amount
                    plug::IPort        *pOutGain;       // Output gain
                    plug::IPort        *pInLevel;       // Input level meter
                    plug::IPort        *pOutLevel;      // Output level meter
                } primary_channel_t;

                typedef struct mix_channel_t
                {
                    float              *vIn;            // Input buffer
                    float               fGain[2];       // Gain for left and right outputs
                    bool                bSolo;          // Solo flag

                    plug::IPort        *pIn;            // Input data port
                    plug::IPort        *pSolo;          // Solo switch
                    plug::IPort        *pMute;          // Mute switch
                    plug::IPort        *pPhase;         // Phase invert switch
                    plug::IPort        *pPan;           // Panning port
                    plug::IPort        *pBalance;       // Balance port
                    plug::IPort        *pOutGain;       // Output gain
                    plug::IPort        *pOutLevel;      // Output level meter
                } mix_channel_t;

            protected:
                primary_channel_t  *vPChannels;         // Primary channels
                mix_channel_t      *vMChannels;         // Mixer input channels
                size_t              nPChannels;         // Number of primary channels (1 for mono, 2 for stereo)
                size_t              nMChannels;         // Number of mixer channels
                bool                bMonoOut;           // Mono output (for stereo mixer)
                float              *vTemp[2];           // Temporary buffers

                plug::IPort        *pMonoOut;

                uint8_t            *pData;              // Allocated data

            public:
                explicit mixer(const meta::plugin_t *meta);
                virtual ~mixer();

                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports);
                void                destroy();

            public:
                virtual void        update_sample_rate(long sr);
                virtual void        update_settings();
                virtual void        process(size_t samples);
                virtual void        dump(dspu::IStateDumper *v) const;
        };
    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_MIXER_H_ */

