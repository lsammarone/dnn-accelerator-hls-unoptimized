#ifndef INPUT_DOUBLE_BUFFER_H
#define INPUT_DOUBLE_BUFFER_H


template <int size, int IC0, int OC0>
class InputDoubleBufferWriter{
public:
    InputDoubleBufferWriter(){}

    #pragma hls_design interface
    void CCS_BLOCK(run)(ac_channel<Params> &paramsIn,
                        ac_channel<PackedInt<INPUT_PRECISION, 4> > &din,
                        ac_channel<chanStruct<PackedInt<INPUT_PRECISION,IC0>,size> > &dout)
    {
        // -------------------------------
        // Your code starts here
        Params params = paramsIn.read();
        uint_16 IX0 = (params.OX0-1)*params.STRIDE + params.FX;
        uint_16 IY0 = (params.OY0-1)*params.STRIDE + params.FY;
        uint_16 total_number_block = params.OY1 * params.OX1;

        for (uint_16 i=0; i < params.OY1 * params.OX1; i++ ){
            chanStruct<PackedInt<INPUT_PRECISION, IC0>, size> temp;
            for (uint_16 j=0; j < params.IC1 * IX0 * IY0; j++){
                PackedInt<INPUT_PRECISION, IC0> temp_data;
                for (uint_16 k=0; k <IC0; k+=4) {
                    PackedInt<INPUT_PRECISION, 4> temp_din = din.read();
                    temp_data.value[k] = temp_din .value[0];
                    temp_data.value[k+1] = temp_din .value[1];
                    temp_data.value[k+2] = temp_din .value[2];
                    temp_data.value[k+3] = temp_din .value[3];
                }
                temp.data[j] = temp_data;
            }
            dout.write(temp);
        }
        
        /*for (uint_16 i= 0; i < total_number_block; i++) {
            chanStruct<PackedInt<INPUT_PRECISION,IC0>,size> temp;
            for(uint_16 loopic1 = 0; loopic1 < params.IC1; loopic1++) {
                for(uint_16 loopiy0 = 0; loopiy0 < IY0; loopiy0++) {
                    for(uint_16 loopix0 = 0; loopix0 < IX0; loopix0++){
                        PackedInt<INPUT_PRECISION, IC0> temp_data;
                        for(int j = 0; j < IC0; j+=4) {
                            PackedInt<INPUT_PRECISION, 4> temp_din = din.read();
                            temp_data.value[j] = temp_din.value[0];
                            temp_data.value[j+1] = temp_din.value[1];
                            temp_data.value[j+2] = temp_din.value[2];
                            temp_data.value[j+3] = temp_din.value[3];
                        }
                        uint_16 address = loopix0 + loopiy0*IX0 + loopic1*IX0*IY0;
                        temp.data[address] = temp_data;
                    }
                }
            }
            dout.write(temp);
        } */
        // Your code ends here
        // -------------------------------
    }
};

template <int size, int IC0, int OC0>
class InputDoubleBufferReader{
public:
    InputDoubleBufferReader(){}

    #pragma hls_design interface
    void CCS_BLOCK(run)(ac_channel<Params> &paramsIn,
                        ac_channel<chanStruct<PackedInt<INPUT_PRECISION, IC0>,size> > &din, 
                        ac_channel<PackedInt<INPUT_PRECISION, IC0> > &dout)
    {
        // -------------------------------
        // Your code starts here
        Params params = paramsIn.read();
        uint_16 IX0 = (params.OX0-1)*params.STRIDE + params.FX;
        uint_16 IY0 = (params.OY0-1)*params.STRIDE + params.FY;
        uint_16 total_number_block = params.OY1 * params.OX1;

        for(uint_16 i = 0; i < total_number_block; i++) {
            chanStruct<PackedInt<INPUT_PRECISION, IC0>,size> temp = din.read();
            for(uint_16 loopoc1 = 0; loopoc1 < params.OC1; loopoc1++){  
                for(uint_16 loopic1 = 0; loopic1 < params.IC1; loopic1++) {  
                    for(uint_16 loopfy = 0; loopfy < params.FY; loopfy++) {
                        for(uint_16 loopfx = 0; loopfx < params.FX; loopfx++) {
                            for (uint_16 loopoy0 = 0; loopoy0 < params.OY0; loopoy0++) {
                                for(uint_16 loopox0 = 0; loopox0 < params.OX0; loopox0++){
                                
                                //assign ix0 = config_STRIDE * ox0 + fx;
                                //assign iy0 = config_STRIDE * oy0 + fy;
                                //ssign adrc = ic1 * config_IX0 * config_IY0 + iy0 * config_IX0 + ix0;

                                uint_16 ix_0 = params.STRIDE * loopox0 + loopfx;
                                uint_16 iy_0 = params.STRIDE * loopoy0 + loopfy;
                                uint_16 address = loopic1 * IX0 * IY0 + iy_0 * IX0 + ix_0;
                                PackedInt<INPUT_PRECISION, IC0> temp1 = temp.data[address];

                                dout.write(temp1);
                                }
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

template <int size, int IC0, int OC0>
class InputDoubleBuffer{
public:
  InputDoubleBuffer(){}

  #pragma hls_design interface
  void CCS_BLOCK(run)(ac_channel<PackedInt<INPUT_PRECISION, 4> > &inputs_in, 
                      ac_channel<PackedInt<INPUT_PRECISION, IC0> > &inputs_out,
                      ac_channel<Params> &paramsIn)
    {

        Params params = paramsIn.read();

        inputDoubleBufferReaderParams.write(params);
        inputDoubleBufferWriterParams.write(params);

        inputDoubleBufferWriter.run(inputDoubleBufferWriterParams, inputs_in, mem);

        inputDoubleBufferReader.run(inputDoubleBufferReaderParams, mem, inputs_out);
    }

private:
    ac_channel<chanStruct<PackedInt<INPUT_PRECISION, IC0>,size> > mem;
    
    InputDoubleBufferWriter<size, IC0, OC0> inputDoubleBufferWriter;
    ac_channel<Params> inputDoubleBufferWriterParams;
    
    InputDoubleBufferReader<size, IC0, OC0> inputDoubleBufferReader;
    ac_channel<Params> inputDoubleBufferReaderParams;
};

#endif
