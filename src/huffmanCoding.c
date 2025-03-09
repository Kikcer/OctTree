# include "octoMapSerializer_srtp.h"
# include "randomNum_tool.h"
# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <time.h>
# include <windows.h>
# define NOTE 65535 // 即相当于欧学长代码中的0xffff
# define INF 0x3f3f3f3f

// 字符编码记录表
typedef struct
{
    uint8_t code[MAX_DICT_SIZE][MAX_DICT_SIZE]; // 每个字符的编码
    uint8_t length[MAX_DICT_SIZE];             // 每个字符编码的长度
} CodeTable;

// 初始化哈夫曼树
void initHuffmanTree(HuffmanTree* tree)
{
    tree->size = 0;
    tree->free = 0;
    tree->root = NOTE;
    // 将指定的哈夫曼树的节点置空
    memset(tree->nodes, 0, sizeof(tree->nodes));
}

// 寻找到当前出现次数times最小的两个索引位置
void findTwoSmallest(HuffmanTree* tree, int* min, int* next)
{
    *min = NOTE;
    *next = NOTE;
    for (int i = 0; i < tree->size; i++)
    {
        // 若节点i对应的times为0，则节点已合并
        if (tree->nodes[i].times == 0) continue;
        // *min未初始化或*min已初始化且i索引对应times小于*min索引对应times
        if (*min == NOTE || (*min != NOTE && tree->nodes[*min].times > tree->nodes[i].times))
        {
            *next = *min;
            *min = i;
        }
        // *next未初始化或nodes[i].times大于*min但小于*next
        else if (*next == NOTE || (*next != NOTE && tree->nodes[*next].times > tree->nodes[i].times))
        {
            *next = i;
        }
    }
}

// 构建哈夫曼树
void buildHuffmanTree(dict_t* oldDict, HuffmanTree* tree)
{
    // 将旧字典中的数据迁移到HuffmanTree的nodes[]中
    for (int i = 0; i < MAX_DICT_SIZE; i++)
    {
        if(oldDict->times[i]!=NOTE) // 旧字典对应索引经初始化
        {
            tree->nodes[tree->free].value = oldDict->value[i];
            tree->nodes[tree->free].times = oldDict->times[i];
            tree->nodes[tree->free].left = NOTE;
            tree->nodes[tree->free].right = NOTE;
            tree->free++; // free++ -> 地址顺序分配
            tree->size++;
        }
    }
    // for(int i=0;i<tree->size;i++)
    // {
    //     printf("字符：%d, 统计：%d",tree->nodes[i].value,tree->nodes[i].times);
    //     printf("\n");
    // }
    printf("\n");
    // 原始字典大小--ori_num
    int ori_num = tree->size;
    while (tree->size < 2 * ori_num - 1)
    {
        int min=0, next=0; 
        findTwoSmallest(tree, &min, &next);
        // 合并节点的创建
        tree->nodes[tree->free].value = NOTE;
        tree->nodes[tree->free].times = tree->nodes[min].times + tree->nodes[next].times;
        tree->nodes[tree->free].left = min;
        tree->nodes[tree->free].right = next;
        tree->free++;
        tree->size++;
        // 标记最小节点为已合并(times=0)
        tree->nodes[min].times = 0;
        tree->nodes[next].times = 0;
    }
    // 根节点为当前最后一个有效节点
    tree->root = tree->size - 1;
}

// 递归生成哈夫曼编码
void generateCodes(HuffmanTree* tree, CodeTable* table, uint16_t node, uint8_t* code, uint8_t length)
{
    if (tree->nodes[node].left == NOTE && tree->nodes[node].right == NOTE)
    {
        // 叶子节点：保存编码
        int index = tree->nodes[node].value;
        table->length[index] = length;
        memcpy(table->code[index], code, length);
        return;
    }
    // 左子树，添加 '0'
    if (tree->nodes[node].left != NOTE)
    {
        code[length] = 0;
        generateCodes(tree, table, tree->nodes[node].left, code, length + 1);
    }
    // 右子树，添加 '1'
    if (tree->nodes[node].right != NOTE)
    {
        code[length] = 1;
        generateCodes(tree, table, tree->nodes[node].right, code, length + 1);
    }
}

// 工具:初始化字典
void initDict(dict_t *dict)
{
    dict->size = 0; // 字典大小为0
    for(int i = 0; i < MAX_DICT_SIZE; i++)
    {
        dict->value[i] = 0;
        dict->times[i] = NOTE;
    }
}

// 工具:利用data来构建字典
void fillDictFromData(uint8_t* data, int dataLength, dict_t* oldDict)
{
    initDict(oldDict); // 初始化字典
    for(int i=0;i<dataLength;i++)
    {
        uint8_t cur_val=data[i];
        if(oldDict->times[cur_val]==NOTE) // 若索引i对应cur_val的times未经初始化
        {
            oldDict->value[cur_val]=cur_val;
            oldDict->times[cur_val]=1;
            oldDict->size++;
        }
        else // 索引i对应的times已经被初始化
        {
            oldDict->times[cur_val]++; // 统计次数++
        }
    }
}

// 哈夫曼树报文编码
void huffmanEncode(uint8_t* data, int dataLength, uint8_t* result, HuffmanTree* newDict, int* resultBitSize)
/** 
 * data 为待传输数据;
 * resultBitSize 用于返回 result 报文的长度;
 * oldDict--dict_t类型(先进行数据的统计) ; newDict--HuffmanTree类型;
 * 由于若将newDict为dict，将导致数组的大量空白，空间利用率较低！
*/
{
    // 从data中统计出oldDict:对每一段数据，均进行哈夫曼的编码！
    dict_t oldDict;
    fillDictFromData(data,dataLength,&oldDict);
    // ****************************************************
    CodeTable table = { 0 };
    uint8_t tempCode[MAX_DICT_SIZE] = { 0 };
    // 构建哈夫曼树
    buildHuffmanTree(&oldDict, newDict);
    // 生成哈夫曼编码表
    generateCodes(newDict, &table, newDict->root, tempCode, 0);
    // 压缩数据
    int bitPos = 0; // 记录压缩结果中已使用的位数
    memset(result, 0, DATA_SIZE); // 清空结果数组
    for (int i = 0; i < dataLength; i++) // 遍历输入数据
    {
        int index = data[i]; // 获取字典索引
        if (index > MAX_DICT_SIZE) continue;
        // printf("字符 '%d' 的编码是: ", data[i]); // 输出当前字符
        for (int j = 0; j < table.length[index]; j++) // 遍历当前符号的编码
        {
            if (table.code[index][j] == 1)
                result[bitPos / 8] |= (1 << (7 - (bitPos % 8))); // 设置结果中的相应位
            // 输出编码位（每个符号的编码）
            // printf("%d", table.code[index][j]); 
            bitPos++;
        }
        // printf("\n"); // 换行
    }
    *resultBitSize = bitPos; // 返回总位数（非字节数）
}

// 哈夫曼解压缩
int huffmanDecode(uint8_t* data, int dataLength, HuffmanTree* tree, uint8_t* result, int maxOutputSize)
{
    int outputIndex = 0; // 输出数据的索引
    uint16_t bitPos = 0;      // 当前处理的位位置
    uint16_t node = tree->root; // 从哈夫曼树的根节点开始

    // 逐位读取压缩数据
    while (bitPos < dataLength) {
        // 读取当前位
        uint8_t bit = (data[bitPos / 8] >> (7 - (bitPos % 8))) & 1;
        bitPos++;

        // 根据位值遍历哈夫曼树
        if (bit == 0) {
            node = tree->nodes[node].left; // 左子树
        }
        else {
            node = tree->nodes[node].right; // 右子树
        }

        // 如果是叶子节点，输出对应的字符
        if (tree->nodes[node].left == NOTE && tree->nodes[node].right == NOTE) {
            if (outputIndex >= maxOutputSize) {
                return outputIndex; // 输出缓冲区不足
            }
            result[outputIndex++] = tree->nodes[node].value; // 存储解码结果
            node = tree->root; // 回到根节点，继续解码下一个字符
        }
    }
    return outputIndex; // 返回解码后的数据长度
}

// 将原始数据与编码数据写入文件
void writeIntoData(uint8_t* data, uint8_t* result, int result_length, uint8_t* decodedData, int decodedLength)
{
    // 打开文件
    FILE *outputFile = fopen("huffman_result.txt", "w");
    if (outputFile == NULL) 
    {
        printf("Error opening file!\n");
        return ;
    }
    // 写入原始数据
    fprintf(outputFile, "Original Data: ");
    for (int i = 0; i < DATA_SIZE; i++) 
    {
        fprintf(outputFile, "%d ", data[i]);
    }
    fprintf(outputFile, "\n");
    // 写入哈夫曼编码结果
    fprintf(outputFile, "Huffman Encoded Result (in bits): ");
    for (int i = 0; i < result_length; i++) 
    {
        fprintf(outputFile, "%d", (result[i / 8] >> (7 - (i % 8))) & 1);// 逐位输出编码结果
    }
    fprintf(outputFile, "\n");
    // 输出解码结果
    fprintf(outputFile,"Decoded Data: ");
    for (int i = 0; i < decodedLength; i++) 
    {
        fprintf(outputFile,"%d ", decodedData[i]); // 以字符形式输出(修改为以%d形式输出)
    }
    printf("\n");
    // 关闭文件
    fclose(outputFile);
}

int main()
{
// *******随机生成测试数据*******
    uint8_t data[DATA_SIZE];
    int dataLength=DATA_SIZE;
    srand(time(NULL));  // 初始化随机数种子

// *******压缩率测试*******
    FILETIME start, end;
    GetSystemTimeAsFileTime(&start);
    generate_input_Nonuniformity(data); 
    uint8_t result[DATA_SIZE] = { 0 }; // 存储压缩结果(由于可能导致数组越界，result大小修改为DATA_SIZE)
    HuffmanTree newDict;
    initHuffmanTree(&newDict);
    int result_length = 0;
    huffmanEncode(data,dataLength,result, &newDict,&result_length);
    GetSystemTimeAsFileTime(&end);
    // 将FILETIME转换为64位整数（单位：100纳秒）
    ULARGE_INTEGER start_encode, end_encode;
    start_encode.LowPart = start.dwLowDateTime;
    start_encode.HighPart = start.dwHighDateTime;
    end_encode.LowPart = end.dwLowDateTime;
    end_encode.HighPart = end.dwHighDateTime;
    // 输出编码位数
    double time_spent = (end_encode.QuadPart - start_encode.QuadPart) / 1e4;
    printf("压缩时间：%.6f毫秒\n",time_spent);
    printf("Total bits used: %d\n", result_length);
    printf("Huffman coding compression ratio: %f\n",(double)result_length/(DATA_SIZE*8));

// *******解压缩部分*******
    FILETIME start_, end_;
    GetSystemTimeAsFileTime(&start_);
    uint8_t decodedData[DATA_SIZE] = { 0 }; // 存储解码结果
    int decodedLength = huffmanDecode(result, result_length, &newDict, decodedData, DATA_SIZE);
    GetSystemTimeAsFileTime(&end_);

    // 解码数据与原始数据对比
    ULARGE_INTEGER start_decode, end_decode;
    start_decode.LowPart = start_.dwLowDateTime;
    start_decode.HighPart = start_.dwHighDateTime;
    end_decode.LowPart = end_.dwLowDateTime;
    end_decode.HighPart = end_.dwHighDateTime;
    // 输出编码位数
    time_spent = (end_decode.QuadPart - start_decode.QuadPart) / 1e4;
    printf("解压缩时间：%.6f 毫秒\n",time_spent);
    if (memcmp(data, decodedData, dataLength) == 0) 
    {
        printf("Decoding successful!\n");
    }
    else 
    {
        printf("Decoding failed!\n");
    }
    writeIntoData(data,result,result_length,decodedData,decodedLength);
    return 0;
}