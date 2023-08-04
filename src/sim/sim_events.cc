/*
 * Copyright (c) 2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2002-2005 The Regents of The University of Michigan
 * Copyright (c) 2013 Advanced Micro Devices, Inc.
 * Copyright (c) 2013 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sim/sim_events.hh"

#include <iomanip>
#include <string>

#include "base/callback.hh"
#include "burst_counter.hh"
#include "sim/eventq.hh"
#include "sim/sim_exit.hh"
#include "sim/stats.hh"

namespace gem5
{
extern burstCounter bc;
extern Tick BCClockPeriod;

GlobalSimLoopExitEvent::GlobalSimLoopExitEvent(Tick when,
                                               const std::string &_cause,
                                               int c, Tick r)
    : GlobalEvent(when, Sim_Exit_Pri, IsExitEvent),
      cause(_cause), code(c), repeat(r)
{
}

GlobalSimLoopExitEvent::GlobalSimLoopExitEvent(const std::string &_cause,
                                               int c, Tick r)
    : GlobalEvent(curTick(), Minimum_Pri, IsExitEvent),
      cause(_cause), code(c), repeat(r)
{
}

const char *
GlobalSimLoopExitEvent::description() const
{
    return "global simulation loop exit";
}

//
// handle termination event
//
void
GlobalSimLoopExitEvent::process()
{
    if (repeat) {
        schedule(curTick() + repeat);
    }
}

void
exitSimLoop(const std::string &message, int exit_code, Tick when, Tick repeat,
            bool serialize)
{
    warn_if(serialize && (when != curTick() || repeat),
            "exitSimLoop called with a delay and auto serialization. This is "
            "currently unsupported.");

    bc.dumpBCtoFile<1>(16, bc.eventIn16Cycles);
    bc.dumpBCtoFile<2>(16, bc.eventIn16CyclesV2);
    bc.dumpBCtoFile<1>(32, bc.eventIn32Cycles);
    bc.dumpBCtoFile<2>(32, bc.eventIn32CyclesV2);
    bc.dumpBCtoFile<1>(64, bc.eventIn64Cycles);
    bc.dumpBCtoFile<2>(64, bc.eventIn64CyclesV2);
    bc.dumpBCtoFile<1>(128, bc.eventIn128Cycles);
    bc.dumpBCtoFile<2>(128, bc.eventIn128CyclesV2);
    bc.dumpBCtoFile<1>(256, bc.eventIn256Cycles);
    bc.dumpBCtoFile<2>(256, bc.eventIn256CyclesV2);
    std::cout << "1145141919810" << std::endl;
    bc.printEventNames();
    bc.printAllCounters();

    new GlobalSimLoopExitEvent(when + simQuantum, message, exit_code, repeat);
}

void
exitSimLoopNow(const std::string &message, int exit_code, Tick repeat,
               bool serialize)
{
    new GlobalSimLoopExitEvent(message, exit_code, repeat);
}

LocalSimLoopExitEvent::LocalSimLoopExitEvent(const std::string &_cause, int c,
                                   Tick r)
    : Event(Sim_Exit_Pri, IsExitEvent),
      cause(_cause), code(c), repeat(r)
{
}

//
// handle termination event
//
void
LocalSimLoopExitEvent::process()
{
    exitSimLoop(cause, 0);
}


const char *
LocalSimLoopExitEvent::description() const
{
    return "simulation loop exit";
}

void
LocalSimLoopExitEvent::serialize(CheckpointOut &cp) const
{
    Event::serialize(cp);

    SERIALIZE_SCALAR(cause);
    SERIALIZE_SCALAR(code);
    SERIALIZE_SCALAR(repeat);
}

void
LocalSimLoopExitEvent::unserialize(CheckpointIn &cp)
{
    Event::unserialize(cp);

    UNSERIALIZE_SCALAR(cause);
    UNSERIALIZE_SCALAR(code);
    UNSERIALIZE_SCALAR(repeat);
}

//
// constructor: automatically schedules at specified time
//
CountedExitEvent::CountedExitEvent(const std::string &_cause, int &counter)
    : Event(Sim_Exit_Pri), cause(_cause), downCounter(counter)
{
    // catch stupid mistakes
    assert(downCounter > 0);
}


//
// handle termination event
//
void
CountedExitEvent::process()
{
    if (--downCounter == 0) {
        exitSimLoop(cause, 0);
    }
}


const char *
CountedExitEvent::description() const
{
    return "counted exit";
}

} // namespace gem5
