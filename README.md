**提交Tips** 建议先clone当前库，手动处理冲突再pull/push!  

**哈夫曼部分说明**  2025/3/4  
  1、interface/octoMapSerializer_srtp.h为哈夫曼部分的头文件，咱们要尽快合并头文件函数与结构体定义，否则提交存在冲突；  
  2、src/huffmanCoding.c为哈夫曼建树文件，按今天讨论的理解，输入将uint8_t的一个数组，并输出二进制的哈夫曼编码，huffmanCoding.c文件中包含以下测试用例：  
  <img src="https://github.com/user-attachments/assets/7673ddc6-a56c-4833-8fcb-a74eb80771d0" width="400" />
