/*********************************************************************************************
* 文件：main.c
* 作者：Lixm 2017.09.25
* 说明：光照度传感器实验 例程    
*       通过光强传感器采集环境光照强度，并将光强信息打印在PC上，1S更新一次
* 修改：zhoucj  2018.01.02  修改为串口一
* 注释：
*********************************************************************************************/

/*********************************************************************************************
* 头文件
*********************************************************************************************/
#include <ioCC2530.h>
#include "clock.h"
#include "delay.h"
#include "uart1.h"
#include "stdio.h"
#include "string.h"
#include "BH1750.h"

// 定义判决数组，1110为开始检测信号，1100为结束信号
int startSignal[4] = {1, 1, 1, 0};
int endSignal[4] = {1, 1, 0, 0};

// 用于将4位二进制转换回十进制数字
int binaryToDecimal[10][4] = {
    {1, 0, 1, 0}, // 0
    {0, 0, 0, 1}, // 1
    {0, 0, 1, 0}, // 2
    {0, 0, 1, 1}, // 3
    {0, 1, 0, 0}, // 4
    {0, 1, 0, 1}, // 5
    {0, 1, 1, 0}, // 6
    {0, 1, 1, 1}, // 7
    {1, 0, 0, 0}, // 8
    {1, 0, 0, 1}  // 9
};

void processBitStream(int bitStream[], int *bitStreamIndex, int binaryBits[], int *binaryBitCount, int *startFlag, int *endFlag)
{

    // 检测比特流是否发生跳变
    if (*bitStreamIndex > 1 && bitStream[*bitStreamIndex - 1] != bitStream[*bitStreamIndex - 2])
    {
        uart1_send_string("判决比特流中发生1与0跳变\n");
        if (*bitStreamIndex == 4)
        {
            uart1_send_string("已接收到四位判决比特，赋值到binaryBits\n");            
            binaryBits[*binaryBitCount]=bitStream[*bitStreamIndex - 2];
            (*binaryBitCount)++;          
            bitStream[0] = bitStream[*bitStreamIndex - 1];
            *bitStreamIndex = 1;
        }
        else
        {
            bitStream[0] = bitStream[*bitStreamIndex - 1];
            *bitStreamIndex = 1;
        }
    }
    else if (*bitStreamIndex == 4)
    {
        uart1_send_string("判决比特流中没有发生跳变\n");
        uart1_send_string("已接收到四位判决比特，赋值到binaryBits\n");
        binaryBits[*binaryBitCount]=bitStream[*bitStreamIndex - 1];
        (*binaryBitCount)++;
        *bitStreamIndex = 0;
    }

    if (*binaryBitCount == 4)
    {
        uart1_send_string("四位二进制输出: ");
        *binaryBitCount = 0; 
        for (int i = 0; i < 4; ++i)
        {
            char tx_buff[2] = {0};
            sprintf(tx_buff, "%d", binaryBits[i]);
            uart1_send_string(tx_buff);
        }
        uart1_send_string("\n");

        if (!(*startFlag))
        {
            int flag = 1;
            for (int j = 0; j < 4; ++j)
            {
                if (binaryBits[j] != startSignal[j])
                {
                    flag = 0;                    
                    uart1_send_string("不为开始信号，binaryBitCount置3，后三位左移\n");
                    *binaryBitCount = 3;
                    for (int i = 0; i < 3; i++)
                    {
                        binaryBits[i] = binaryBits[i + 1];
                    }
                    binaryBits[3] = 0;
                    break;
                }
            }
            if (flag)
            {
                uart1_send_string("开始检测接收：\n");
                *startFlag = 1;
                return;
            }
        }

        if (!(*endFlag))
        {
            int flag = 1;
            for (int j = 0; j < 4; ++j)
            {
                if (binaryBits[j] != endSignal[j])
                {
                    flag = 0;
                    break;
                }
            }
            if (flag)
            {
                uart1_send_string("结束检测接收\n");
                *endFlag = 1;
                return;
            }
        }

        for (int i = 0; i < 10; ++i)
        {
            int flag = 1;
            for (int j = 0; j < 4; ++j)
            {
                if (binaryBits[j] != binaryToDecimal[i][j])
                {
                    flag = 0;
                    break;
                }
            }
            if (flag == 1 && *startFlag == 1)
            {
                char tx_buff[32] = {0};
                sprintf(tx_buff, "接收到十进制数字：%d\n", i);
                uart1_send_string(tx_buff);
            }
        }
    }
}

/*********************************************************************************************
* 名称：main()
* 功能：逻辑代码
* 参数：无
* 返回：无
* 修改：
* 注释：
*********************************************************************************************/
void main(void)
{
    float lightData = 0; // 存储光照度数据变量
    char txBuffer[64];   // 串口发送缓冲数组
    memset(txBuffer, 0, sizeof(txBuffer));
    int binaryBits[4] = {0, 0, 0, 0}; // 四位二进制数组
    int bitStream[4];                 // 判决比特位数组
    xtal_init();                      // 系统时钟初始化     
    bh1750_init();                    // 光照度传感器初始化
    uart1_init(0x00, 0x00); 
    int bitStreamIndex = 0;           // 判决比特流下标                                              
    int binaryBitCount = 0;           // 四位二进制位数
    int startFlag = 0, endFlag = 0;   // 开始，结束标识符

    
    while (1)
    {
        lightData = bh1750_get_data();
        sprintf(txBuffer, "接收检测光强Lx:%f\n", lightData); // 输出检测光强
        uart1_send_string(txBuffer);

        if (lightData > 75) // 检测到发亮
        {
            uart1_send_string("发送端发送: 1\n"); // 输出‘1’
            bitStream[bitStreamIndex++] = 1;
        }
        else
        {
            uart1_send_string("发送端发送: 0\n"); // 输出0
            bitStream[bitStreamIndex++] = 0;
        }

        processBitStream(bitStream, &bitStreamIndex, binaryBits, &binaryBitCount, &startFlag, &endFlag);
        
        delay_ms(200);
        if (endFlag) // 如果检测到结束信号，则重置标识符
        {
            startFlag = 0, endFlag = 0;
            continue;
        }
    }


    return;
}
