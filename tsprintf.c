/*
  Tiny sprintf module
   for Embedded microcontrollers
   File Version : 1.0
   Yasuhiro ISHII 2005

   【バージョンアップ履歴】
   0.1 : とりあえずなんか動いた版
   0.2 : decimalの0表示対応   20050313
   0.3 : hexa decimalの0表示対応 20050313
   0.4 : ソース中の変なコード(^M)を削除した 20050503
   0.5 : tsprintf関数の変数 sizeを初期化するようにした 20050503
   0.6 : %dの負数対応,%xのunsigned処理化 20050522
   0.7 : %d,%xの桁数指定(%[n]d)/0補完指定(%0[n]d)対応 20050522
   0.8 : va_listで渡すvtsprintfを作成し、vsprintfをvtsprintfの親関数にした 20050522
   0.9 : hexで、値が0の時に桁が1になってしまうバグ修正 20050526
   1.0 : decで、値が0の時に桁が1になってしまうバグ修正 20050629
   
   printfの書式設定を簡易的なものにして実装してあるので使用時には
   説明書を確認して下さい。

*/

#include <stdarg.h>

#define vax_arg(ap,type) ((type)*ap++)

int tsprintf(char* buff,char* fmt,long *arg);

static int tsprintf_string(char* ,char* );
static int tsprintf_char(int ,char* );
static int tsprintf_decimal(signed long,char* ,int ,int );
static int tsprintf_hexadecimal(unsigned long ,char* ,int ,int ,int );

/*
  Tiny sprintf関数
*/

int tsprintf(char* buff,char* fmt,long *arg){
	int len;
	int size;
	int zeroflag,width;

	size = 0;
	len = 0;

	while(*fmt){
		if(*fmt=='%'){		/* % に関する処理 */
			zeroflag = width = 0;
			fmt++;

			if (*fmt == '0'){
				fmt++;
				zeroflag = 1;
			}
			if ((*fmt >= '0') && (*fmt <= '9')){
				width = *(fmt++) - '0';
			}

			/* printf ("zerof = %d,width = %d\n",zeroflag,width); */

			switch(*fmt){
			case 'd':		/* 10進数 */
				size = tsprintf_decimal(vax_arg(arg,signed long),buff,zeroflag,width);
				break;
			case 'x':		/* 16進数 0-f */
				size = tsprintf_hexadecimal(vax_arg(arg,unsigned long),buff,0,zeroflag,width);
				break;
			case 'X':		/* 16進数 0-F */
				size = tsprintf_hexadecimal(vax_arg(arg,unsigned long),buff,1,zeroflag,width);
				break;
			case 'c':		/* キャラクター */
				size = tsprintf_char(vax_arg(arg,int),buff);
				break;
			case 's':		/* ASCIIZ文字列 */
				size = tsprintf_string(vax_arg(arg,char*),buff);
				break;
			default:		/* コントロールコード以外の文字 */
				/* %%(%に対応)はここで対応される */
				len++;
				*(buff++) = *fmt;
				break;
			}
			len += size;
			buff += size;
			fmt++;
		} else {
			*(buff++) = *(fmt++);
			len++;
		}
	}

	*buff = '\0';		/* 終端を入れる */

	return (len);
}




/*
  数値 => 10進文字列変換
*/
static int tsprintf_decimal(signed long val,char* buff,int zf,int wd){
	int i;
	char tmp[10];
	char* ptmp = tmp + 10;
	int len = 0;
	int minus = 0;

	if (!val){		/* 指定値が0の場合 */
		*(ptmp--) = '0';
		len++;
	} else {
		/* マイナスの値の場合には2の補数を取る */
		if (val < 0){
			val = ~val;
			val++;
			minus = 1;
		}
		while (val){
			/* バッファアンダーフロー対策 */
			if (len >= 8){
				break;
			}
	
			*ptmp = (val % 10) + '0';
			val /= 10;
			ptmp--;
			len++;
		}

	}

	/* 符号、桁合わせに関する処理 */
	if (zf){
		if (minus){
			wd--;
		}
		while (len < wd){
			*(ptmp--) =  '0';
			len++;
		}
		if (minus){
			*(ptmp--) = '-';
			len++;
		}
	} else {
		if (minus){
			*(ptmp--) = '-';
			len++;
		}
		while (len < wd){
			*(ptmp--) =  ' ';
			len++;
		}
	}

	/* 生成文字列のバッファコピー */
	for (i=0;i<len;i++){
		*(buff++) = *(++ptmp);
	}

	return (len);
}

/*
  数値 => 16進文字列変換
*/
static int tsprintf_hexadecimal(unsigned long val,char* buff,
								int capital,int zf,int wd){
	int i;
	char tmp[10];
	char* ptmp = tmp + 10;
	int len = 0;
	char str_a;

	/* A～Fを大文字にするか小文字にするか切り替える */
	if (capital){
		str_a = 'A';
	} else {
		str_a = 'a';
	}
	
	if (!val){		/* 指定値が0の場合 */
		*(ptmp--) = '0';
		len++;
	} else {
		while (val){
			/* バッファアンダーフロー対策 */
			if (len >= 8){
				break;
			}

			*ptmp = (val % 16);
			if (*ptmp > 9){
				*ptmp += str_a - 10;
			} else {
				*ptmp += '0';
			}
		
			val >>= 4;		/* 16で割る */
			ptmp--;
			len++;
		}
	}
	while (len < wd){
		*(ptmp--) =  zf ? '0' : ' ';
		len++;
	}
		
	for (i=0;i<len;i++){
		*(buff++) = *(++ptmp);
	}

	return(len);
}

/*
  数値 => 1文字キャラクタ変換
*/
static int tsprintf_char(int ch,char* buff){
	*buff = (char)ch;
	return(1);
}

/*
  数値 => ASCIIZ文字列変換
*/
static int tsprintf_string(char* str,char* buff){
	int count = 0;
	while(*str){
		*(buff++) = *str;
		str++;
		count++;
	}
	return(count);
}

