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
                    float              *vSend;          // Send buffer pointer
                    float              *vRet;           // Return buffer pointer
                    float               fOldDry;        // Old dry signal amount
                    float               fDry;           // Dry signal amount
                    float               fOldWet;        // Old wet signal amount
                    float               fWet;           // Wet signal amount
                    float               fOldGain[2];    // Old output gain
                    float               fGain[2];       // Output gain (balanced)

                    plug::IPort        *pIn;            // Input data port
                    plug::IPort        *pOut;           // Output data port
                    plug::IPort        *pSend;          // Send data port
                    plug::IPort        *pRet;           // Return data port
                    plug::IPort        *pDry;           // Dry signal amount
                    plug::IPort        *pWet;           // Wet signal amount
                    plug::IPort        *pOutGain;       // Output gain
                    plug::IPort        *pInLevel;       // Input level meter
                    plug::IPort        *pOutLevel;      // Output level meter
                } primary_channel_t;

                typedef struct mix_channel_t
                {
                    float              *vIn;            // Input buffer
                    float              *vRet;           // Return buffer
                    float               fOldGain[2];    // Old gain value
                    float               fGain[2];       // Gain for left and right outputs
                    float               fOldPostGain;   // Old post-gain value (after metering stage)
                    float               fPostGain;      // Post-gain (after metering stage)
                    bool                bSolo;          // Solo flag

                    plug::IPort        *pIn;            // Input data port
                    plug::IPort        *pRet;           // Input return port
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
                float              *vWet[2];            // Wet buffers
                float              *vTemp[2];           // Temporary buffers

                plug::IPort        *pBypass;            // Bypass switch
                plug::IPort        *pMonoOut;           // Mono output
                plug::IPort        *pBalance;           // Balance control

                uint8_t            *pData;              // Allocated data

            protected:
                void                do_destroy();

            public:
                explicit mixer(const meta::plugin_t *meta);
                virtual ~mixer() override;

                virtual void        init(plug::IWrapper *wrapper, plug::IPort **ports) override;
                void                destroy() override;

            public:
                virtual void        update_sample_rate(long sr) override;
                virtual void        update_settings() override;
                virtual void        process(size_t samples) override;
                virtual void        dump(dspu::IStateDumper *v) const override;
        };
    } /* namespace plugins */
} /* namespace lsp */


#endif /* PRIVATE_PLUGINS_MIXER_H_ */

