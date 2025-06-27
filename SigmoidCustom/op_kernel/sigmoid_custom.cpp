#include "kernel_operator.h"
//#include "sigmoid_custom_tiling.h"
using namespace AscendC;
constexpr int32_t BUFFER_NUM = 2;
class KernelSigmoid {
public:
    __aicore__ inline KernelSigmoid() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, uint32_t totalLength, uint32_t tileNum)
    {
        //考生补充初始化代码
        //ASSERT(GetBlockNum() > 0 && "block dim can not be zero!");
        //ASSERT(tileNum > 0 && "tile num can not be zero!");
        //ASSERT(BUFFER_NUM > 0 && "buffer num can not be zero!");

        this->blockLength = totalLength/GetBlockNum();
        this->tileNum = tileNum;
        this-> tileLength = this-> blockLength/this->tileNum/BUFFER_NUM;

        //ASSERT(tileLength > 0 && "tile length can not be zero!");
        //if (GetBlockIdx() >= GetBlockNum()) return;

        xGm.SetGlobalBuffer((__gm__ DTYPE_X *)x + this->blockLength * GetBlockIdx(), this->blockLength);
        yGm.SetGlobalBuffer((__gm__ DTYPE_Y *)y + this->blockLength * GetBlockIdx(), this->blockLength);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(DTYPE_X));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, this->tileLength * sizeof(DTYPE_Y));
        pipe.InitBuffer(Tbuf1, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf2, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf3, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf4, this->tileLength * sizeof(half));

    }
    __aicore__ inline void Process()
    {
        //考生补充对“loopCount”的定义，注意对Tiling的处理
        uint32_t loopCount = this -> tileNum * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) {
            CopyIn(i);
            Compute(i);
            CopyOut(i);
        }
    }

private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        //考生补充算子代码
        LocalTensor<DTYPE_X> xLocal = inQueueX.AllocTensor<DTYPE_X>();
        DataCopy(xLocal, xGm[progress* this->tileLength], this->tileLength);

        inQueueX.EnQue<DTYPE_X>(xLocal);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        //考生补充算子计算代码
        LocalTensor<half> xLocal = inQueueX.DeQue<half>();
        //if(0 == GetBlockIdx()&& progress == 0)
        //    xLocal.Print(16);
        //if(0 == GetBlockIdx())
            //printf("size=%d\n", xLocal.GetSize());
            //xLocal.Print();
        LocalTensor<half> negXLocal = Tbuf1.Get<half>();   
        LocalTensor<half> expNegXLocal = Tbuf2.Get<half>();    
        LocalTensor<half> minExpXLocal = Tbuf3.Get<half>(); 
        LocalTensor<half> oneTensor = Tbuf4.Get<half>();   
        LocalTensor<half> yLocal = outQueueY.AllocTensor<half>();
        
        half minus = -1;
        half one = 1;

        Muls(negXLocal, xLocal, minus, this->tileLength); 
        //if(0 == GetBlockIdx()&& progress == 0)
        //    negXLocal.Print(16);

        Exp(expNegXLocal, negXLocal, this->tileLength);
        //if(0 == GetBlockIdx()&& progress == 0)
        //    expNegXLocal.Print(16);

        Adds(minExpXLocal, expNegXLocal, one, this->tileLength); 
        //if(0 == GetBlockIdx()&& progress == 0)
        //    minExpXLocal.Print(16);
 
        for(int32_t i = 0; i<this->tileLength ; i++) {
            oneTensor.SetValue(i, 1);
        }

        Div(yLocal, oneTensor, minExpXLocal, this->tileLength);
        
        //if(0 == GetBlockIdx()&& progress == 0)
        //    yLocal.Print(16);

        outQueueY.EnQue<half>(yLocal);
        inQueueX.FreeTensor(xLocal);

    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        //考生补充算子代码
        LocalTensor<DTYPE_Y> yLocal = outQueueY.DeQue<DTYPE_Y>();
        DataCopy(yGm[progress * this->tileLength], yLocal, this->tileLength);
        outQueueY.FreeTensor(yLocal);
    }

private:
    TPipe pipe;
    //create queue for input, in this case depth is equal to buffer num
    TQue<QuePosition::VECIN, BUFFER_NUM> inQueueX;
    //create queue for output, in this case depth is equal to buffer num
    TQue<QuePosition::VECOUT, BUFFER_NUM> outQueueY;
    
    TBuf<TPosition::VECCALC>Tbuf1, Tbuf2, Tbuf3, Tbuf4;

    GlobalTensor<DTYPE_X> xGm;
    GlobalTensor<DTYPE_Y> yGm;

    //考生补充自定义成员变量
    uint32_t blockLength;
    uint32_t tileNum;
    uint32_t tileLength;


};
extern "C" __global__ __aicore__ void sigmoid_custom(GM_ADDR x, GM_ADDR y, GM_ADDR workspace, GM_ADDR tiling) {
    GET_TILING_DATA(tiling_data, tiling);
    KernelSigmoid op;
    //补充init和process函数调用内容
    op.Init(x, y, tiling_data.totalLength, tiling_data.tileNum);
    op.Process();
}

#ifndef ASCENDC_CPU_DEBUG
// call of kernel function
void Sidmoid_custom_do(uint32_t blockDim, void *l2ctrl, void *stream, uint8_t *x, uint8_t *y,
                   uint8_t *workspace, uint8_t *tiling)
{
    sigmoid_custom<<<blockDim, l2ctrl, stream>>>(x, y, workspace, tiling);
}
#endif
 
