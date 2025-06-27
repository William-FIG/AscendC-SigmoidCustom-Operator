#include "kernel_operator.h"
#include "sigmoid_custom_tiling.h"
using namespace AscendC;
constexpr int32_t BUFFER_NUM = 2;
class KernelSigmoid {
public:
    __aicore__ inline KernelSigmoid() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, size_t totalLength, size_t tileNum)
    {
        //考生补充初始化代码
        //ASSERT(GetBlockNum()!=0 && "block dim can not be zero!");
        this->blockLength = totalLength/GetBlockNum();
        this->tileNum = tileNum;
        //ASSERT(tileNum!=0 && "tile num can not be zero!");
        //this-> tileLength = this-> blockLength/this->tileNum/BUFFER_NUM;
        this->tileLength = this->blockLength / BUFFER_NUM / this->tileNum;

        xGm.SetGlobalBuffer((__gm__ half*)x + this->blockLength * GetBlockIdx(), this->blockLength);
        yGm.SetGlobalBuffer((__gm__ half*)y + this->blockLength * GetBlockIdx(), this->blockLength);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(half));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf1, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf2, this->tileLength * sizeof(half));
        pipe.InitBuffer(Tbuf3, this->tileLength * sizeof(half));

    }
    __aicore__ inline void Process()
    {
        //考生补充对“loopCount”的定义，注意对Tiling的处理
        const int32_t loopCount = this -> tileNum * BUFFER_NUM;
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
        LocalTensor<half> xLocal = inQueueX.AllocTensor<half>();
        DataCopy(xLocal, xGm[progress* this->tileLength], this->tileLength);

        inQueueX.EnQue<half>(xLocal);
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        //考生补充算子计算代码
        LocalTensor<half> xLocal = inQueueX.DeQue<half>();
        LocalTensor<half> negXLocal = Tbuf1.Get<half>();
        LocalTensor<half> expNegXLocal = Tbuf2.Get<half>();
        LocalTensor<half> minExpXLocal = Tbuf3.Get<half>();
        LocalTensor<half> yLocal = outQueueY.AllocTensor<half>();
        
        half minus = -1;
        half posi = 1;

        Muls(negXLocal, xLocal, minus, this->tileLength); 
        Exp(expNegXLocal, negXLocal, this->tileLength);
        Adds(minExpXLocal, expNegXLocal, posi, this->tileLength); // exp(-x) + 1
        Reciprocal(yLocal, expNegXLocal, this->tileLength);//更改前的版本

        outQueueY.EnQue(yLocal);
        inQueueX.FreeTensor(xLocal);

    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        //考生补充算子代码
        LocalTensor<half> yLocal = outQueueY.DeQue<half>();
        DataCopy(yGm[progress * this->tileLength], yLocal, this->tileLength);
        outQueueY.FreeTensor(yLocal);
    }

private:
    TPipe pipe;
    //create queue for input, in this case depth is equal to buffer num
    TQue<QuePosition::VECIN, BUFFER_NUM> inQueueX;
    //create queue for output, in this case depth is equal to buffer num
    TQue<QuePosition::VECOUT, BUFFER_NUM> outQueueY;
    
    TBuf<TPosition::VECCALC>Tbuf1, Tbuf2, Tbuf3;

    GlobalTensor<half> xGm;
    GlobalTensor<half> yGm;

    //考生补充自定义成员变量
    uint32_t blockLength;
    uint32_t tileNum;
    uint32_t tileLength;


};
extern "C" __global__ __aicore__ void sigmoid_custom(GM_ADDR x, GM_ADDR y,SigmoidCustomTilingData tiling) {
    //GET_TILING_DATA(tiling_data, tiling);
    KernelSigmoid op;
    //补充init和process函数调用内容
    op.Init(x, y, tiling.totalLength, tiling.tileNum);
    op.Process();
}

/**
 * #ifndef ASCENDC_CPU_DEBUG
// call of kernel function
void Sidmoid_custom_do(uint32_t blockDim, void *l2ctrl, void *stream, uint8_t *x, uint8_t *y,
                   uint8_t *workspace, uint8_t *tiling)
{
    sigmoid_custom_do<<<blockDim, l2ctrl, stream>>>(x, y, workspace, tiling);
}
#endif
 */
