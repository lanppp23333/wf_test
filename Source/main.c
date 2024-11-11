/*********************************************************************************************
* �ļ���main.c
* ���ߣ�Lixm 2017.09.25
* ˵�������նȴ�����ʵ�� ����    
*       ͨ����ǿ�������ɼ���������ǿ�ȣ�������ǿ��Ϣ��ӡ��PC�ϣ�1S����һ��
* �޸ģ�zhoucj  2018.01.02  �޸�Ϊ����һ
* ע�ͣ�
*********************************************************************************************/

/*********************************************************************************************
* ͷ�ļ�
*********************************************************************************************/
#include <ioCC2530.h>
#include "clock.h"
#include "delay.h"
#include "uart1.h"
#include "stdio.h"
#include "string.h"
#include "BH1750.h"

// �����о����飬1110Ϊ��ʼ����źţ�1100Ϊ�����ź�
int startSignal[4] = {1, 1, 1, 0};
int endSignal[4] = {1, 1, 0, 0};
//��ͻ���Test1
// ���ڽ�4λ������ת����ʮ��������
//Test by HZM
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

    // ���������Ƿ�������
    if (*bitStreamIndex > 1 && bitStream[*bitStreamIndex - 1] != bitStream[*bitStreamIndex - 2])
    {
        uart1_send_string("�о��������з���1��0����\n");
        if (*bitStreamIndex == 4)
        {
            uart1_send_string("�ѽ��յ���λ�о����أ���ֵ��binaryBits\n");            
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
        uart1_send_string("�о���������û�з�������\n");
        uart1_send_string("�ѽ��յ���λ�о����أ���ֵ��binaryBits\n");
        binaryBits[*binaryBitCount]=bitStream[*bitStreamIndex - 1];
        (*binaryBitCount)++;
        *bitStreamIndex = 0;
    }

    if (*binaryBitCount == 4)
    {
        uart1_send_string("��λ���������: ");
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
                    uart1_send_string("��Ϊ��ʼ�źţ�binaryBitCount��3������λ����\n");
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
                uart1_send_string("��ʼ�����գ�\n");
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
                uart1_send_string("����������\n");
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
                sprintf(tx_buff, "���յ�ʮ�������֣�%d\n", i);
                uart1_send_string(tx_buff);
            }
        }
    }
}

/*********************************************************************************************
* ���ƣ�main()
* ���ܣ��߼�����
* ��������
* ���أ���
* �޸ģ�
* ע�ͣ�
*********************************************************************************************/
void main(void)
{
    float lightData = 0; // �洢���ն����ݱ���
    char txBuffer[64];   // ���ڷ��ͻ�������
    memset(txBuffer, 0, sizeof(txBuffer));
    int binaryBits[4] = {0, 0, 0, 0}; // ��λ����������
    int bitStream[4];                 // �о�����λ����
    xtal_init();                      // ϵͳʱ�ӳ�ʼ��     
    bh1750_init();                    // ���նȴ�������ʼ��
    uart1_init(0x00, 0x00); 
    int bitStreamIndex = 0;           // �о��������±�                                              
    int binaryBitCount = 0;           // ��λ������λ��
    int startFlag = 0, endFlag = 0;   // ��ʼ��������ʶ��

    
    while (1)
    {
        lightData = bh1750_get_data();
        sprintf(txBuffer, "���ռ���ǿLx:%f\n", lightData); // �������ǿ
        uart1_send_string(txBuffer);

        if (lightData > 75) // ��⵽����
        {
            uart1_send_string("���Ͷ˷���: 1\n"); // �����1��
            bitStream[bitStreamIndex++] = 1;
        }
        else
        {
            uart1_send_string("���Ͷ˷���: 0\n"); // ���0
            bitStream[bitStreamIndex++] = 0;
        }

        processBitStream(bitStream, &bitStreamIndex, binaryBits, &binaryBitCount, &startFlag, &endFlag);
        
        delay_ms(200);
        if (endFlag) // �����⵽�����źţ������ñ�ʶ��
        {
            startFlag = 0, endFlag = 0;
            continue;
        }
    }


    return;
}
