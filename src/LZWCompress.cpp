# include "octoMapSerializer_srtp.h"
# include "randomNum_tool.h"
#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstdio>

using namespace std;

//每8位视为一个字符整体处理
#define MAX_DICT_SIZE 4096//最大字典容量，压缩码为log2(MAX_DICT_SIZE)=12位
//#define MAX_WORD_SIZE 6//最大单词长度（取消了这条限制）

#define ROOT 0
#define NOTE -1//not exist
// const int DATA_SIZE = 200000;//可调整的输入字节数
const int MAX_DATA_SIZE = 300000; //预留的输出空间

struct dict_node {
	uint16_t code;//12位压缩码，同时为数组索引，0保留
	uint8_t value;//当前单词的结尾字符
	short pre;
	short next_brother;
	short first_child;
	short last_child;
	short depth;
	dict_node(): pre(NOTE), next_brother(NOTE), first_child(NOTE), last_child(NOTE), depth(0) {}
};
dict_node dict[MAX_DICT_SIZE];
int dict_cnt = 1;//dict[ROOT]为空

uint8_t input_data[DATA_SIZE];
uint8_t output_data[MAX_DATA_SIZE];
int output_cnt = 0;
int code_cnt = 0;



//========================================缓冲输出部分========================================

uint8_t buffer_byte;
int buffer_cnt = 0; //0或4

void write_code8(uint8_t code) {
	if (buffer_cnt == 0) {
		output_data[output_cnt++] = code;
	} else {
		buffer_byte = buffer_byte | (code >> 4);
		output_data[output_cnt++] = buffer_byte;
		buffer_byte = code << 4;
		buffer_cnt = 4;
	}
}

void write_code12(uint16_t code) {
	code &= 0x0FFF;
	if (buffer_cnt == 0) {
		output_data[output_cnt++] = code >> 4;
		buffer_byte = code << 4;
		buffer_cnt = 4;
	} else {
		buffer_byte |= code >> 8;
		output_data[output_cnt++] = buffer_byte;
		output_data[output_cnt++] = code;
		buffer_cnt = 0;
	}
}

void serialize_dict() {
	for (int i = 1; i < dict_cnt; i++) {
		write_code8(dict[i].value);
		write_code12(dict[i].pre);
	}
	if (buffer_cnt != 0) {
		output_data[output_cnt++] = buffer_byte;
		buffer_cnt = 0;
	}
}



//========================================LZW压缩部分========================================

int InDictionary(int pre_node, uint8_t curr_char) { //返回当前单词（由pre_node和curr_char给出）在字典树中的位置
	int curr_child = dict[pre_node].first_child;
	while (curr_child != NOTE) {
		if (dict[curr_child].value == curr_char) return curr_child;
		curr_child = dict[curr_child].next_brother;
	}
	return NOTE;
}

bool AddToDictionary(int pre_node, uint8_t curr_char) {
//	if (dict[pre_node].depth + 1 > MAX_WORD_SIZE) return false;
	if (dict_cnt >= MAX_DICT_SIZE) return false;

	dict[dict_cnt].value = curr_char;
	dict[dict_cnt].code = dict_cnt;
	dict[dict_cnt].pre = pre_node;
	dict[dict_cnt].depth = dict[pre_node].depth + 1;

	if (dict[pre_node].first_child != NOTE) {
		dict[dict[pre_node].last_child].next_brother = dict_cnt;
		dict[pre_node].last_child = dict_cnt;
	} else {
		dict[pre_node].first_child = dict_cnt;
		dict[pre_node].last_child = dict_cnt;
	}
	dict_cnt++;
	return true;
}

void LZW_compress() {
	for (int c = 0; c < 256; c++) {
		AddToDictionary(ROOT, c);//初始化（0-255）
	}
	int pre_node = ROOT;
	for (int i = 0; i < DATA_SIZE; i++) {
		uint8_t c = input_data[i];
		int curr_node = InDictionary(pre_node, c);
		if (curr_node != NOTE) {//若当前单词在字典树中，以当前单词作为前缀单词，继续下一次循环
			pre_node = curr_node;
		} else {
			if (pre_node != ROOT) {
				write_code12(dict[pre_node].code);//输出前缀单词
				code_cnt++;
			}
			AddToDictionary(pre_node, c);
			pre_node = InDictionary(ROOT, c);
			if (pre_node == NOTE) {
				pre_node = ROOT;
			}
		}
	}
	if (pre_node != ROOT) {
		write_code12(dict[pre_node].code);
		code_cnt++;
	}
	serialize_dict();
}



//========================================LZW解压缩部分========================================

struct dcpr_dict_node {//重建字典树
	uint8_t value;
	short pre;
};
dcpr_dict_node dcpr_dict[MAX_DICT_SIZE];

uint8_t dcpr_data[MAX_DATA_SIZE];
int dcpr_cnt = 0;

void LZW_decode(uint16_t code) {
	if (dcpr_dict[code].pre != ROOT) {
		LZW_decode(dcpr_dict[code].pre);
	}
	dcpr_data[dcpr_cnt++] = dcpr_dict[code].value;
}

void LZW_decompress() {
	//假设已读入output_data,code_cnt,dict_cnt
	//先读字典部分，重建字典树
	int bit4_cnt = code_cnt * 3; //以半字节为单位计算下标
	for (int id = 1; id < dict_cnt; id++) {//索引即为压缩码
		int pos = bit4_cnt / 2;
		if (bit4_cnt % 2 == 0) {
			dcpr_dict[id].value = output_data[pos];
			dcpr_dict[id].pre = (output_data[pos + 1] << 4) | (output_data[pos + 2] >> 4);

		} else {
			dcpr_dict[id].value = (output_data[pos] << 4) | (output_data[pos + 1] >> 4);
			dcpr_dict[id].pre = ((output_data[pos + 1] & 0x0F) << 8) | output_data[pos + 2];
		}
		bit4_cnt += 5;
	}
	//再读压缩码部分，根据字典树解压缩
	bit4_cnt = 0;
	for (int i = 0; i < code_cnt; i++) {
		int pos = bit4_cnt / 2;
		if (bit4_cnt % 2 == 0) {
			LZW_decode((output_data[pos] << 4) | (output_data[pos + 1] >> 4));
		} else {
			LZW_decode(((output_data[pos] & 0x0F) << 8) | output_data[pos + 1]);
		}
		bit4_cnt += 3;
	}
}



//========================================测试部分========================================

//========================================计时函数========================================

#include <time.h>
#include <sys/time.h>
#define TIMER_START 1
#define TIMER_RETURN 2

struct timeval t_start = {0, 0}, t_end = {0, 0};

float t1 = 0, t2 = 0;

float TIMER(int timer_command) { //毫秒计时器
	if (timer_command == TIMER_START) { //开始计时
		gettimeofday(&t_start, NULL);
		return 0;
	}

	if (timer_command == TIMER_RETURN) { //返回时间(ms)
		gettimeofday(&t_end, NULL);
		int timeuse = 1000000 * (t_end.tv_sec - t_start.tv_sec) +
		              t_end.tv_usec - t_start.tv_usec;
		return timeuse / 1000.0;//ms
	}
	return 0;
}



//========================================测试数据生成========================================
# define WEIGHT_SIZE 256

void generateWeights(uint8_t* weights);

void generate_input_uniformity(uint8_t* data) { // 生成均匀测试数据
	for (int i = 0; i < DATA_SIZE; i++) {
		data[i] = (rand() % 256);  // 生成0到255的均匀分布数据
	}
}

void generate_input_Nonuniformity(uint8_t* data) { // 生成非均匀测试数据
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

void generateWeights(uint8_t* weights) { // 生成初始非均匀权重
	// 生成1到10之间的随机权重
	for (int i = 0; i < WEIGHT_SIZE; i++) {
		weights[i] = rand() % 10 + 1;
	}
	// 增加部分权重
	for (int i = 0; i < WEIGHT_SIZE; i++) {
		if (rand() % 100 < 10) {  // 10%的概率给出较大的权重
			weights[i] = rand() % 10 + 210;//+30/+210，七倍数据偏好
		}
	}
}



void generate_input() {
//	generate_input_uniformity(input_data);
	generate_input_Nonuniformity(input_data);
}

bool test_correct() {
	if (dcpr_cnt != DATA_SIZE) {
		return false;
	}
	for (int i = 0; i < DATA_SIZE; i++) {
		if (dcpr_data[i] != input_data[i]) {
			return false;
		}
	}
	return true;
}

void print_stats() {
	int original_bytes = DATA_SIZE;
	float compressed_bytes = code_cnt * 1.5;
	float dict_bytes = dict_cnt * 2.5;

	cout << "Original size: " << original_bytes << " bytes\n";
	cout << "Compressed size: " << compressed_bytes << " bytes\n";
	cout << "Dictionary size: " << dict_bytes << " bytes\n";
	cout << "Total output size: " << compressed_bytes + dict_bytes << " bytes\n";
	cout << "Compression ratio: " << (float)compressed_bytes / original_bytes << endl;
	cout << "Total ratio: " << (float)(compressed_bytes + dict_bytes) / original_bytes << endl;

	printf("Compression time: %.3f ms\n", t1);
	printf("Decompression time: %.3f ms\n", t2);

	if (test_correct()) {
		cout << "yes!\n";
	} else {
		cout << "no！\n";
	}
}

int main() {
	generate_input();
	TIMER(TIMER_START);
	LZW_compress();
	t1 = TIMER(TIMER_RETURN);
	TIMER(TIMER_START);
	LZW_decompress();
	t2 = TIMER(TIMER_RETURN);
	print_stats();
	return 0;
}