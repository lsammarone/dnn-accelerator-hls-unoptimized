#ifndef SYSTOLIC_ARRAY_CORE_H
#define SYSTOLIC_ARRAY_CORE_H

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/comparison/not_equal.hpp>
#include <boost/preprocessor/repetition/for.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/size.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/arithmetic/dec.hpp>

#include "ProcessingElement.h"
#include "Fifo.h"


struct LoopIndices{
    uint_16 ic1_idx;
    uint_16 fx_idx;
    uint_16 fy_idx;
};



template <typename IDTYPE, typename WDTYPE, typename ODTYPE, int OC0, int IC0>
class SystolicArrayCore
{
public:
    SystolicArrayCore() {}

#pragma hls_design interface
    void CCS_BLOCK(run)(
        ac_channel<PackedInt<INPUT_PRECISION, IC0> > &input, 
        ac_channel<PackedInt<WEIGHT_PRECISION, OC0> > &weight, 
        ac_channel<PackedInt<OUTPUT_PRECISION, OC0> > &output,
        ac_channel<Params> &paramsIn,
        ac_channel<LoopIndices> &loopIndicesIn)
    {
        #ifndef __SYNTHESIS__
        //assert(params.OX0 * params.OY0 < OC+IC-1);
        // Debug example:
        // printf("paramsIn channel size: %d\n", paramsIn.size());
        // printf("weigh channel size: %d\n", weight.size());
        // printf("input channel size: %d\n\n", input.size());
        #endif

        #ifndef __SYNTHESIS__
        while(paramsIn.available(1))
        #endif
        {
            // -------------------------------
            // Read in the params and loop indices from the channel
            // Your code starts here
            Params params = paramsIn.read();
            LoopIndices loopIndices = loopIndicesIn.read();
            // Your code ends here
            // -------------------------------


            // -------------------------------
            // Create a loop for a "run" of the systolic array.
            // The number of steps in a run of the systolic array is equal to:
            // the ramp-up time + number of pixels + flush time
            // Your code starts here
            for(uint_16 step=0; step < (params.OX0 * params.OY0 + OC0 + IC0 -1); step++ ){
            // Your code ends here 
            // You should now be in the body of the loop
            // -------------------------------

                // -------------------------------
                // If you are in the ramp up time, read in weights from the channel
                // and store it in the weights array
                // Your code starts here
                if (step < IC0) {
                    PackedInt<WEIGHT_PRECISION, OC0> weight_eachchannel = weight.read();
                        reg_weight.value[step] = weight_eachchannel;
                }
                // Your code ends here
                // -------------------------------
                
                
                PackedInt<INPUT_PRECISION, IC0> in_col;

                // -------------------------------
                // Read inputs from the channel and store in the variable in_col
                // Note: you don't read in any inputs during the flush time
                // Your code starts here
                if (step < (params.OX0 * params.OY0) ){
                    in_col = input.read();
                }
                // Your code ends here
                // -------------------------------

                // Debug example:        
                // printf("in_col: %s\n", in_col.to_string());


                /*
                 * FIFOs for inputs coming in to the systolic array
                 * assign values to in_col, and the skewed version will be in input_buf
                 */
                PackedInt<INPUT_PRECISION, IC0> input_buf;

                #define INPUT_FIFO_BODY(z,i,unused) \
                    IDTYPE BOOST_PP_CAT(input_fifo_output_, i); \
                    IDTYPE BOOST_PP_CAT(input_fifo_input_, i) = in_col.value[i]; \
                    BOOST_PP_CAT(input_fifo_, i).run( BOOST_PP_CAT(input_fifo_input_, i) , BOOST_PP_CAT(input_fifo_output_, i) ); \
                    input_buf.value[i] = BOOST_PP_CAT(input_fifo_output_, i);
                
                REPEAT(INPUT_FIFO_BODY)

                // -------------------------------
                // Assign values from input_buf into the registers for the first column of PEs
                // Your code starts here 
                for (int x=0; x<IC0; x++){
                    reg_input_in.value[0].value[x+1]=input_buf.value[x];
                }
                // Your code ends here
                // -------------------------------

                PackedInt<OUTPUT_PRECISION, OC0> psum_buf;
                
                // -------------------------------
                // Set partial outputs for the array to psum_buf.
                // Depending on the loop index, the partial output will be 0 or a value from the accumulation buffer
                // Your code starts here
                if (step < params.OX0 * params.OY0){  //NEED TO CHECK IF THE RIGHT CONDITION
                    if(loopIndices.fx_idx==0 && loopIndices.fy_idx==0 && loopIndices.ic1_idx==0){
                        for (int z=0; z < OC0; z++){
                            psum_buf.value[z]=0;
                        }
                    }
                    else{
                        psum_buf = accumulation_buffer.value[step];   //NEED TO DO a value from the accumulation buffer
                    }
                }
                // Your code ends here
                // -------------------------------
                
                // Debug example:
                // printf("psum_buf: %s\n", psum_buf.to_string());

                /*
                 * FIFOs for partial outputs coming in to the systolic array
                 * assign values to psum_buf, and the skewed version will be in output_buf
                 */
                PackedInt<OUTPUT_PRECISION, OC0> output_buf;
                #define ACCUM_FIFO_BODY(z,i,unused) \
                    ODTYPE BOOST_PP_CAT(psum_fifo_output_, i); \
                    ODTYPE BOOST_PP_CAT(psum_fifo_input_, i) = psum_buf.value[i]; \
                    BOOST_PP_CAT(psum_fifo_, i).run( BOOST_PP_CAT(psum_fifo_input_, i) , BOOST_PP_CAT(psum_fifo_output_, i) ); \
                    output_buf.value[i] = BOOST_PP_CAT(psum_fifo_output_, i);
                
                REPEAT(ACCUM_FIFO_BODY)
        
                // -------------------------------
                // Assign values from output_buf into the partial sum registers for the first row of PEs
                // Your code starts here
                for (int y=0; y < OC0; y++){
                    reg_psum_in.value[0].value[y+1]=output_buf.value[y];
                }
                // Your code ends here
                // -------------------------------
            

                // -------------------------------
                // Run the 16x16 PE array
                // Make sure that the correct registers are given to the PE
                // Your code starts here
                for (int x=0; x<OC0; x++){
                    for (int y=0; y<IC0; y++){
                        PE_unit[y][x].run(reg_input_in.value[x].value[y+1],reg_psum_in.value[x].value[y+1],reg_weight.value[x].value[y],reg_input_out.value[x+1].value[y+1],reg_psum_out.value[x+1].value[y+1]); //NEED TO DO
                    }
                }
                // Your code ends here
                // -------------------------------
                

                /*
                 * FIFOs for partial outputs coming out of the systolic array
                 * The skewed version will be in the variable output_row
                 */
                PackedInt<OUTPUT_PRECISION, OC0> output_row;

                #define FIFO_WRITE_BODY_NEW(z,i,unused)\
                    ODTYPE BOOST_PP_CAT(accum_fifo_output_, i); \
                    BOOST_PP_CAT(accum_fifo_, i).run( reg_psum_in.value[IC0].value[i+1], BOOST_PP_CAT(accum_fifo_output_, i) ); \
                    output_row.value[i] = BOOST_PP_CAT(accum_fifo_output_,i); \
                
                REPEAT(FIFO_WRITE_BODY_NEW)

                // -------------------------------
                // After a certain number of cycles, you will have valid output from the systolic array
                // Depending on the loop indices, this valid output will either be written into the accumulation buffer or written out
                // Your code starts here
                if (step >= OC0+IC0-1){ 
                    if ((loopIndices.fx_idx==params.FX-1) && (loopIndices.fy_idx==params.FY-1) && (loopIndices.ic1_idx==params.IC1-1)){
                        output.write(output_row);
                    } //THIS IS WHEN THE OX*OY CYCLES FINISH, LOAD THE OUTPUT OF ONE SYSTOLICARRRAYCORE (ONE SYSTOLICARRAYCORE HAS OX*OY+RAMPUP+FLUSH CYCLES )
                    accumulation_buffer.value[step - (OC0+IC0-1)] = output_row; //
                }
                // Your code ends here
                // -------------------------------
                
                // -------------------------------
                // Cycle the input/psum registers
                // That is, the outputs that a PE wrote to should now become the input for the next PE
                // Your code starts here
                for (int i = 0; i < OC0; i++) {  //SECOND LINE LOOP BY IC0
                    reg_input_in.value[i+1] = reg_input_out.value[i+1];
                    reg_psum_in.value[i+1] = reg_psum_out.value[i+1];
                }
                // Your code ends here
                // -------------------------------
            }
        }
    
        // Debug example:
        // printf("outputs written: %d\n", output.size());
    }

private:
    
    // -------------------------------
    // Create the following:
    //  - PE array
    //  - accumulation buffer
    //  - weight registers
    //  - input registers (two sets, one at the input of the PE and one at the output) 
    //  - psum registers (two sets, one at the input of the PE and one at the output) 
    // Your code starts here
    PackedInt2D<WEIGHT_PRECISION, OC0, IC0> reg_weight;
    PackedInt2D<INPUT_PRECISION, IC0+1, OC0+1> reg_input_in;
    PackedInt2D<OUTPUT_PRECISION, OC0+1, IC0+1> reg_psum_in;
    ProcessingElement<IDTYPE, ODTYPE> PE_unit[IC0][OC0];
    PackedInt2D<INPUT_PRECISION, IC0+1, OC0+1> reg_input_out;
    PackedInt2D<OUTPUT_PRECISION, OC0+1, IC0+1> reg_psum_out;

    PackedInt2D<OUTPUT_PRECISION, OC0, ACCUMULATION_BUFFER_SIZE> accumulation_buffer;



    // Your code ends here
    // -------------------------------
    

#define INPUT_FIFOS_INIT(z, i, unused) \
    Fifo<IDTYPE, i + 1> BOOST_PP_CAT(input_fifo_, i);

    REPEAT(INPUT_FIFOS_INIT)

#define ACCUM_FIFOS_INIT(z, i, unused) \
    Fifo<ODTYPE, i + 1> BOOST_PP_CAT(psum_fifo_, i);

    REPEAT(ACCUM_FIFOS_INIT)
    

#define OUTPUT_FIFOS_INIT(z, i, unused) \
    Fifo<ODTYPE, OC0 - i> BOOST_PP_CAT(accum_fifo_, i);
    
    REPEAT(OUTPUT_FIFOS_INIT)
};

#endif
