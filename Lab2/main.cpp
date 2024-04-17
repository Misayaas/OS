#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>

using namespace std;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

extern "C" {
void asm_print(const char *);
}

/*
参考文献：https://blog.csdn.net/judyge/article/details/52373751
https://blog.csdn.net/qq_39654127/article/details/88429461#main-toc
*/

//从BPB中取出的信息存入全局变量
int BytsPerSec; //每扇区字节数
int SecPerClus; //每簇扇区数
int RsvdSecCnt; //Boot记录占用的扇区数
int NumFATs; //FAT表个数
int RootEntCnt; //根目录最大文件数
int FATSz; //FAT扇区数

string str_print;

//节点类用于构建文件树的节点
class Node {
public:
    string name;
    string path; //绝对路径
    vector<Node *> nextNode;
    char *content = new char[10000]{'\0'};
    int fileCnt = 0; //下一级文件数量
    int dirCnt = 0; //下一级目录数量
    u32 fileSize;
    bool isFile = false; //判断文件和目录
    bool isVal = true; //判断点和值
};

#pragma pack(1) //方便读取文件

//每个条目占32字节
struct RootEntry {
    char DIR_Name[11]; //文件名 + 扩展名
    u8 DIR_Attr; //文件属性
    u8 reserved[10]; //保留位
    u16 DIR_WrtTime; //最后一次写入时间
    u16 DIR_WrtDate; //最后一次写入日期
    u16 DIR_FstClus; //文件开始的簇号
    u32 DIR_FileSize; //文件大小
};

//BPB占25字节
struct BPB {
    u16 BPB_BytesPerSec; //每扇区字节数
    u8 BPB_SecPerClus; //每簇扇区数
    u16 BPB_ResvdSecCnt; //Boot记录占用多少扇区
    u8 BPB_NumFATs; //共有多少FAT表
    u16 BPB_RootEntCnt; //根目录区文件最大数
    u16 BPB_TotSec16; //扇区总数
    u8 BPB_Media; //介质描述符
    u16 BPB_FATSz16; //每个FAT表所占扇区数
    u16 BPB_SecPerTrk; //每磁道扇区数
    u16 BPB_NumHeads; //磁头数
    u32 BPB_HiddSec; //隐藏扇区数
    u32 BPB_TotSec32; //如果BPB_TotSec16=0,则由这里给出扇区数
};

#pragma pack() //取消特殊对齐

//nasm实现的print函数
void myPrint(const char *str);

//创建文件树
void createFIleTree(FILE *fat12, struct RootEntry *rootEntry_ptr, Node *root);

//用划分符号分割token
void getInputList(string &input, vector<string> &inputList, char flag);

//获取文件内容
void getFileContent(FILE *fat12, int startClus, Node *son);

//创建目录节点,也就是添加"."和".."
void createDirNode(Node *node);

//递归读取子目录
void readChildren(FILE *fat12, int startClus, Node *father);

//获取FAT表中的值,返回下一个簇号
int getFATValue(FILE *fat12, int clus);

//判断文件名是否合法
bool isLegalChar(RootEntry *rootEntry_ptr);

//查找文件名,返回是否找到
bool findFileName(Node *root, string name);

//在路径首尾加上'/'
void addPath(string &s);

//单纯的打印文件树，也就是不带-l的ls
void printList(Node *node);

//打印带-l的文件树
void printList_L(Node *root);

//linux cat
void cat(Node *root, string source, int &mode);

//打印带路径的文件树
void printListWithPath(Node *root, string path, int &mode, bool hasL);


int main(int argc, char *argv[]) {
    FILE *fat12 = fopen("a.img", "rb");
    //初始化根节点
    Node *root = new Node();
    root->name = "";
    root->path = "/";

    struct BPB bpb;
    struct BPB *bpb_ptr = &bpb;

    struct RootEntry rootEntry;
    struct RootEntry *rootEntry_ptr = &rootEntry;

    //读取img并存入bpb
    int seek_flag = fseek(fat12, 11, SEEK_SET);
    int read_flag = fread(bpb_ptr, 1, 25, fat12);
    if (seek_flag != 0 || read_flag != 25) {
        myPrint("载入错误");
    }
    //初始化全局变量
    BytsPerSec = bpb_ptr->BPB_BytesPerSec;
    SecPerClus = bpb_ptr->BPB_SecPerClus;
    RsvdSecCnt = bpb_ptr->BPB_ResvdSecCnt;
    NumFATs = bpb_ptr->BPB_NumFATs;
    RootEntCnt = bpb_ptr->BPB_RootEntCnt;
    if (bpb_ptr->BPB_FATSz16 != 0) {
        FATSz = bpb_ptr->BPB_FATSz16;
    }
        //如果FATSz16为0，则使用FATSz32
    else {
        FATSz = bpb_ptr->BPB_TotSec32;
    }

    //创建文件树
    createFIleTree(fat12, rootEntry_ptr, root);

    while (true) {
        myPrint(">");
        string input;
        vector<string> inputList;
        getline(cin, input);
        //根据空格分割开输入的每个token
        getInputList(input, inputList, ' ');

        if (inputList[0] == "exit") {
            myPrint("退出\n");
            break;
        } else if (inputList[0] == "ls") {
            //单纯的ls
            if (inputList.size() == 1) {
                printList(root);
                //带参数的ls
            } else {
                bool hasL = false;
                bool hasPath = false;
                bool error = false;
                string *path = NULL;
                //从1开始是直接把ls，cat，exit这一部分过滤掉
                for (int i = 1; i < inputList.size(); i++) {
                    string s = inputList[i];
                    //路径
                    if (s[0] != '-') {
                        //多路径报错情况
                        if (hasPath) {
                            myPrint("已有路径\n");
                            error = true;
                            break;
                            //正常单路径
                        } else {
                            hasPath = true;
                            addPath(inputList[i]);
                            vector<string> inputPath;
                            getInputList(inputList[i], inputPath, '/');
                            string newPath = "/";
                            //存储路径上的各个部分，遇到..则pop返回上一级
                            auto *doublePoint = new vector<string>;
                            for (const auto &j: inputPath) {
                                if (j != "..") {
                                    if (!findFileName(root, j)) {
                                        error = true;
                                        myPrint("ls参数错误\n");
                                        break;
                                    }
                                }
                            }
                            //确认路径合法过后的处理
                            for (const auto &j: inputPath) {
                                if (j == "..") {
                                    if (!doublePoint->empty()) {
                                        doublePoint->pop_back();
                                    }
                                } else {
                                    doublePoint->push_back(j);
                                }
                            }

                            if (!error) {
                                if (!doublePoint->empty()) {
                                    for (const auto &j: *doublePoint) {
                                        newPath += j + "/";
                                    }
                                }
                                path = &newPath;
                            }
                        }
                    }
                        //-参数
                    else {
                        if (s.length() == 1) {
                            myPrint("ls参数错误\n");
                            error = true;
                            break;
                        }
                        for (int j = 1; j < s.length(); j++) {
                            if (s[j] != 'l') {
                                myPrint("ls参数错误\n");
                                error = true;
                                break;
                            }
                        }
                        hasL = true;
                    }
                }
                if (error) {
                    continue;
                }

                //ls -l
                int mode = 0; //mode==1有L没path，mode==2没L有path且文件无法打开
                if (hasL && !hasPath) {
                    mode = 1;
                    printList_L(root);
                } else if (!hasL && hasPath) {
                    //ls nju
                    printListWithPath(root, *path, mode, false);
                } else if (hasL && hasPath) {
                    printListWithPath(root, *path, mode, true);
                } else {
                    printList(root);
                    continue;
                }

                if (mode == 0) {
                    myPrint("无法找到目录\n");
                    continue;
                } else if (mode == 2) {
                    myPrint("无法打开文件\n");
                    continue;
                }
            }
        } else if (inputList[0] == "cat") {
            if (inputList.size() == 2 && inputList[1][0] != '-') {
                int mode = 0;
                addPath(inputList[1]);
                cat(root, inputList[1], mode);
                if (mode == 0) {
                    myPrint("无法找到文件\n");
                    continue;
                } else if (mode == 2) {
                    myPrint("无法打开文件\n");
                    continue;
                }
            } else {
                myPrint("cat参数错误\n");
                continue;
            }
        } else {
            myPrint("命令错误\n");
            continue;
        }
    }

    //防止内存泄漏
    fclose(fat12);
    return 0;
}

void myPrint(const char *str) {
    asm_print(str);
}

void createFIleTree(FILE *fat12, struct RootEntry *rootEntry_ptr, Node *root) {
    int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz); //根目录区的起始地址
    //处理每个根目录条目
    for (int i = 0; i < RootEntCnt; i++) {
        int seek_flag = fseek(fat12, base + i * 32, SEEK_SET);
        int read_flag = fread(rootEntry_ptr, 1, 32, fat12);
        if (seek_flag != 0 || read_flag != 32) {
            myPrint("读取错误");
        }

        //空条目直接过
        if (rootEntry_ptr->DIR_Name[0] == '\0') continue;

        if (!isLegalChar(rootEntry_ptr)) {
            //myPrint("文件名不合法");
            continue;
        }

        //处理文件名
        char realName[12] = {' '}; //清空文件名防止未访问到的地方存在缓存
        //0x10表示目录，处理文件
        if ((rootEntry_ptr->DIR_Attr & 0x10) == 0) {
            int index = -1;
            for (int j = 0; j < 11; j++) {
                if (rootEntry_ptr->DIR_Name[j] != ' ') {
                    index++;
                    realName[index] = rootEntry_ptr->DIR_Name[j];
                } else {
                    //文件名和扩展名之间加一个'.'，并且去掉空格
                    index++;
                    if (rootEntry_ptr->DIR_Name[8] != ' ') {
                        realName[index] = '.';
                    }

                    while (rootEntry_ptr->DIR_Name[j] == ' ') {
                        j++;
                    }
                    j--;
                }
            }

            index++;
            //名称结尾
            realName[index] = '\0';

            Node *son = new Node();
            root->nextNode.push_back(son);
            son->name = realName;
            son->path = root->path + realName + "/";
            son->fileSize = rootEntry_ptr->DIR_FileSize;
            son->isFile = true;
            root->fileCnt++;
            getFileContent(fat12, rootEntry_ptr->DIR_FstClus, son); //获取文件内容
        }
            //处理目录
        else {
            int index = -1;
            for (char j: rootEntry_ptr->DIR_Name) {
                if (j != ' ') {
                    index++;
                    realName[index] = j;
                } else {
                    index++;
                    realName[index] = '\0';
                    break;
                }
            }
            Node *son = new Node();
            root->nextNode.push_back(son);
            son->name = realName;
            son->path = root->path + realName + "/";
            root->dirCnt++;
            createDirNode(son);
            readChildren(fat12, rootEntry_ptr->DIR_FstClus, son); //递归读取子目录
        }
    }
}

void getInputList(string &input, vector<string> &inputList, char flag) {
    inputList.clear(); //清空上次的缓存
    int start = 0;
    int end = 0;
    for (int i = 0; i < input.size(); i++) {
        if (input[i] == flag && start == end) {
            start++;
            end++;
        } else if (input[i] == flag && start < end) {
            inputList.push_back(input.substr(start, end - start));
            start = end + 1;
            end = start;
        } else {
            end++;
        }
    }
    //处理最后一个不为空格的字符串
    if (start < end) {
        inputList.push_back(input.substr(start, end - start));
    }
}

//创建目录节点,也就是添加"."和".."
void createDirNode(Node *node) {
    Node *dot = new Node();
    dot->name = ".";
    dot->isVal = false;
    node->nextNode.push_back(dot);
    dot = new Node();
    dot->name = "..";
    dot->isVal = false;
    node->nextNode.push_back(dot);
}

void readChildren(FILE *fat12, int startClus, Node *father) {
    //数据区的起始地址，保证了向上取整
    int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int clus = startClus;
    int value = 0; //value指向下一个簇号

    //0xFF8表示簇号的结束
    while (value < 0xFF8) {
        value = getFATValue(fat12, clus);
        if (value == 0xFF7) {
            myPrint("坏簇");
            break;
        }
        //0和1号簇是保留簇
        int start = base + (clus - 2) * SecPerClus * BytsPerSec;
        int bytesPerClus = SecPerClus * BytsPerSec;
        for (int i = 0; i < bytesPerClus; i += 32) {
            RootEntry sonEntry;
            RootEntry *sonEntry_ptr = &sonEntry;
            int seek_flag = fseek(fat12, start + i, SEEK_SET);
            int read_flag = fread(sonEntry_ptr, 1, 32, fat12);
            if (seek_flag != 0 || read_flag != 32) {
                myPrint("读取簇错误");
            }
            //空条目直接过
            if (sonEntry_ptr->DIR_Name[0] == '\0') {
                continue;
            }

            //考虑文件名是否合法
            if (!isLegalChar(sonEntry_ptr)) {
                continue;
            }
            //文件
            if ((sonEntry_ptr->DIR_Attr & 0x10) == 0) {
                char tempName[12];
                int index = -1;
                for (int j = 0; j < 11; j++) {
                    if (sonEntry_ptr->DIR_Name[j] != ' ') {
                        index++;
                        tempName[index] = sonEntry_ptr->DIR_Name[j];
                    } else {
                        index++;
                        tempName[index] = '.';
                        while (sonEntry_ptr->DIR_Name[j] == ' ') {
                            j++;
                        }
                        j--;
                    }
                }
                index++;
                tempName[index] = '\0';
                Node *son = new Node();
                father->nextNode.push_back(son);
                son->name = tempName;
                son->path = father->path + tempName + "/";
                son->fileSize = sonEntry_ptr->DIR_FileSize;
                son->isFile = true;
                father->fileCnt++;
                getFileContent(fat12, sonEntry_ptr->DIR_FstClus, son);
            }
                //目录
            else {
                char tempName[12];
                int index = -1;
                for (char j: sonEntry_ptr->DIR_Name) {
                    if (j != ' ') {
                        index++;
                        tempName[index] = j;
                    } else {
                        index++;
                        tempName[index] = '\0';
                    }
                }
                Node *son = new Node();
                father->nextNode.push_back(son);
                son->name = tempName;
                son->path = father->path + tempName + "/";
                father->dirCnt++;
                createDirNode(son);
                readChildren(fat12, sonEntry_ptr->DIR_FstClus, son);
            }
        }
        clus = value; //下一个簇
    }
}

int getFATValue(FILE *fat12, int clus) {
    int base = RsvdSecCnt * BytsPerSec;
    int position = base + clus * 3 / 2;

    int type = 0;
    if (clus % 2 == 0) {
        type = 0;
    } else {
        type = 1;
    }

    //一个簇占1.5个字节，先读取两个字节
    u16 bytes;
    u16 *bytes_ptr = &bytes;
    int seek_flag = fseek(fat12, position, SEEK_SET);
    int read_flag = fread(bytes_ptr, 1, 2, fat12);
    if (seek_flag != 0 || read_flag != 2) {
        myPrint("读取FAT的value错误");
    }

    //因为clus的存储很恶心.....
    //举个例子76543210和54321098，小端法的话就是5432109876543210
    if (type == 0) {
        bytes = bytes << 4;
        return bytes >> 4;
    } else {
        return bytes >> 4;
    }
}

bool isLegalChar(RootEntry *rootEntry_ptr) {
    for (char i: rootEntry_ptr->DIR_Name) {
        if (!((i >= '0' && i <= '9') ||
              (i >= 'A' && i <= 'Z') ||
              (i == ' ') ||
              (i >= 'a' && i <= 'z'))) {
            return false;
        } else {
            continue;
        }
    }
    return true;
}

void getFileContent(FILE *fat12, int startClus, Node *son) {
    int base = BytsPerSec * (RsvdSecCnt + NumFATs * FATSz + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
    int clus = startClus;
    int value = 0; //读取簇
    char *p = son->content;
    if (clus == 0) {
        return;
    }
    while (value < 0xFF8) {
        value = getFATValue(fat12, clus);
        if (value == 0xFF7) {
            myPrint("坏簇");
            break;
        }
        char *str = (char *) malloc(SecPerClus * BytsPerSec);
        char *content = str;

        int start = base + (clus - 2) * SecPerClus * BytsPerSec;
        int seek_flag = fseek(fat12, start, SEEK_SET);
        int read_flag = fread(content, 1, SecPerClus * BytsPerSec, fat12);
        if (seek_flag != 0 || read_flag != SecPerClus * BytsPerSec) {
            myPrint("读取文件内容错误");
        }

        for (int i = 0; i < SecPerClus * BytsPerSec; i++) {
            *p = content[i];
            p++;
        }
        free(str);
        clus = value;
    }
}

bool findFileName(Node *root, string name) {
    if (root->name == name) {
        return true;
    }
    bool flag = false;
    for (auto &i: root->nextNode) {
        flag = findFileName(i, name);
        if (flag == 1) {
            break;
        }
    }
    return flag;
}

void addPath(string &s) {
    if (s[0] != '/') {
        s = "/" + s;
    }

    if (s[s.length() - 1] != '/') {
        s += "/";
    }
}

void cat(Node *root, string source, int &mode) {
    if (source == root->path) {
        //成功查找
        if (root->isFile) {
            mode = 1;
            if (root->content[0] != 0) {
                myPrint(root->content);
                myPrint("\n");
            }
            return;
        } else {
            mode = 2;
            return;
        }
    }

    if (source.length() <= root->path.length()) {
        return;
    }

    string temp = source.substr(0, root->path.length());
    if (temp == root->path) {
        for (Node *node: root->nextNode) {
            cat(node, source, mode);
        }
    }
}

void printList(Node *node) {
    Node *father = node;
    if (father->isFile) {
        return;
    } else {
        str_print = father->path + ":\n";
        myPrint(str_print.c_str());
        str_print.clear();
        //打印每个next
        Node *son;
        int len = father->nextNode.size();
        for (int i = 0; i < len; i++) {
            son = father->nextNode[i];
            if (son->isFile) {
                str_print = son->name + "  ";
                myPrint(str_print.c_str());
                str_print.clear();
            } else {
                str_print = "\033[31m" + son->name + "\033[0m" + "  ";
                myPrint(str_print.c_str());
                str_print.clear();
            }
        }
        str_print = "\n";
        myPrint(str_print.c_str());
        str_print.clear();
        for (int i = 0; i < len; i++) {
            if (father->nextNode[i]->isVal) {
                printList(father->nextNode[i]);
            }
        }
    }
}

void printList_L(Node *root) {
    Node *father = root;
    if (father->isFile) {
        return;
    } else {
        str_print = father->path + " " + to_string(father->dirCnt) + " " + to_string(father->fileCnt) + ":\n";
        myPrint(str_print.c_str());
        str_print.clear();

        Node *son;
        int len = father->nextNode.size();
        for (int i = 0; i < len; i++) {
            son = father->nextNode[i];
            if (son->isFile) {
                str_print = son->name + "  " + to_string(son->fileSize) + "\n";
                myPrint(str_print.c_str());
                str_print.clear();
            } else {
                if (son->isVal) {
                    str_print = "\033[31m" + son->name + "\033[0m" + "  " + to_string(son->dirCnt) + " " +
                                to_string(son->fileCnt) + "\n";
                    myPrint(str_print.c_str());
                    str_print.clear();
                } else {
                    str_print = "\033[31m" + son->name + "\033[0m" + "  \n";
                    myPrint(str_print.c_str());
                    str_print.clear();
                }
            }
        }
        myPrint("\n");
        for (int i = 0; i < len; i++) {
            if (father->nextNode[i]->isVal) {
                printList_L(father->nextNode[i]);
            }
        }
    }
}

void printListWithPath(Node *root, string path, int &mode, bool hasL) {
    if (path == root->path) {
        if (root->isFile) {
            mode = 2;
            return;
        } else {
            mode = 1;
            if (hasL) {
                printList_L(root);
            } else {
                printList(root);
            }
        }
        return;
    }

    if (path.length() <= root->path.length()) {
        return;
    }
    string temp = path.substr(0, root->path.length());
    if (temp == root->path) {
        for (Node *q: root->nextNode) {
            printListWithPath(q, path, mode, hasL);
        }
    }
}