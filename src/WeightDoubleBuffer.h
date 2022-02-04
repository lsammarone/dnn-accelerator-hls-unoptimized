#ifndef WEIGHT_DOUBLE_BUFFER_H
#define WEIGHT_DOUBLE_BUFFER_H


template <int size, int IC0, int OC0>
class WeightDoubleBufferWriter{
public:
    WeightDoubleBufferWriter(){}

    #pragma hls_design interface
    void CCS_BLOCK(run)(ac_channel<Params> &paramsIn,
                        ac_channel<PackedInt<WEIGHT_PRECISION, 4> > &din,
                        ac_channel<chanStruct<PackedInt<WEIGHT_PRECISION, OC0>, size> > &dout)
    {
        // -------------------------------
        // Your code starts here
        Params params = paramsIn.read();
        for(int i = 0; i < params.OX1 * params.OY1 * params.OC1; i++){
        chanStruct<PackedInt<WEIGHT_PRECISION,OC0>,size> temp;
          for(int j = 0; j < params.FX * params.FY * params.IC1 * IC0; j++) {
              PackedInt<WEIGHT_PRECISION, OC0> temp_data;
              for(int k = 0; k < OC0; k+=4) {
                  PackedInt<WEIGHT_PRECISION, 4> temp_din = din.read();
                  temp_data.value[k] = temp_din.value[0];
                  temp_data.value[k+1] = temp_din.value[1];
                  temp_data.value[k+2] = temp_din.value[2];
                  temp_data.value[k+3] = temp_din.value[3];
              }
              temp.data[j] = temp_data;
          }
        dout.write(temp);
        }
        // Your code ends here
        // -------------------------------
    }
};

template <int size, int IC0, int OC0>
class WeightDoubleBufferReader{
public:
    WeightDoubleBufferReader(){}

    #pragma hls_design interface
    void CCS_BLOCK(run)(ac_channel<Params> &paramsIn,
                        ac_channel<chanStruct<PackedInt<WEIGHT_PRECISION, OC0>,size> > &din, 
                        ac_channel<PackedInt<WEIGHT_PRECISION, OC0> > &dout)
    {
        // -------------------------------
        // Your code starts here
        Params params = paramsIn.read();
        for(int i = 0; i < params.OX1 * params.OY1 * params.OC1; i++){
          chanStruct<PackedInt<WEIGHT_PRECISION, OC0>,size> temp_din = din.read();
          for (int j = 0; j < params.FX * params.FY * params.IC1 * IC0; j++) {
              PackedInt<WEIGHT_PRECISION, OC0> temp1 = temp_din.data[j];
              dout.write(temp1);
          }
        }
        // Your code ends here
        // -------------------------------
    }
};

template <int size, int IC0, int OC0>
class WeightDoubleBuffer{
public:
  WeightDoubleBuffer(){}

  #pragma hls_design interface
  void CCS_BLOCK(run)(ac_channel<PackedInt<WEIGHT_PRECISION, 4> > &weights_in, 
                      ac_channel<PackedInt<WEIGHT_PRECISION, OC0> > &weights_out,
                      ac_channel<Params> &paramsIn)
    {
        Params params = paramsIn.read();

        // #ifndef __SYNTHESIS__
        // ac_int<ac::log2_ceil<size>::val, false> block_size = IC0*params.IC1*params.FX*params.FY;
        // assert(block_size <= size);
        // #endif

        weightDoubleBufferReaderParams.write(params);
        weightDoubleBufferWriterParams.write(params);

        weightDoubleBufferWriter.run(weightDoubleBufferWriterParams, weights_in, mem);
        weightDoubleBufferReader.run(weightDoubleBufferReaderParams, mem, weights_out);
    }

private:
    ac_channel<chanStruct<PackedInt<WEIGHT_PRECISION, OC0>,size> > mem;
    
    WeightDoubleBufferWriter<size, IC0, OC0> weightDoubleBufferWriter;
    ac_channel<Params> weightDoubleBufferWriterParams;
    
    WeightDoubleBufferReader<size, IC0, OC0> weightDoubleBufferReader;
    ac_channel<Params> weightDoubleBufferReaderParams;
};


#endif
