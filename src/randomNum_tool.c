#include "randomNum_tool.h"

void generateWeights(uint8_t* weights);

void generate_input_uniformity(uint8_t* data) // 生成均匀测试数据
{
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = (rand() % 256);  // 生成0到255的均匀分布数据
    }
}

void generate_input_Nonuniformity(uint8_t* data) // 生成非均匀测试数据
{
    // 生成非均匀权值
    uint8_t weights[WEIGHT_SIZE];
    generateWeights(weights);

    // 计算总权值
    int totalWeight = 0;
    for (int i = 0; i < WEIGHT_SIZE; i++) {
        totalWeight += weights[i];
    }

    // 生成数据
    int randValue = 0;
    int sum = 0;
    for (int i = 0; i < DATA_SIZE; i++) {
        randValue = rand() % totalWeight;
        sum = 0;
        // 找到随机数落入的区间
        for (int j = 0; j < WEIGHT_SIZE; j++) {
            sum += weights[j];
            if (sum > randValue) {
                data[i] = j;  // 设置data[i]为对应的区间索引
                break;  // 找到后直接跳出
            }
        }
    }
}

void generateWeights(uint8_t* weights) // 生成初始非均匀权重
{
    // 生成1到10之间的随机权重
    for (int i = 0; i < WEIGHT_SIZE; i++) {
        weights[i] = rand() % 10 + 1;
    }
    // 增加部分权重
    for (int i = 0; i < WEIGHT_SIZE; i++) {
        if (rand() % 100 < 10) {  // 10%的概率给出较大的权重
            weights[i] = rand() % 10 + 30;
        }
    }
}