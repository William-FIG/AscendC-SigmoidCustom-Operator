/**
 * @file sinh_custom.cpp
 *
 * Copyright (C) 2024. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include "kernel_operator.h"
#include "sinh_custom_tiling.h"
using namespace AscendC;
constexpr int32_t BUFFER_NUM = 2;

class KernelSinh {
public:
    __aicore__ inline KernelSinh() {}
    __aicore__ inline void Init(GM_ADDR x, GM_ADDR y, size_t totalLength, size_t tileNum)
    {
        //考生补充初始化代码
        this->blockLength = totalLength / GetBlockNum();
        this->tileNum = tileNum;
        this->tileLength = this->blockLength / BUFFER_NUM / this->tileNum;
        xGm.SetGlobalBuffer((__gm__ half *) x + GetBlockIdx() * this->blockLength, this->blockLength);
        yGm.SetGlobalBuffer((__gm__ half *) y + GetBlockIdx() * this->blockLength, this->blockLength);
        pipe.InitBuffer(inQueueX, BUFFER_NUM, this->tileLength * sizeof(half));
        pipe.InitBuffer(outQueueY, BUFFER_NUM, this->tileLength * sizeof(half));
        pipe.InitBuffer(TBuf1,this->tileLength * sizeof(half));
        pipe.InitBuffer(TBuf2,this->tileLength * sizeof(half));
        pipe.InitBuffer(TBuf3,this->tileLength * sizeof(half));
        pipe.InitBuffer(TBuf4,this->tileLength * sizeof(half));
            }
    __aicore__ inline void Process()
    {
        //考生补充对“loopCount”的定义，注意对Tiling的处理
        uint32_t loopCount = this->tileNum * BUFFER_NUM;
        for (int32_t i = 0; i < loopCount; i++) {
            CopyIn(i);
            Compute(i);
            CopyOut(i);
        }
    }

private:
    __aicore__ inline void CopyIn(int32_t progress)
    {
        LocalTensor<half> xLocal = inQueueX.AllocTensor<half>();
        // LocalTensor<half> yLocal = outQueueY.AllocTensor<half>();
        DataCopy(xLocal, xGm[this->tileLength * progress], this->tileLength);
        inQueueX.EnQue<half>(xLocal);

        //考生补充算子代码
    }
    __aicore__ inline void Compute(int32_t progress)
    {
        //考生补充算子计算代码
        LocalTensor<half> xLocal = inQueueX.DeQue<half>();
        LocalTensor<half> temp1 = TBuf1.Get<half>();
        LocalTensor<half> temp2 = TBuf2.Get<half>();
        LocalTensor<half> temp3 = TBuf3.Get<half>();
        LocalTensor<half> temp4 = TBuf4.Get<half>();
        LocalTensor<half> yLocal = outQueueY.AllocTensor<half>();
        //sinh(x) = (exp(x) - exp(-x)) / 2.0
        half mul = -1;
        half sub = 0.5;
        // if(progress == 0 && GetBlockIdx() == 0){
        //     xLocal.Print();
        //     Exp(temp1, xLocal, this->tileLength);
        //     temp1.Print();
        //     Muls(temp2, xLocal, mul, this->tileLength);
        //     temp2.Print();
        //     Exp(temp3, temp2, this->tileLength);
        //     temp3.Print();
        //     Sub(temp4, temp1, temp3, this->tileLength);
        //     temp4.Print();
        //     Muls(yLocal, temp4, sub, this->tileLength);
        //     yLocal.Print();
        // }
        Exp(temp1, xLocal, this->tileLength);

        Muls(temp2, xLocal, mul, this->tileLength);

        Exp(temp3, temp2, this->tileLength);

        Sub(temp4, temp1, temp3, this->tileLength);

        Muls(yLocal, temp4, sub, this->tileLength);

        outQueueY.EnQue<half>(yLocal);
        inQueueX.FreeTensor(xLocal);
        // outQueueY.FreeTensor(yLocal);
    }
    __aicore__ inline void CopyOut(int32_t progress)
    {
        //考生补充算子代码
        LocalTensor<half> yLocal = outQueueY.DeQue<half>();
        DataCopy(yGm[this->tileLength * progress], yLocal, this->tileLength);
        outQueueY.FreeTensor(yLocal);
    }

private:
    TPipe pipe;
    //create queue for input, in this case depth is equal to buffer num
    TQue<QuePosition::VECIN, BUFFER_NUM> inQueueX;
    //create queue for output, in this case depth is equal to buffer num
    TQue<QuePosition::VECOUT, BUFFER_NUM> outQueueY;
    TBuf<TPosition::VECCALC> TBuf1, TBuf2, TBuf3, TBuf4; 
    GlobalTensor<half> xGm;
    GlobalTensor<half> yGm;
    uint32_t tileNum, tileLength, blockLength;
    //考生补充自定义成员变量

};

extern "C" __global__ __aicore__ void sinh_custom(GM_ADDR x, GM_ADDR y, SinhCustomTilingData tiling) {
    // GET_TILING_DATA(tiling_data, tiling);
    KernelSinh op;
    //补充init和process函数调用内容
    op.Init(x, y, tiling.totalLength, tiling.tileNum);
    op.Process();
}
