**哈夫曼部分说明**  2025/3/4
  1、interface/octoMapSerializer_srtp.h为哈夫曼部分的头文件，咱们要尽快合并头文件函数与结构体定义，否则提交存在冲突；
  2、src/huffmanCoding.c为哈夫曼建树文件，按今天讨论的理解，输入将uint8_t的一个数组，并输出二进制的哈夫曼编码，huffmanCoding.c文件中包含以下测试用例：
  ![image](https://github.com/user-attachments/assets/4ca905a3-1df5-4545-97fd-08187f155ecc)
  
  **提交Tips** 建议先clone当前库，手动处理冲突再pull/push!
