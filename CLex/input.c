#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "debug.h"
#include <unistd.h>
#include "l.h"

#define COPY(d, s, a) memcpy(d, s, a)
//默认情况下字符串从控制台读入
#define STDIN 0

//最大运行程序预读取 16 个字符
#define MAXLOOK  16
//允许识别的最大字符串长度
#define MAXLEX  1024

//定义缓冲区长度
#define BUFSIZE  ((MAXLEX * 3) + (2 * MAXLOOK))
//缓冲区不足标志位
#define  DANGER  (End_buf - MAXLOOK)
//缓冲区末尾指针
#define END  (&Start_buf[BUFSIZE])
//没有可读的字符
#define  NO_MORE_CHARS()  (Eof_read && Next >= End_buf)

typedef unsigned char  uchar;
//存储读入数据的缓冲区
PRIVATE uchar Start_buf[BUFSIZE];
//缓冲区结尾
PRIVATE uchar *End_buf = END;
//当前读入字符位置
PRIVATE uchar *Next = END;
//当前识别字符串的起始位置
PRIVATE uchar *sMark = END;
//当前识别字符串的结束位置
PRIVATE uchar *eMark = END;
//上次已经识别字符串的起始位置
PRIVATE uchar *pMark = NULL;
//上次识别字符串所在的起始行号
PRIVATE int pLineno = 0;
//上次识别字符串的长度
PRIVATE int pLength = 0;
//读入的文件急病号
PRIVATE int Inp_file = STDIN;
//当前读取字符串的行号
PRIVATE int Lineno = 1;
//函数 mark_end()调用时所在行号
PRIVATE int Mline = 1;
//我们会使用字符'\0'去替换当前字符，下面变量用来存储被替换的字符
PRIVATE int Termchar = 0;
//读取到文件末尾的标志符
PRIVATE int Eof_read = 0;

/*
后面Openp 等函数直接使用文件打开，读取和关闭函数
*/

int ii_newfile(char* name) {
    /*
     打开输入文件，在这里指定要进行词法解析的代码文件。如果该函数没有被调用，那么系统默认从控制台读取输入。
     他将返回-1 如果指定文件打开失败，要不然就会返回被打开的文件句柄。在调用该接口打开新文件时，他会自动关闭
     上次由该接口打开的文件。
     */
    int fd = STDIN; //打开的文件句柄
    if (name) {
        fd = open(name, O_RDONLY);
    }
    if (fd != -1) {
        //打开新文件成功后关闭原先打开的文件
        if (Inp_file != STDIN) {
            close(Inp_file);
        }

        Inp_file = fd;
        Eof_read = 0;

        Next = END;
        sMark = END;
        pMark = NULL;
        End_buf = END;
        Lineno = 1;
        Mline = 1;
    }

   return fd;
}

//PUBLIC 在 debug.h 中定义为空，这里使用它只是为了代码上说明，函数可以被其他外部模块调用
PUBLIC uchar* ii_text() {
    //返回当前读取字符串的起始位置
    return sMark;
}

PUBLIC int ii_length() {
    //返回当前读取字符串的长度
    return (eMark - sMark);
}

PUBLIC int ii_lineno() {
    //返回当前读取字符串所在的行号
    return Lineno;
}

PUBLIC uchar* ii_ptext() {
    //返回上次读取的字符串起始位置
    return pMark;
}

PUBLIC int ii_plength() {
    //返回上次读取字符串的长度
    return pLength;
}

PUBLIC int ii_plineno() {
    //返回上次读取字符串所在行号
    return pLineno;
}

PUBLIC uchar* ii_mark_start() {
    //将当前 Next 所指的位置作为当前读取字符串的开始
    Mline = Lineno;
    eMark = sMark = Next;
    return sMark;
}

PUBLIC uchar* ii_mark_end() {
    //将当前 Next 所指位置作为当前读取字符串的结尾
    Mline = Lineno;
    eMark = Next;
    return eMark;
}

PUBLIC uchar* ii_move_start() {
    /*
     将当前识别字符串的起始地址向右挪动一位，这意味着丢弃当前读取字符串的第一个字符
     */
    if (sMark >= eMark) {
        /*
         需要确保当前识别的字符串起始指针不能越过其末尾指针
         */
        return NULL;
    } else {
        sMark += 1;
        return sMark;
    }
}

PUBLIC uchar* ii_to_mark() {
    /*
     * 将 Next 指针挪动 eMark 所在位置，这意味着我们放弃预读取的字符
     */
    Lineno = Mline;
    Next = eMark;
    return Next;
}

PUBLIC uchar* ii_mark_prev() {
    /*
     * 把当前读取的字符串设置为已经读取完成，将其转换成“上一个”已经识别的字符串，
     * 注意这个函数要再 ii_mark_start 调用前调用，因为 ii_mark_start 会更改 sMark 的值
     */

    pLineno = Lineno;
    pLength = eMark - sMark;
    pMark = sMark;
    return pMark;
}

int ii_advance() {
    /*
     *该函数返回当前 Next 指向的字符，然后将 Next 后移一位，如果Next 越过了 Danger 所在位置，、
     * 那么我们将促发缓冲区当前数据的移动，然后从文件中读入数据，将数据写入移动后空出来的位置
     */
    static int been_called = 0;
    if (!been_called) {

        /*
         * 该函数还没有被调用过，走到这里是第一次调用。我们在这里首先插入一个回车符。其目的在于，如果当前
         * 正则表达式需要开头匹配，那么就需要字符串起始时以回车开头
         */
        Next = sMark = eMark = END - 1;
        //开头先插入一个回车符，以便匹配正则表达式要求的开头匹配
        *Next = '\n';
        --Lineno;
        --Mline;
        been_called = 1;
    }

    if (NO_MORE_CHARS()) {
        //缓冲区没有数据可读取，而且文件也已经全部读取
        return 0;
    }

    if (!Eof_read && ii_flush(0) < 0) {
        /*
         * ii_flush 负责将缓冲区的数据进行迁移，然后从文件中读取数据，再把数据填入迁移空出来的空间
         * 如果当前缓冲区的数据已经读取完毕，但是从文件读取数据到缓冲区没有成功，那么直接返回-1
         */
        return -1;
    }

    if (*Next == '\n') {
        /*
         * 在函数第一次调用时，也就是 been_called 取值为 0 时，我们把 Lineno--设置成-1，
         * 这里将其重新设置为 0
         */
        Lineno++;
    }

   // int c = *Next;
    Next++;
    return *Next;
}

int ii_fillbuf(uchar* starting_at) {
    /*
     * 从文件读取数据然后写入 starting_at 起始的位置。一次从磁盘读入的字符数量必须是 MAXLEX 的倍数。
     * 如果从磁盘读取的数据不足 MAXLEX，那意味着我们已经读完整个文件
     */
    int need, got;
    need = ((END - starting_at) / MAXLEX) * MAXLEX;
    //如果处于调试模式则输出相关内容
    D(printf("Reading %d bytes\n", need);)
    if (need < 0) {
        //starting_at 的地址越过了缓冲区末尾
        return 0;
    }
    if (need == 0) {
        return 0;
    }
    got = read(Inp_file, starting_at, need);
    if (got == -1) {
        //文件读取出错
        return 0;
    }
    End_buf = starting_at + got;
    if (got < need) {
        //已经读完整个文件
        Eof_read = 1;
    }
    return got;
}

int ii_flush(int force) {
    /*
     * 如果 force 不是 0，那么强制刷新缓冲区
     */
    int copy_amt, shift_amt;
    uchar* left_edge;
    if (NO_MORE_CHARS()) {
        //没有多余数据可以读取则直接返回
        return 1;
    }

    if (Next >= DANGER || force) {
        left_edge = pMark? min(sMark, pMark) : sMark;
        shift_amt = left_edge - Start_buf;

        if (shift_amt < MAXLEX) {
            //一次读入的数据要求至少是 MAXLEX
            if (!force) {
                return -1;
            }
            /*
             * 如果要强制刷新，那么把 Next 前面的数据全部丢弃
             */
            left_edge = ii_mark_start();
            ii_mark_prev();
            shift_amt = left_edge - Start_buf;
        }

        copy_amt = End_buf - left_edge;
        //将 left_edge 后面的数据挪动到起始位置
        memmove(Start_buf, left_edge, copy_amt);
        if (!ii_fillbuf(Start_buf+copy_amt)) {
            return -1;
        }

        if (pMark) {
            pMark -= shift_amt;
        }

        sMark -= shift_amt;
        eMark -= shift_amt;
        Next -= shift_amt;
    }

    return 1;
}

int  ii_look(int n) {
    /*
     * 在基于 Next 的基础上获取前或后 n 个位置的字符,如果n=0，那么返回当前正在被读取的字符，
     * 由于每读取一个字符后，Next 会向前一个单位，因此当 n=0时，函数返回 Next-1 位置处的
     * 字符，因此它返回当前正在读取的字符
     */
    uchar* p;
    p = Next + (n-1);
    if (Eof_read && p >= End_buf) {
        //越过了缓冲区末尾直接返回-1
        return EOF;
    }

    return (p < Start_buf || p >= End_buf) ? 0 : *p;
}

int ii_pushback(int n) {
    /*
     * 将当前已经读取的字符进行回退，回退时位置不能越过 sMark,
     * 成功返回 1，失败返回 0
     */
    while (--n >= 0 && Next >= sMark) {
        Next -= 1;
        if (*Next == '\n' || !*Next) {
            --Lineno;
        }
    }

    if (Next < eMark) {
        eMark = Next;
        Mline = Lineno;
    }

    return (Next > sMark);
}

void ii_term() {
    /*
     * 为当前识别的字符串设置'\0'作为结尾，c 语言字符串都需要这个字符作为结尾
     */
    Termchar = *Next;
    *Next = '\0';
}

void ii_unterm() {
    /*
     * 恢复'\0'原来对应的字符,是函数 ii_term 的逆操作
     */
    if (Termchar) {
        *Next = Termchar;
        Termchar = 0;
    }
}

int ii_input() {
    /*
     * 该函数是对 ii_advance 的封装，ii_advance 没有考虑到调用了 ii_term 的情况。
     * 如果调用了 ii_term，那么 ii_advance 就有可能返回字符'\0'，该函数会先调用
     * ii_unterm 然后再调用 ii_advance
     */
    int rval;
    if (Termchar) {
        ii_unterm();
        rval = ii_advance();
        ii_mark_end();
        ii_term();
    } else {
        rval = ii_advance();
        ii_mark_end();
    }

    return rval;
}

void ii_unput(int c) {
    /*
     * 将字符 c 替换掉当前所读取字符
     */
    if (Termchar) {
        ii_unterm();
        if (ii_pushback(1)) {
            *Next = c;
        }
        ii_term();
    } else {
        if (ii_pushback(1)) {
            *Next = c;
        }
    }
}

int ii_lookahead(int n) {
    if (n == 1 && Termchar) {
        return Termchar;
    } else {
        return ii_look(n);
    }
}

int ii_flushbuf() {
    if (Termchar) {
        ii_unterm();
    }

    return ii_flush(1);
}






