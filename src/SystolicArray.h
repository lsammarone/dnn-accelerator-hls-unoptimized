#ifndef SYSTOLIC_ARRAY_H
#define SYSTOLIC_ARRAY_H

#include "ProcessingElement.h"
#include "conv.h"
#include "Fifo.h"
#include "SystolicArrayCore.h"

// Include mc_scverify.h for CCS_* macros
#include <mc_scverify.h>

class SystolicArrayLooper
{
public:
    SystolicArrayLooper() {}

#pragma hls_design interface
void run(ac_channel<Params> &paramsIn,
         ac_channel<Params> &paramsOut,
         ac_channel<LoopIndices> &loopIndicesOut)
    {
        // -------------------------------
        // Generate the loop indices here for the systolic array.
        // Write the loop indices as well as the params out to channels.
        // Your code starts here
        Params params = paramsIn.read();
        for (uint_16 i = 0; i < params.OY1; i++ ){
            for (uint_16 j = 0; j < params.OX1; j++){
                for (uint_16 m = 0; m < params.OC1; m++){
                    for(uint_16 n = 0; n < params.IC1; n++){
                        for (uint_16 y = 0; y < params.FY; y++){
                            for(uint_16 x = 0; x < params.FX; x++) {
                                LoopIndices loopIndices_for_Core;
                                loopIndices_for_Core.fx_idx = x;
                                loopIndices_for_Core.fy_idx = y;
                                loopIndices_for_Core.ic1_idx = n;
                                paramsOut.write(params);
                                loopIndicesOut.write(loopIndicesCore);
                            }
                        }
                    }
                }
            }
        }
        // Your code ends here
        // -------------------------------
    }
};

template <typename IDTYPE, typename WDTYPE, typename ODTYPE, int OC0, int IC0>
class SystolicArrayWrapper
{
public:
    SystolicArrayWrapper(){}
    
#pragma hls_design interface
    void run(ac_channel<PackedInt<INPUT_PRECISION, IC0> > &input, 
             ac_channel<PackedInt<WEIGHT_PRECISION, OC0> > &weight, 
             ac_channel<PackedInt<OUTPUT_PRECISION, OC0> > &output,
             ac_channel<Params> &paramsIn)
    {
        systolicArrayLooper.run(paramsIn, paramsChannel, loopIndicesChannel);
        systolicArrayCore.run(input, weight, output, paramsChannel, loopIndicesChannel);
    }
private:
    SystolicArrayCore<IDTYPE, WDTYPE, ODTYPE, OC0, IC0> systolicArrayCore;
    SystolicArrayLooper systolicArrayLooper;
    ac_channel<Params> paramsChannel;
    ac_channel<LoopIndices> loopIndicesChannel;
};

#endif
