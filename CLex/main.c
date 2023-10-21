#include "l.h"
#include <errno.h>
#include <stdio.h>
int FCON = 1;
int ICON = 2;
typedef unsigned char	YY_TTYPE;
#define YYF		(( YY_TTYPE )(-1))

int yywrap() {
    //是否读入新文件，这里默认返回 0
    return 0;
}

char   *yytext;		/* Pointer to lexeme.           */
int    	yyleng;		/* Length of lexeme.            */
int    	yylineno;	/* Input line number.           */

/*--------------------------------------
* The Yy_cmap[] and Yy_rmap arrays are used as follows:
*
*  next_state= Yydtran[ Yy_rmap[current_state] ][ Yy_cmap[input_char] ];
*
* Character positions in the Yy_cmap array are:
*
*    ^@  ^A  ^B  ^C  ^D  ^E  ^F  ^G  ^H  ^I  ^J  ^K  ^L  ^M  ^N  ^O
*    ^P  ^Q  ^R  ^S  ^T  ^U  ^V  ^W  ^X  ^Y  ^Z  ^[  ^\  ^]  ^^  ^_
*         !   "   #   $   %   &   '   (   )   *   +   ,   -   .   /
*     0   1   2   3   4   5   6   7   8   9   :   ;   <   =   >   ?
*     @   A   B   C   D   E   F   G   H   I   J   K   L   M   N   O
*     P   Q   R   S   T   U   V   W   X   Y   Z   [   \   ]   ^   _
*     `   a   b   c   d   e   f   g   h   i   j   k   l   m   n   o
*     p   q   r   s   t   u   v   w   x   y   z   {   |   }   ~   DEL
*/

static unsigned char Yy_cmap[128]=
        {
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,1,0,2,2,
                2,2,2,2,2,2,2,2,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0
        };

static  unsigned char  Yy_rmap[6]=
        {
                0,1,2,3,4,5
        };

static unsigned char Yy_nxt[6][3]=
        {
/*  0 */ {-1, 2, 4},
/*  1 */ {-1, -1, -1},
/*  2 */ {-1, -1, 1},
/*  3 */ {-1, -1, 3},
/*  4 */ {-1, 3, 5},
/*  5 */ {-1, 2, 5}
        };

/*--------------------------------------
* yy_next(state,c) is given the current state number and input
* character and evaluates to the next state.
*/

#define yy_next(state, c) (Yy_nxt[Yy_rmap[state]][Yy_cmap[c]])

/*--------------------------------------
 * 输出基于 DFA 的跳转表,首先我们将生成一个 Yyaccept数组，如果 Yyaccept[i]取值为 0，
	那表示节点 i 不是接收态，如果它的值不是 0，那么节点是接受态，此时他的值对应以下几种情况：
	1 表示节点对应的正则表达式需要开头匹配，也就是正则表达式以符号^开始，2 表示正则表达式需要
	末尾匹配，也就是表达式以符号$结尾，3 表示同时开头和结尾匹配，4 表示不需要开头或结尾匹配
 */

YY_TTYPE Yyaccept[]=
        {
                0  ,  /*State 0  */
                0  ,  /*State 1  */
                0  ,  /*State 2  */
                4  ,  /*State 3  */
                4  ,  /*State 4  */
                0     /*State 5  */
        };

int yylex() {
    static int yystate = -1;
    int yylastaccept;
    int yyprev;
    int yynstate;
    int yylook;  //预读取字符
    int yyanchor;

    if(yystate == -1) {
        //将数据读入缓冲区
        ii_advance();
        //ii_advance 使得 Next 指针移动了一位，因此在我们还没有读入任何字符时，需要将其后退回去
        ii_pushback(1);
    }

    yystate = 0;
    yyprev = 0;
    yylastaccept = 0;
    ii_unterm();
    ii_mark_start();
    while(1) {
        /*
        * 这里我们采取贪婪算法，如果当前识别的字符串已经进入识别状态，
        * 但是还有字符可以读取，那么我们先缓存当前识别状态，然后继续识别后续字符，
        * 直到文件抵达末尾或者输入的字符导致识别失败，此时我们再返回到上次识别状态
        * 进行处理，这种方法让我们尽可能获得能进入完成状态的最长字符串
        */
        while(1) {
            yylook = ii_look(1);
            if (yylook != EOF) {
                yynstate = yy_next(yystate, yylook);
                break;
            } else {
                if (yylastaccept) {
                    /*
                     * 如果文件数据读取完毕，并且我们抵达过完成状态，那么设置下一个状态为
                     * 非法状态
                     */
                    yynstate = YYF;
                    break;
                }
                else if(yywrap()) {
                    yytext = "";
                    yyleng = 0;
                    return 0;
                }
                else {
                    ii_advance();
                    ii_pushback(1);
                }
            }
        }// inner while

        if (yynstate != YYF) {
            //跳转到下一个有效状态
            printf("Transation from state %d ", yystate);
            printf(" to state %d on <%c>\n", yynstate, yylook);

            if (ii_advance() < 0) {
                //缓冲区已满
                printf("Line %d, lexeme too long. Discarding extra characters.\n", ii_lineno());
                ii_flush(1);
            }

            yyanchor = Yyaccept[yynstate];
            if (yyanchor) {
                yyprev = yystate;
                yylastaccept = yynstate;
                ii_mark_end(); //完成一个字符串的识别
            }

            yystate = yynstate;
        } else {
            //跳转到无效状态，说明输入字符串不合法
            if (!yylastaccept) {
                //忽略掉非法字符
                printf("Ignoring bad input\n");
                ii_advance();
            } else {
                //回退到上一次接受状态
                ii_to_mark();
                if (yyanchor & 2) {
                    //末尾匹配,将末尾回车符号放回缓冲区
                    ii_pushback(1);
                }
                if (yyanchor & 1) {
                    //开头匹配，忽略掉字符串开头的回车符号
                    ii_move_start();
                }
                ii_term();
                //获取当前识别的字符串，极其长度和所在行号
                yytext = (char*) ii_text();
                yyleng = ii_length();
                yylineno = ii_lineno();

                printf("Accepting state%d, ", yylastaccept);
                printf("line %d: <%s>\n", yylineno, yytext);

                switch (yylastaccept) {
                    /*
                     * 这里根据接受状态执行其对应的代码，实际上这里的代码
                     * 后面会由 GoLex 生成
                     */
                    case 3:
                    case 4:
                        printf("%s is a float number", yytext);
                        return FCON;
                    default:
                        printf("internal error, yylex: unkonw accept state %d.\n", yylastaccept);
                        break;
                }
            }

            ii_unterm();
            yylastaccept = 0;
            yystate = yyprev;

        }

    }// outer while


}

int main() {
    int fd = ii_newfile("/Users/my/Documents/CLex/num.txt");
    if (fd == -1) {
        printf("value of errno: %d\n", errno);
    }
    yylex();
    return 0;
}
