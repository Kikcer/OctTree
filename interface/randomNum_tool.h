# ifndef RANDOMNUM_TOOL_H
# define RANDOMNUM_TOOL_H

/**
 * Huffman/LZW编码测试工具箱
 */
# include <stdint.h>
# include <stdbool.h>
# include <stdlib.h>
# include <time.h>
# include <math.h>
# define DATA_SIZE 10000
# define WEIGHT_SIZE 256

void generate_input_uniformity(uint8_t* data); // 生成均匀测试数据
void generate_input_Nonuniformity(uint8_t* data); // 生成非均匀测试数据

#endif